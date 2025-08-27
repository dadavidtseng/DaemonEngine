//----------------------------------------------------------------------------------------------------
// NetworkSubsystem.cpp
//----------------------------------------------------------------------------------------------------

#include "Engine/Network/NetworkSubsystem.hpp"

#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Core/StringUtils.hpp"

// Windows networking headers - only in .cpp file
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "Engine/Core/EngineCommon.hpp"

#pragma comment(lib, "ws2_32.lib")

//----------------------------------------------------------------------------------------------------
NetworkSubsystem::NetworkSubsystem(sNetworkSubsystemConfig config)
    : m_config(std::move(config))
{
    m_networkClock = new Clock(Clock::GetSystemClock());
}

//----------------------------------------------------------------------------------------------------
NetworkSubsystem::~NetworkSubsystem()
{
    ShutDown();
}

//----------------------------------------------------------------------------------------------------
void NetworkSubsystem::StartUp()
{
    // Allocate buffers
    m_recvBuffer = static_cast<char*>(malloc(m_config.recvBufferSize));
    m_sendBuffer = static_cast<char*>(malloc(m_config.sendBufferSize));

    if (!m_recvBuffer || !m_sendBuffer) ERROR_AND_DIE("Failed to allocate network buffers")

    // Initialize Winsock
    InitializeWinsock();

    // Determine mode from config
    if (m_config.m_mode == eNetworkMode::CLIENT)
    {
        LogMessage("NetworkSubsystem initialized as CLIENT");
        CreateClientSocket();
    }
    else if (m_config.m_mode == eNetworkMode::SERVER)
    {
        LogMessage("NetworkSubsystem initialized as SERVER");
        CreateServerSocket();
    }
    else
    {
        LogMessage("NetworkSubsystem initialized in NONE mode");
    }
}

//----------------------------------------------------------------------------------------------------
void NetworkSubsystem::BeginFrame()
{
    if (m_mode == eNetworkMode::NONE) return;

    // Clear previous frame's incoming messages
    m_incomingMessages.clear();

    if (m_mode == eNetworkMode::CLIENT)
    {
        // Client connection logic
        if (m_connectionState == eConnectionState::CONNECTING || m_connectionState == eConnectionState::DISCONNECTED)
        {
            // Attempt connection
            sockaddr_in addr          = {};
            addr.sin_family           = AF_INET;
            addr.sin_addr.S_un.S_addr = htonl(m_hostAddress);
            addr.sin_port             = htons(m_hostPort);

            int result = connect(m_clientSocket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));

            // Check connection status with select
            fd_set writeSockets, exceptSockets;
            FD_ZERO(&writeSockets);
            FD_ZERO(&exceptSockets);
            FD_SET((SOCKET)m_clientSocket, &writeSockets);
            FD_SET((SOCKET)m_clientSocket, &exceptSockets);

            timeval waitTime = {};
            result           = select(0, nullptr, &writeSockets, &exceptSockets, &waitTime);

            if (result >= 0)
            {
                if (FD_ISSET(m_clientSocket, &writeSockets))
                {
                    m_connectionState = eConnectionState::CONNECTED;
                    if (!ProcessClientMessages())
                    {
                        return;
                    }
                }
                else if (FD_ISSET(m_clientSocket, &exceptSockets))
                {
                    // Connection attempt, check for errors
                    if (!DealWithSocketError(m_clientSocket))
                    {
                        m_connectionState = eConnectionState::DISCONNECTED;
                    }
                }
            }
        }
        else if (m_connectionState == eConnectionState::CONNECTED)
        {
            if (!ProcessClientMessages())
            {
                return;
            }
        }

        // Log connection state changes
        if (m_lastFrameConnectionState != m_connectionState)
        {
            if (m_lastFrameConnectionState == eConnectionState::DISCONNECTED && m_connectionState == eConnectionState::CONNECTED)
            {
                LogMessage(Stringf("Connected to server %s! Socket: %llu", m_config.hostAddressString.c_str(), m_clientSocket));
            }
            else if (m_lastFrameConnectionState == eConnectionState::CONNECTED && m_connectionState == eConnectionState::DISCONNECTED)
            {
                LogMessage(Stringf("Disconnected from server %s! Socket: %llu", m_config.hostAddressString.c_str(), m_clientSocket));
            }
        }
        m_lastFrameConnectionState = m_connectionState;
    }
    else if (m_mode == eNetworkMode::SERVER)
    {
        // Server: accept new connections and process existing ones
        ProcessIncomingConnections();

        if (!ProcessServerMessages())
        {
            return;
        }

        CheckClientConnections();
    }
}

//----------------------------------------------------------------------------------------------------
void NetworkSubsystem::EndFrame()
{
    // Nothing specific needed for end frame currently
}

//----------------------------------------------------------------------------------------------------
void NetworkSubsystem::Update()
{
    if (m_mode == eNetworkMode::NONE) return;

    float const deltaSeconds = static_cast<float>(m_networkClock->GetDeltaSeconds());

    // Update heartbeat system
    if (m_config.enableHeartbeat)
    {
        ProcessHeartbeat(deltaSeconds);
    }
}

//----------------------------------------------------------------------------------------------------
void NetworkSubsystem::ShutDown()
{
    if (m_mode == eNetworkMode::CLIENT)
    {
        if (m_clientSocket != ~0ull)
        {
            shutdown(m_clientSocket, SD_BOTH);
            closesocket(m_clientSocket);
            m_clientSocket = ~0ull;
        }
    }

    // return allSuccess;
}

//----------------------------------------------------------------------------------------------------
void NetworkSubsystem::ProcessIncomingConnections()
{
    if (m_listenSocket == ~0ull) return;

    if (m_clientList.size() >= (size_t)m_config.maxClients) return; // Already at max capacity

    SOCKET newClientSocket = accept(m_listenSocket, nullptr, nullptr);
    if (newClientSocket != INVALID_SOCKET)
    {
        // Set non-blocking mode
        SetSocketNonBlocking(newClientSocket);

        // Create a new client connection
        sClientConnection newClient;
        newClient.m_socket            = newClientSocket;
        newClient.m_clientId          = m_nextClientId++;
        newClient.m_state             = eConnectionState::CONNECTED;
        newClient.m_address           = "Unknown"; // Could get actual IP if needed
        newClient.m_port              = 0;
        newClient.m_lastHeartbeatTime = 0.0f;
        newClient.m_recvQueue         = "";  // Initialize empty receive queue

        m_clientList.push_back(newClient);
        m_connectionsAccepted++;

        LogMessage(Stringf("Client %d connected! Socket: %llu", newClient.m_clientId, newClient.m_socket));

        // Fire connection event
        if (g_eventSystem)
        {
            EventArgs args;
            args.SetValue("clientId", std::to_string(newClient.m_clientId));
            g_eventSystem->FireEvent("ClientConnected", args);
        }
    }
}

//----------------------------------------------------------------------------------------------------
void NetworkSubsystem::CheckClientConnections()
{
    // Remove disconnected clients
    for (auto it = m_clientList.begin(); it != m_clientList.end();)
    {
        if (it->m_state == eConnectionState::DISCONNECTED || it->m_state == eConnectionState::ERROR_STATE)
        {
            LogMessage(Stringf("Client %d disconnected", it->m_clientId));

            // Fire disconnection event
            if (g_eventSystem)
            {
                EventArgs args;
                args.SetValue("clientId", std::to_string(it->m_clientId));
                g_eventSystem->FireEvent("ClientDisconnected", args);
            }

            if (it->m_socket != ~0ull)
            {
                closesocket(it->m_socket);
            }

            it = m_clientList.erase(it);
            m_connectionsLost++;
        }
        else
        {
            ++it;
        }
    }
}

//----------------------------------------------------------------------------------------------------
bool NetworkSubsystem::SendRawDataToSocket(uintptr_t const socket,
                                           String const&   data)
{
    int const result = send(socket, data.c_str(), (int)data.length() + 1, 0);

    if (result <= 0)
    {
        return DealWithSocketError(socket);
    }

    return true;
}

//----------------------------------------------------------------------------------------------------
String NetworkSubsystem::ReceiveRawDataFromSocket(uintptr_t const socket)
{
    int const result = recv(socket, m_recvBuffer, m_config.recvBufferSize - 1, 0);

    if (result > 0)
    {
        m_recvBuffer[result] = '\0';
        return String(m_recvBuffer, result);
    }
    else if (result == 0)
    {
        // Connection closed
        for (auto& client : m_clientList)
        {
            if (client.m_socket == socket)
            {
                client.m_state = eConnectionState::DISCONNECTED;
                break;
            }
        }
    }
    else
    {
        DealWithSocketError(socket);
    }

    return "";
}

//----------------------------------------------------------------------------------------------------
void NetworkSubsystem::ExecuteReceivedMessage(String const& message,
                                              int const     fromClientId)
{
    // Try to deserialize as NetworkMessage first
    sNetworkMessage netMsg = DeserializeMessage(message, fromClientId);
    if (!netMsg.m_messageType.empty())
    {
        QueueIncomingMessage(netMsg);

        // Fire specific events based on message type
        if (g_eventSystem)
        {
            EventArgs args;
            args.SetValue("messageType", netMsg.m_messageType);
            args.SetValue("data", netMsg.m_data);
            args.SetValue("fromClientId", std::to_string(fromClientId));

            if (netMsg.m_messageType == "RemoteCommand")
            {
                // 處理 RemoteCommand：在命令字串後面加上 remote=true
                String commandToExecute = netMsg.m_data + " remote=true";

                // 在 DevConsole 中執行命令
                if (g_devConsole)
                {
                    g_devConsole->Execute(commandToExecute);

                    // 記錄接收到的遠端命令
                    g_devConsole->AddLine(DevConsole::INFO_MAJOR,
                                             Stringf("[Network] Received remote command from client %d: %s",
                                                     fromClientId, netMsg.m_data.c_str()));
                }
            }
            else if (netMsg.m_messageType == "GameData")
            {
                g_eventSystem->FireEvent("GameDataReceived", args);
            }
            else if (netMsg.m_messageType == "ChatMessage")
            {
                g_eventSystem->FireEvent("ChatMessageReceived", args);
            }
            else if (netMsg.m_messageType == "Heartbeat")
            {
                ProcessHeartbeatMessage(fromClientId);
            }

            g_eventSystem->FireEvent("NetworkMessageReceived", args);
        }
    }
    else
    {
        // Fall back to executing as console command (legacy behavior)
        if (g_devConsole)
        {
            g_devConsole->Execute(message, true);
        }
    }
}

//----------------------------------------------------------------------------------------------------
void NetworkSubsystem::QueueIncomingMessage(sNetworkMessage const& message)
{
    m_incomingMessages.push_back(message);
}

//----------------------------------------------------------------------------------------------------
bool NetworkSubsystem::DealWithSocketError(uintptr_t socket, int clientId)
{
    UNUSED(clientId)
    int const error = WSAGetLastError();

    if (error == WSAECONNABORTED || error == WSAECONNRESET || error == 0)
    {
        if (m_mode == eNetworkMode::SERVER)
        {
            // Find and disconnect the client
            for (auto& client : m_clientList)
            {
                if (client.m_socket == socket)
                {
                    LogMessage(Stringf("Client %d disconnected due to connection error", client.m_clientId));
                    client.m_state = eConnectionState::DISCONNECTED;
                    break;
                }
            }
        }
        else if (m_mode == eNetworkMode::CLIENT)
        {
            m_connectionState = eConnectionState::DISCONNECTED;
            if (m_clientSocket == socket)
            {
                closesocket((SOCKET)m_clientSocket);
                CreateClientSocket(); // Recreate socket for reconnection attempts
            }
        }
        return false;
    }
    else if (error == WSAEWOULDBLOCK || error == WSAEALREADY)
    {
        return true; // Non-blocking operation, continue
    }
    else
    {
        LogError(Stringf("Socket error code: %d", error));
        return false;
    }
}

//----------------------------------------------------------------------------------------------------
void NetworkSubsystem::CloseClientConnection(int clientId)
{
    for (auto& client : m_clientList)
    {
        if (client.m_clientId == clientId)
        {
            if (client.m_socket != (uintptr_t)~0ull)
            {
                shutdown((SOCKET)client.m_socket, SD_BOTH);
                closesocket((SOCKET)client.m_socket);
                client.m_socket = (uintptr_t)~0ull;
            }
            client.m_state = eConnectionState::DISCONNECTED;
            break;
        }
    }
}

//----------------------------------------------------------------------------------------------------
void NetworkSubsystem::CloseAllConnections()
{
    for (auto& client : m_clientList)
    {
        if (client.m_socket != (uintptr_t)~0ull)
        {
            shutdown((SOCKET)client.m_socket, SD_BOTH);
            closesocket((SOCKET)client.m_socket);
        }
    }
    m_clientList.clear();
}

//----------------------------------------------------------------------------------------------------
void NetworkSubsystem::ProcessHeartbeat(float const deltaSeconds)
{
    m_heartbeatTimer += deltaSeconds;
    m_lastHeartbeatReceived += deltaSeconds;

    // Send heartbeat periodically
    if (m_heartbeatTimer >= m_config.heartbeatInterval)
    {
        SendHeartbeat();
        m_heartbeatTimer = 0.0f;
    }

    // Check for heartbeat timeout (client only)
    if (m_mode == eNetworkMode::CLIENT && m_lastHeartbeatReceived > m_config.heartbeatInterval * 3.0f)
    {
        LogMessage("Heartbeat timeout, disconnecting...");
        DisconnectFromServer();
    }
}

//----------------------------------------------------------------------------------------------------
void NetworkSubsystem::SendHeartbeat()
{
    sNetworkMessage heartbeat("Heartbeat", "", -1);
    std::string     serialized = SerializeMessage(heartbeat);

    if (m_mode == eNetworkMode::CLIENT && m_connectionState == eConnectionState::CONNECTED)
    {
        SendRawData(serialized);
    }
    else if (m_mode == eNetworkMode::SERVER)
    {
        for (auto& client : m_clientList)
        {
            if (client.m_state == eConnectionState::CONNECTED)
            {
                SendRawDataToSocket(client.m_socket, serialized);
            }
        }
    }
}

//----------------------------------------------------------------------------------------------------
void NetworkSubsystem::ProcessHeartbeatMessage(int const fromClientId)
{
    if (m_mode == eNetworkMode::CLIENT)
    {
        m_lastHeartbeatReceived = 0.0f;
    }
    else if (m_mode == eNetworkMode::SERVER)
    {
        // Update client's last heartbeat time
        for (auto& client : m_clientList)
        {
            if (client.m_clientId == fromClientId)
            {
                client.m_lastHeartbeatTime = 0.0f;
                break;
            }
        }
    }
}

//----------------------------------------------------------------------------------------------------
std::string NetworkSubsystem::SerializeMessage(const sNetworkMessage& message)
{
    // 清理訊息內容，移除可能造成問題的字符
    std::string cleanData = message.m_data;

    // 移除所有控制字符（除了正常的可列印字符）
    std::string filteredData;
    for (char c : cleanData)
    {
        // 只允許可列印字符和空格
        if ((c >= 32 && c <= 126) || c == ' ')
        {
            filteredData += c;
        }
    }

    // 簡化的序列化格式，使用更安全的分隔符
    std::string serialized = message.m_messageType + "|" + std::to_string(message.m_fromClientId) + "|" + filteredData + "\0";

    return serialized;
}

//----------------------------------------------------------------------------------------------------
sNetworkMessage NetworkSubsystem::DeserializeMessage(String const& data, int const fromClientId)
{
    // 移除尾部的 null terminator 和換行符
    std::string cleanData = data;
    while (!cleanData.empty() && (cleanData.back() == '\0' || cleanData.back() == '\n' || cleanData.back() == '\r'))
    {
        cleanData.pop_back();
    }

    StringList parts;
    SplitStringOnDelimiter(parts, cleanData, '|');

    if (parts.size() >= 3)
    {
        std::string messageType      = parts[0];
        int         originalClientId = atoi(parts[1].c_str());
        std::string messageData      = parts[2];

        // 再次清理 messageData
        std::string cleanMessageData;
        for (char c : messageData)
        {
            if ((c >= 32 && c <= 126) || c == ' ')
            {
                cleanMessageData += c;
            }
        }

        // 使用提供的 fromClientId for server mode, 原始的 for client mode
        int actualClientId = (m_mode == eNetworkMode::SERVER) ? fromClientId : originalClientId;

        return sNetworkMessage(messageType, cleanMessageData, actualClientId);
    }

    return sNetworkMessage(); // 空訊息表示解析失敗
}

//----------------------------------------------------------------------------------------------------
void NetworkSubsystem::ParseHostAddress(const std::string& hostString, std::string& out_IP, unsigned short& out_port) const
{
    StringList ipAndPort;
    SplitStringOnDelimiter(ipAndPort, hostString, ':');

    if (ipAndPort.size() >= 2)
    {
        out_IP   = ipAndPort[0];
        out_port = (unsigned short)atoi(ipAndPort[1].c_str());
    }
    else
    {
        out_IP   = "127.0.0.1";
        out_port = 3100;
    }
}

//----------------------------------------------------------------------------------------------------
void NetworkSubsystem::LogMessage(const std::string& message)
{
    if (m_config.enableConsoleOutput && g_devConsole)
    {
        g_devConsole->AddLine(Rgba8(255, 255, 255), "[NetworkSubsystem] " + message);
    }
}

//----------------------------------------------------------------------------------------------------
void NetworkSubsystem::LogError(const std::string& error)
{
    if (m_config.enableConsoleOutput && g_devConsole)
    {
        g_devConsole->AddLine(Rgba8(255, 0, 0), "[NetworkSubsystem ERROR] " + error);
    }
    else if (m_mode == eNetworkMode::SERVER)
    {
        CloseAllConnections();

        if (m_listenSocket != (uintptr_t)~0ull)
        {
            shutdown((SOCKET)m_listenSocket, SD_BOTH);
            closesocket((SOCKET)m_listenSocket);
            m_listenSocket = (uintptr_t)~0ull;
        }
    }

    CleanupWinsock();

    // Free buffers
    if (m_recvBuffer)
    {
        free(m_recvBuffer);
        m_recvBuffer = nullptr;
    }
    if (m_sendBuffer)
    {
        free(m_sendBuffer);
        m_sendBuffer = nullptr;
    }

    m_mode            = eNetworkMode::NONE;
    m_connectionState = eConnectionState::DISCONNECTED;

    LogMessage("NetworkSubsystem shut down");
}

//----------------------------------------------------------------------------------------------------
bool NetworkSubsystem::IsEnabled() const
{
    return m_connectionState != eConnectionState::DISABLED && m_mode != eNetworkMode::NONE;
}

//----------------------------------------------------------------------------------------------------
bool NetworkSubsystem::IsServer() const
{
    return m_mode == eNetworkMode::SERVER;
}

//----------------------------------------------------------------------------------------------------
bool NetworkSubsystem::IsClient() const
{
    return m_mode == eNetworkMode::CLIENT;
}

//----------------------------------------------------------------------------------------------------
bool NetworkSubsystem::IsConnected() const
{
    if (m_mode == eNetworkMode::CLIENT)
    {
        return m_connectionState == eConnectionState::CONNECTED;
    }
    if (m_mode == eNetworkMode::SERVER)
    {
        return !m_clientList.empty();
    }
    return false;
}

//----------------------------------------------------------------------------------------------------
eNetworkMode NetworkSubsystem::GetNetworkMode() const
{
    return m_mode;
}

//----------------------------------------------------------------------------------------------------
eConnectionState NetworkSubsystem::GetConnectionState() const
{
    return m_connectionState;
}

String NetworkSubsystem::GetCurrentIP() const
{
    String         currentIP;
    unsigned short currentPort;
    ParseHostAddress(m_config.hostAddressString, currentIP, currentPort);
    return currentIP;
}

unsigned short NetworkSubsystem::GetCurrentPort() const
{
    String         currentIP;
    unsigned short currentPort;
    ParseHostAddress(m_config.hostAddressString, currentIP, currentPort);
    return currentPort;
}

String NetworkSubsystem::GetHostAddressString() const
{
    return m_config.hostAddressString;
}

void NetworkSubsystem::SetCurrentIP(String const& newIP)
{
    if (m_mode != eNetworkMode::NONE)
    {
        LogError("Cannot change IP while in network mode. Disconnect first.");
        return;
    }

    std::string    currentIP;
    unsigned short currentPort;
    ParseHostAddress(m_config.hostAddressString, currentIP, currentPort);

    m_config.hostAddressString = newIP + ":" + std::to_string(currentPort);
    LogMessage(Stringf("IP set to %s (port remains %d)", newIP.c_str(), currentPort));
}

void NetworkSubsystem::SetCurrentPort(unsigned short newPort)
{
    if (m_mode != eNetworkMode::NONE)
    {
        LogError("Cannot change port while in network mode. Disconnect first.");
        return;
    }

    std::string    currentIP;
    unsigned short currentPort;
    ParseHostAddress(m_config.hostAddressString, currentIP, currentPort);

    m_config.hostAddressString = currentIP + ":" + std::to_string(newPort);
    LogMessage(Stringf("Port set to %d (IP remains %s)", newPort, currentIP.c_str()));
}

void NetworkSubsystem::SetHostAddressString(String const& newHostAddress)
{
    if (m_mode != eNetworkMode::NONE)
    {
        LogError("Cannot change host address while in network mode. Disconnect first.");
        return;
    }

    m_config.hostAddressString = newHostAddress;
    LogMessage(Stringf("Host address set to %s", m_config.hostAddressString.c_str()));
}

//----------------------------------------------------------------------------------------------------
bool NetworkSubsystem::StartServer(int const newPort)
{
    if (m_mode != eNetworkMode::NONE)
    {
        LogError("Cannot start server: already in network mode");
        return false;
    }

    m_mode = eNetworkMode::SERVER;

    if (newPort != -1)
    {
        // Update port in host address string
        String         ip;
        unsigned short oldPort;
        ParseHostAddress(m_config.hostAddressString, ip, oldPort);
        m_config.hostAddressString = ip + ":" + std::to_string(newPort);
        m_hostPort                 = (unsigned short)newPort;
    }

    InitializeWinsock();
    CreateServerSocket();

    m_connectionState = eConnectionState::CONNECTED;
    LogMessage(Stringf("Server started on port %d", m_hostPort));

    return true;
}

//----------------------------------------------------------------------------------------------------
void NetworkSubsystem::StopServer()
{
    if (m_mode != eNetworkMode::SERVER) return;

    CloseAllConnections();

    if (m_listenSocket != ~0ull)
    {
        shutdown(m_listenSocket, SD_BOTH);
        closesocket(m_listenSocket);
        m_listenSocket = ~0ull;
    }

    m_mode            = eNetworkMode::NONE;
    m_connectionState = eConnectionState::DISCONNECTED;

    LogMessage("Server stopped");
}

//----------------------------------------------------------------------------------------------------
int NetworkSubsystem::GetConnectedClientCount() const
{
    if (m_mode != eNetworkMode::SERVER) return 0;

    int count = 0;
    for (const auto& client : m_clientList)
    {
        if (client.m_state == eConnectionState::CONNECTED) count++;
    }
    return count;
}

//----------------------------------------------------------------------------------------------------
std::vector<int> NetworkSubsystem::GetConnectedClientIds() const
{
    std::vector<int> clientIds;

    if (m_mode != eNetworkMode::SERVER) return clientIds;

    for (sClientConnection const& client : m_clientList)
    {
        if (client.m_state == eConnectionState::CONNECTED) clientIds.push_back(client.m_clientId);
    }

    return clientIds;
}

//----------------------------------------------------------------------------------------------------
bool NetworkSubsystem::ConnectToServer(const std::string& address, int port)
{
    if (m_mode != eNetworkMode::NONE)
    {
        LogError("Cannot connect to server: already in network mode");
        return false;
    }

    m_mode                     = eNetworkMode::CLIENT;
    m_config.hostAddressString = address + ":" + std::to_string(port);

    InitializeWinsock();
    CreateClientSocket();

    m_connectionState = eConnectionState::CONNECTING;
    LogMessage(Stringf("Attempting to connect to %s:%d", address.c_str(), port));

    return true;
}

//----------------------------------------------------------------------------------------------------
void NetworkSubsystem::DisconnectFromServer()
{
    if (m_mode != eNetworkMode::CLIENT) return;

    if (m_clientSocket != (uintptr_t)~0ull)
    {
        shutdown((SOCKET)m_clientSocket, SD_BOTH);
        closesocket((SOCKET)m_clientSocket);
        m_clientSocket = (uintptr_t)~0ull;
    }

    m_mode            = eNetworkMode::NONE;
    m_connectionState = eConnectionState::DISCONNECTED;

    LogMessage("Disconnected from server");
}

//----------------------------------------------------------------------------------------------------
void NetworkSubsystem::SendRawData(const std::string& data)
{
    m_sendQueue.push_back(data);
}

//----------------------------------------------------------------------------------------------------
void NetworkSubsystem::SendGameData(const std::string& gameData, int targetClientId)
{
    sNetworkMessage message("GameData", gameData, targetClientId);
    std::string     serialized = SerializeMessage(message);

    if (m_mode == eNetworkMode::CLIENT)
    {
        SendRawData(serialized);
    }
    else if (m_mode == eNetworkMode::SERVER)
    {
        if (targetClientId == -1)
        {
            // Broadcast to all clients
            for (auto& client : m_clientList)
            {
                if (client.m_state == eConnectionState::CONNECTED)
                {
                    SendRawDataToSocket(client.m_socket, serialized);
                }
            }
        }
        else
        {
            // Send to specific client
            SendMessageToClient(targetClientId, message);
        }
    }
}

//----------------------------------------------------------------------------------------------------
void NetworkSubsystem::SendChatMessage(const std::string& message, int targetClientId)
{
    sNetworkMessage chatMsg("ChatMessage", message, targetClientId);
    std::string     serialized = SerializeMessage(chatMsg);

    if (m_mode == eNetworkMode::CLIENT)
    {
        SendRawData(serialized);
    }
    else if (m_mode == eNetworkMode::SERVER)
    {
        if (targetClientId == -1)
        {
            // Broadcast to all clients
            for (auto& client : m_clientList)
            {
                if (client.m_state == eConnectionState::CONNECTED)
                {
                    SendRawDataToSocket(client.m_socket, serialized);
                }
            }
        }
        else
        {
            SendMessageToClient(targetClientId, chatMsg);
        }
    }
}

//----------------------------------------------------------------------------------------------------
bool NetworkSubsystem::SendMessageToClient(int const clientId, const sNetworkMessage& message)
{
    if (m_mode != eNetworkMode::SERVER) return false;

    for (auto& client : m_clientList)
    {
        if (client.m_clientId == clientId && client.m_state == eConnectionState::CONNECTED)
        {
            std::string serialized = SerializeMessage(message);
            return SendRawDataToSocket(client.m_socket, serialized);
        }
    }
    return false;
}

//----------------------------------------------------------------------------------------------------
bool NetworkSubsystem::SendMessageToAllClients(sNetworkMessage const& message)
{
    if (m_mode != eNetworkMode::SERVER) return false;

    std::string serialized = SerializeMessage(message);
    bool        allSuccess = true;

    for (auto& client : m_clientList)
    {
        if (client.m_state == eConnectionState::CONNECTED)
        {
            if (!SendRawDataToSocket(client.m_socket, serialized))
            {
                allSuccess = false;
            }
        }
    }
    return allSuccess;
}

//----------------------------------------------------------------------------------------------------
bool NetworkSubsystem::SendMessageToServer(sNetworkMessage const& message)
{
    if (m_mode != eNetworkMode::CLIENT || m_connectionState != eConnectionState::CONNECTED) return false;

    String serialized = SerializeMessage(message);
    SendRawData(serialized);
    return true;
}

//----------------------------------------------------------------------------------------------------
bool NetworkSubsystem::HasPendingMessages() const
{
    return !m_incomingMessages.empty();
}

//----------------------------------------------------------------------------------------------------
sNetworkMessage NetworkSubsystem::GetNextMessage()
{
    if (m_incomingMessages.empty()) return sNetworkMessage{};

    sNetworkMessage message = m_incomingMessages.front();
    m_incomingMessages.pop_front();
    return message;
}

//----------------------------------------------------------------------------------------------------
void NetworkSubsystem::ClearMessageQueue()
{
    m_incomingMessages.clear();
}

//----------------------------------------------------------------------------------------------------
// Engage the network adapter and start a network interface instance for this program.
// Windows Sockets API Version 2.2 (0x00000202)
void NetworkSubsystem::InitializeWinsock()
{
    if (m_winsockInitialized) return;

    WSADATA   data;
    int const result = WSAStartup(MAKEWORD(2, 2), &data);

    if (result != 0) ERROR_AND_DIE(Stringf("WSAStartup failed with error: %d", result))

    m_winsockInitialized = true;
}

//----------------------------------------------------------------------------------------------------
void NetworkSubsystem::CleanupWinsock()
{
    if (m_winsockInitialized)
    {
        WSACleanup();
        m_winsockInitialized = false;
    }
}

//----------------------------------------------------------------------------------------------------
void NetworkSubsystem::CreateClientSocket()
{
    m_clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_clientSocket == INVALID_SOCKET)
    {
        ERROR_AND_DIE(Stringf("Error creating client socket: %ld", WSAGetLastError()))
    }

    SetSocketNonBlocking(m_clientSocket);

    // Parse host address
    std::string ip;
    ParseHostAddress(m_config.hostAddressString, ip, m_hostPort);

    IN_ADDR   addr   = {};
    int const result = inet_pton(AF_INET, ip.c_str(), &addr);
    if (result <= 0)
    {
        LogError(Stringf("Invalid IP address: %s", ip.c_str()));
    }
    m_hostAddress = ntohl(addr.S_un.S_addr);
}

//----------------------------------------------------------------------------------------------------
void NetworkSubsystem::CreateServerSocket()
{
    m_listenSocket = (uintptr_t)socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_listenSocket == (uintptr_t)INVALID_SOCKET)
    {
        ERROR_AND_DIE(Stringf("Error creating server socket: %ld", WSAGetLastError()));
    }

    SetSocketNonBlocking(m_listenSocket);

    // Allow address reuse
    char yes = 1;
    setsockopt((SOCKET)m_listenSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    // Parse host address
    std::string ip;
    ParseHostAddress(m_config.hostAddressString, ip, m_hostPort);
    m_hostAddress = INADDR_ANY;

    // Bind to port
    sockaddr_in addr          = {};
    addr.sin_family           = AF_INET;
    addr.sin_addr.S_un.S_addr = htonl(m_hostAddress);
    addr.sin_port             = htons(m_hostPort);

    int result = bind((SOCKET)m_listenSocket, (sockaddr*)&addr, sizeof(addr));
    if (result == SOCKET_ERROR)
    {
        ERROR_AND_DIE(Stringf("bind failed with error: %d", WSAGetLastError()));
    }

    // Start listening
    result = listen((SOCKET)m_listenSocket, m_config.maxClients);
    if (result == SOCKET_ERROR)
    {
        ERROR_AND_DIE(Stringf("listen failed with error: %d", WSAGetLastError()));
    }
}

//----------------------------------------------------------------------------------------------------
bool NetworkSubsystem::SetSocketNonBlocking(uintptr_t const socket) const
{
    unsigned long blockingMode = 1;
    return ioctlsocket(socket, FIONBIO, &blockingMode) == 0;
}

//----------------------------------------------------------------------------------------------------
bool NetworkSubsystem::ProcessClientMessages()
{
    // Send queued messages
    while (!m_sendQueue.empty())
    {
        String const& data   = m_sendQueue.front();
        int const     result = send(m_clientSocket, data.c_str(), static_cast<int>(data.length()) + 1, 0);

        if (result > 0)
        {
            m_sendQueue.pop_front();
            m_messagesSent++;
        }
        else
        {
            if (!DealWithSocketError(m_clientSocket))
            {
                return false;
            }
            break;
        }
    }

    // Receive messages
    int const result = recv(m_clientSocket, m_recvBuffer, m_config.recvBufferSize - 1, 0);
    if (result > 0)
    {
        m_recvBuffer[result] = '\0';

        // Process received data (similar to original logic)
        StringList lines;
        bool       hasStringInIt = false;
        int        lastStrEnd    = 0;

        for (int i = 0; i < result; i++)
        {
            if (m_recvBuffer[i] == '\0')
            {
                hasStringInIt = true;
                lines.emplace_back(&m_recvBuffer[lastStrEnd]);
                lastStrEnd = i + 1;
            }
        }

        if (!hasStringInIt)
        {
            m_recvQueue += String(m_recvBuffer, result);
        }
        else if (lastStrEnd < result)
        {
            lines.emplace_back(&m_recvBuffer[lastStrEnd], result - lastStrEnd);
        }

        for (size_t i = 0; i < lines.size(); i++)
        {
            if (!m_recvQueue.empty() && i == 0)
            {
                m_recvQueue += lines[i];
                ExecuteReceivedMessage(m_recvQueue);
                m_recvQueue.clear();
            }
            else if (i == lines.size() - 1 && lastStrEnd < result)
            {
                m_recvQueue += lines[i];
            }
            else
            {
                ExecuteReceivedMessage(lines[i]);
            }
        }

        m_messagesReceived++;
    }
    else if (result == 0)
    {
        // Connection closed
        m_connectionState = eConnectionState::DISCONNECTED;
        return false;
    }
    else
    {
        if (!DealWithSocketError(m_clientSocket))
        {
            return false;
        }
    }

    return true;
}

//----------------------------------------------------------------------------------------------------
bool NetworkSubsystem::ProcessServerMessages()
{
    bool constexpr allSuccess = true;

    for (sClientConnection const& client : m_clientList)
    {
        if (client.m_state != eConnectionState::CONNECTED) continue;

        // Receive messages from this client
        String receivedData = ReceiveRawDataFromSocket(client.m_socket);
        if (!receivedData.empty())
        {
            ExecuteReceivedMessage(receivedData, client.m_clientId);
            m_messagesReceived++;
        }
    }

    // Send queued messages to all clients
    while (!m_sendQueue.empty())
    {
        String const& data      = m_sendQueue.front();
        bool          sentToAll = true;

        for (sClientConnection const& client : m_clientList)
        {
            if (client.m_state == eConnectionState::CONNECTED)
            {
                if (!SendRawDataToSocket(client.m_socket, data))
                {
                    sentToAll = false;
                }
            }
        }

        if (sentToAll)
        {
            m_sendQueue.pop_front();
            m_messagesSent++;
        }
        else
        {
            break; // Stop trying to send if we can't send to all clients
        }
    }
    return allSuccess;
}
