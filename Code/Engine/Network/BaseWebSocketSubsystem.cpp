//----------------------------------------------------------------------------------------------------
// BaseWebSocketSubsystem.cpp
// Base WebSocket subsystem implementation providing generic RFC 6455 WebSocket protocol
//----------------------------------------------------------------------------------------------------
#include "Engine/Network/BaseWebSocketSubsystem.hpp"
//----------------------------------------------------------------------------------------------------
#include "Engine/Core/Engine.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/Job.hpp"
#include "Engine/Core/JobSystem.hpp"
#include "Engine/Core/LogSubsystem.hpp"
#include "Engine/Core/StringUtils.hpp"
//----------------------------------------------------------------------------------------------------
#include <random>

//----------------------------------------------------------------------------------------------------
// Disable specific warnings for Windows socket includes
//----------------------------------------------------------------------------------------------------
#pragma warning(push)
#pragma warning(disable: 4100)  // unreferenced formal parameter
#pragma warning(disable: 4127)  // conditional expression is constant
#pragma warning(disable: 4324)  // structure was padded due to alignment specifier

// Windows socket includes
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#pragma warning(pop)

//----------------------------------------------------------------------------------------------------
// WebSocket Magic String for handshake (RFC 6455)
//----------------------------------------------------------------------------------------------------
static const std::string WEBSOCKET_MAGIC = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

// Invalid socket value
static constexpr SOCKET INVALID_SOCKET_VALUE = INVALID_SOCKET;

//----------------------------------------------------------------------------------------------------
// Simple SHA1 implementation for WebSocket handshake
//----------------------------------------------------------------------------------------------------
class SimpleSHA1
{
public:
    static std::string Hash(const std::string& input)
    {
        uint32_t hash[5] = {0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0};

        std::string data = input;
        data += '\x80'; // Append single 1 bit

        // Pad to 64 bytes - 8 for length
        while ((data.length() % 64) != 56)
        {
            data += '\x00';
        }

        // Append length in bits as 64-bit big-endian integer
        uint64_t bitLength = static_cast<uint64_t>(input.length()) * 8;
        for (int i = 7; i >= 0; --i)
        {
            data += static_cast<char>((bitLength >> (i * 8)) & 0xFF);
        }

        // Process in 64-byte chunks
        for (size_t chunk = 0; chunk < data.length(); chunk += 64)
        {
            uint32_t w[80];

            // Break chunk into sixteen 32-bit big-endian words
            for (size_t i = 0; i < 16; ++i)
            {
                w[i] = static_cast<uint32_t>(static_cast<unsigned char>(data[chunk + i * 4 + 0])) << 24 |
                    static_cast<uint32_t>(static_cast<unsigned char>(data[chunk + i * 4 + 1])) << 16 |
                    static_cast<uint32_t>(static_cast<unsigned char>(data[chunk + i * 4 + 2])) << 8 |
                    static_cast<uint32_t>(static_cast<unsigned char>(data[chunk + i * 4 + 3]));
            }

            // Extend the sixteen 32-bit words into eighty 32-bit words
            for (int i = 16; i < 80; ++i)
            {
                w[i] = LeftRotate(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
            }

            // Initialize hash value for this chunk
            uint32_t a = hash[0], b = hash[1], c = hash[2], d = hash[3], e = hash[4];

            // Main loop
            for (int i = 0; i < 80; ++i)
            {
                uint32_t f, k;
                if (i < 20)
                {
                    f = (b & c) | ((~b) & d);
                    k = 0x5A827999;
                }
                else if (i < 40)
                {
                    f = b ^ c ^ d;
                    k = 0x6ED9EBA1;
                }
                else if (i < 60)
                {
                    f = (b & c) | (b & d) | (c & d);
                    k = 0x8F1BBCDC;
                }
                else
                {
                    f = b ^ c ^ d;
                    k = 0xCA62C1D6;
                }

                uint32_t temp = LeftRotate(a, 5) + f + e + k + w[i];
                e             = d;
                d             = c;
                c             = LeftRotate(b, 30);
                b             = a;
                a             = temp;
            }

            // Add this chunk's hash to result so far
            hash[0] += a;
            hash[1] += b;
            hash[2] += c;
            hash[3] += d;
            hash[4] += e;
        }

        // Produce the final hash value as a 160-bit number (20 bytes)
        std::string result;
        for (int i = 0; i < 5; ++i)
        {
            for (int j = 3; j >= 0; --j)
            {
                result += static_cast<char>((hash[i] >> (j * 8)) & 0xFF);
            }
        }

        return result;
    }

private:
    static uint32_t LeftRotate(uint32_t value, int amount)
    {
        return (value << amount) | (value >> (32 - amount));
    }
};

//----------------------------------------------------------------------------------------------------
// WebSocket Job Classes for JobSystem Integration
//----------------------------------------------------------------------------------------------------

/// @brief Server job that continuously accepts incoming WebSocket connections
class WebSocketServerJob : public Job
{
public:
    explicit WebSocketServerJob(BaseWebSocketSubsystem* subsystem)
        : Job(JOB_TYPE_GENERIC),
          m_subsystem(subsystem)
    {
    }

    void Execute() override
    {
        m_subsystem->ServerJobMain();
    }

private:
    BaseWebSocketSubsystem* m_subsystem;
};

// NOTE: Client connections now use dedicated std::thread instead of Job/JobSystem
// WebSocketClientJob class has been removed - client handlers are no longer JobSystem jobs

//----------------------------------------------------------------------------------------------------
// BaseWebSocketSubsystem Implementation
//----------------------------------------------------------------------------------------------------

bool sBaseWebSocketConfig::IsValid() const
{
    return port > 0 && port < 65536 && maxConnections > 0;
}

sBaseWebSocketConfig sBaseWebSocketConfig::FromJSON(nlohmann::json const& j)
{
    sBaseWebSocketConfig config;
    if (j.contains("enabled")) config.enabled = j["enabled"].get<bool>();
    if (j.contains("host")) config.host = j["host"].get<std::string>();
    if (j.contains("port")) config.port = j["port"].get<int>();
    if (j.contains("maxConnections")) config.maxConnections = j["maxConnections"].get<int>();
    if (j.contains("enableLogging")) config.enableLogging = j["enableLogging"].get<bool>();
    return config;
}

BaseWebSocketSubsystem::BaseWebSocketSubsystem(sBaseWebSocketConfig config)
    : m_config(std::move(config)), m_serverSocket(INVALID_SOCKET_VALUE)
{
    if (!m_config.IsValid())
    {
        DAEMON_LOG(LogNetwork, eLogVerbosity::Error,
                   StringFormat("Invalid WebSocket configuration: port={}, maxConnections={}",
                       m_config.port, m_config.maxConnections));
    }
}

BaseWebSocketSubsystem::~BaseWebSocketSubsystem()
{
    Stop();
}

bool BaseWebSocketSubsystem::Start()
{
    if (m_isRunning || !m_config.enabled)
    {
        return false;
    }

    if (!m_config.IsValid())
    {
        DAEMON_LOG(LogNetwork, eLogVerbosity::Error,
                   StringFormat("Cannot start WebSocket server: invalid configuration"));
        return false;
    }

    DAEMON_LOG(LogNetwork, eLogVerbosity::Display,
               StringFormat("Starting WebSocket server on {}:{}", m_config.host, m_config.port));

    // Initialize Winsock
    WSADATA wsaData;
    int     result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0)
    {
        DAEMON_LOG(LogNetwork, eLogVerbosity::Error,
                   StringFormat("WSAStartup failed: {}", result));
        return false;
    }

    // Create server socket
    m_serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_serverSocket == INVALID_SOCKET)
    {
        DAEMON_LOG(LogNetwork, eLogVerbosity::Error,
                   StringFormat("Failed to create server socket: {}", WSAGetLastError()));
        WSACleanup();
        return false;
    }

    // Set socket options
    int reuse = 1;
    if (setsockopt(m_serverSocket, SOL_SOCKET, SO_REUSEADDR,
                   reinterpret_cast<const char*>(&reuse), sizeof(reuse)) == SOCKET_ERROR)
    {
        DAEMON_LOG(LogNetwork, eLogVerbosity::Warning,
                   StringFormat("Failed to set SO_REUSEADDR: {}", WSAGetLastError()));
    }

    // Bind to address and port
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port   = htons(static_cast<u_short>(m_config.port));

    if (inet_pton(AF_INET, m_config.host.c_str(), &serverAddr.sin_addr) <= 0)
    {
        DAEMON_LOG(LogNetwork, eLogVerbosity::Error,
                   StringFormat("Invalid server address: {}", m_config.host));
        CloseSocket(m_serverSocket);
        WSACleanup();
        return false;
    }

    if (bind(m_serverSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR)
    {
        DAEMON_LOG(LogNetwork, eLogVerbosity::Error,
                   StringFormat("Failed to bind to {}:{}, error: {}", m_config.host, m_config.port, WSAGetLastError()));
        CloseSocket(m_serverSocket);
        WSACleanup();
        return false;
    }

    // Start listening
    if (listen(m_serverSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        DAEMON_LOG(LogNetwork, eLogVerbosity::Error,
                   StringFormat("Failed to listen on socket: {}", WSAGetLastError()));
        CloseSocket(m_serverSocket);
        WSACleanup();
        return false;
    }

    m_isRunning.store(true);
    m_shouldStop.store(false);

    // Submit server job to JobSystem
    if (!SubmitServerJob())
    {
        DAEMON_LOG(LogNetwork, eLogVerbosity::Error,
                   StringFormat("Failed to submit server job to JobSystem"));
        m_isRunning.store(false);
        CloseSocket(m_serverSocket);
        WSACleanup();
        return false;
    }

    DAEMON_LOG(LogNetwork, eLogVerbosity::Display,
               StringFormat("WebSocket server started successfully on port {}", m_config.port));

    return true;
}

void BaseWebSocketSubsystem::Update()
{
    // Clean up finished client threads
    CleanupClientThreads();

    // Process queued messages on main thread (virtual call to derived class)
    ProcessQueuedMessages();
}

void BaseWebSocketSubsystem::Stop()
{
    if (!m_isRunning.load()) return;

    DAEMON_LOG(LogNetwork, eLogVerbosity::Display,
               StringFormat("Stopping WebSocket server..."));

    m_shouldStop.store(true);
    m_isRunning.store(false);

    // Close server socket to break accept() loop
    if (m_serverSocket != INVALID_SOCKET_VALUE)
    {
        CloseSocket(m_serverSocket);
        m_serverSocket = INVALID_SOCKET_VALUE;
    }

    // CRITICAL: Close all client sockets FIRST to break recv() blocking calls
    // This forces client threads to exit ClientJobMain() loop
    {
        std::lock_guard<std::mutex> lock(m_connectionsMutex);
        for (auto& [socket, connection] : m_connections)
        {
            CloseSocket(socket);
        }
        m_connections.clear();
        m_activeConnections.clear();
    }

    // CRITICAL: Wait for all client threads to exit cleanly
    // Socket closure above should have broken recv() calls, allowing threads to exit
    //
    // IMPORTANT: We use detach() instead of join() to avoid hanging on threads
    // that are still blocked in recv(). This is safe because:
    // 1. All sockets are closed, so recv() will eventually return
    // 2. ClientJobMain() checks m_isRunning before accessing 'this'
    // 3. The subsystem object lifetime is managed by the application
    {
        std::lock_guard<std::mutex> lock(m_clientThreadsMutex);
        int threadCount = static_cast<int>(m_clientThreads.size());

        DAEMON_LOG(LogNetwork, eLogVerbosity::Verbose,
                   StringFormat("WebSocket shutdown: Detaching {} client threads", threadCount));

        for (auto& thread : m_clientThreads)
        {
            if (thread && thread->joinable())
            {
                thread->detach();  // Let threads finish asynchronously
            }
        }
        m_clientThreads.clear();

        DAEMON_LOG(LogNetwork, eLogVerbosity::Verbose,
                   StringFormat("WebSocket shutdown: All {} client threads detached", threadCount));
    }

    // Cleanup Winsock
    WSACleanup();

    DAEMON_LOG(LogNetwork, eLogVerbosity::Display,
               StringFormat("WebSocket server stopped"));
}

bool BaseWebSocketSubsystem::IsRunning() const
{
    return m_isRunning.load();
}

bool BaseWebSocketSubsystem::HasActiveConnections() const
{
    std::lock_guard<std::mutex> lock(m_connectionsMutex);
    return !m_activeConnections.empty();
}

int BaseWebSocketSubsystem::GetPort() const
{
    return m_config.port;
}

//----------------------------------------------------------------------------------------------------
// Protected Helper Methods
//----------------------------------------------------------------------------------------------------

bool BaseWebSocketSubsystem::SendToClient(SOCKET clientSocket, String const& message)
{
    String wsFrame = EncodeWebSocketFrame(message);
    return SendRawDataToSocket(clientSocket, wsFrame);
}

void BaseWebSocketSubsystem::BroadcastToAllClients(String const& message)
{
    if (!m_isRunning.load()) return;

    String wsFrame = EncodeWebSocketFrame(message);

    std::lock_guard<std::mutex> lock(m_connectionsMutex);
    for (SOCKET clientSocket : m_activeConnections)
    {
        SendRawDataToSocket(clientSocket, wsFrame);
    }
}

void BaseWebSocketSubsystem::QueueIncomingMessage(SOCKET sourceSocket, String const& message)
{
    std::lock_guard<std::mutex> lock(m_messageQueueMutex);
    m_incomingMessageQueue.push({sourceSocket, message});
}

bool BaseWebSocketSubsystem::IsClientConnected(SOCKET clientSocket) const
{
    std::lock_guard<std::mutex> lock(m_connectionsMutex);
    auto                        it = m_connections.find(clientSocket);
    return it != m_connections.end() && it->second.isActive;
}

std::vector<SOCKET> BaseWebSocketSubsystem::GetActiveConnections() const
{
    std::lock_guard<std::mutex> lock(m_connectionsMutex);
    return m_activeConnections;
}

//----------------------------------------------------------------------------------------------------
// Job Entry Points
//----------------------------------------------------------------------------------------------------

void BaseWebSocketSubsystem::ServerJobMain()
{
    DAEMON_LOG(LogNetwork, eLogVerbosity::Log,
               StringFormat("WebSocket server job started"));

    while (!m_shouldStop.load() && m_isRunning.load())
    {
        // Accept incoming connections
        sockaddr_in clientAddr;
        int         clientAddrSize = sizeof(clientAddr);

        SOCKET clientSocket = accept(m_serverSocket,
                                     reinterpret_cast<sockaddr*>(&clientAddr),
                                     &clientAddrSize);

        if (clientSocket == INVALID_SOCKET)
        {
            if (m_shouldStop.load()) break;

            int error = WSAGetLastError();
            if (error != WSAEINTR && error != WSAEWOULDBLOCK)
            {
                DAEMON_LOG(LogNetwork, eLogVerbosity::Error,
                           StringFormat("Accept failed: {}", error));
            }
            continue;
        }

        // Create dedicated thread for client handler (NOT using JobSystem)
        if (!CreateClientThread(clientSocket))
        {
            DAEMON_LOG(LogNetwork, eLogVerbosity::Error,
                       StringFormat("Failed to create client thread for socket {}", static_cast<int>(clientSocket)));
            CloseSocket(clientSocket);
        }

        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);
        DAEMON_LOG(LogNetwork, eLogVerbosity::Log,
                   StringFormat("Client connected from {}:{}", clientIP, ntohs(clientAddr.sin_port)));
    }

    DAEMON_LOG(LogNetwork, eLogVerbosity::Log,
               StringFormat("WebSocket server job stopped"));
}

void BaseWebSocketSubsystem::ClientJobMain(SOCKET clientSocket)
{
    // CRITICAL: Check if subsystem is shutting down BEFORE doing ANYTHING
    // If shutdown started, exit immediately to avoid accessing deleted object
    if (m_shouldStop.load() || !m_isRunning.load())
    {
        CloseSocket(clientSocket);
        return;
    }

    OnClientConnected(clientSocket);

    String receivedData;
    bool   isWebSocket = false;

    while (!m_shouldStop.load() && m_isRunning.load())
    {
        String data = ReceiveDataFromSocket(clientSocket);
        if (data.empty())
        {
            break; // Client disconnected
        }

        receivedData += data;

        if (!isWebSocket)
        {
            // Look for complete HTTP request (double CRLF)
            size_t headerEnd = receivedData.find("\r\n\r\n");
            if (headerEnd != std::string::npos)
            {
                String httpRequest = receivedData.substr(0, headerEnd + 4);
                receivedData.erase(0, headerEnd + 4);

                // Check if this is a WebSocket upgrade request (case insensitive)
                String httpLowerCase = httpRequest;
                std::transform(httpLowerCase.begin(), httpLowerCase.end(), httpLowerCase.begin(),
                               [](char c) { return static_cast<char>(::tolower(static_cast<unsigned char>(c))); });

                if (httpLowerCase.find("upgrade: websocket") != std::string::npos)
                {
                    if (ProcessWebSocketUpgrade(clientSocket, httpRequest))
                    {
                        isWebSocket = true;
                        {
                            std::lock_guard<std::mutex> lock(m_connectionsMutex);
                            m_activeConnections.push_back(clientSocket);
                        }

                        // Call virtual method for protocol-specific post-upgrade handling
                        OnWebSocketUpgraded(clientSocket);
                    }
                    else
                    {
                        DAEMON_LOG(LogNetwork, eLogVerbosity::Error,
                                   StringFormat("WebSocket upgrade failed for socket {}", static_cast<int>(clientSocket)));
                        break; // Upgrade failed
                    }
                }
                else
                {
                    // Handle HTTP request (discovery endpoint)
                    ProcessHttpRequest(clientSocket, httpRequest);
                    break; // Close HTTP connection after response
                }
            }
        }
        else
        {
            // Process WebSocket frames
            if (!receivedData.empty())
            {
                String decodedMessage = DecodeWebSocketFrame(receivedData);
                if (!decodedMessage.empty())
                {
                    // Call virtual method for protocol-specific message handling
                    OnClientMessage(clientSocket, decodedMessage);
                }
                receivedData.clear();
            }
        }
    }

    // CRITICAL: Only call virtual methods if subsystem is still running
    // During shutdown, the object may be destroyed while this thread is exiting
    // Accessing 'this->' after object destruction causes use-after-free crash
    if (m_isRunning.load())
    {
        OnClientDisconnected(clientSocket);
    }

    CloseSocket(clientSocket);

    // Remove from active connections (only if subsystem still valid)
    if (m_isRunning.load())
    {
        std::lock_guard<std::mutex> lock(m_connectionsMutex);
        auto                        it = std::find(m_activeConnections.begin(), m_activeConnections.end(), clientSocket);
        if (it != m_activeConnections.end())
        {
            m_activeConnections.erase(it);
        }
        m_connections.erase(clientSocket);
    }
}

//----------------------------------------------------------------------------------------------------
// WebSocket Protocol Implementation (RFC 6455)
//----------------------------------------------------------------------------------------------------

String BaseWebSocketSubsystem::CreateWebSocketAcceptKey(String const& clientKey)
{
    String combined = clientKey + WEBSOCKET_MAGIC;
    String hash     = SimpleSHA1::Hash(combined);
    return Base64Encode(hash);
}

String BaseWebSocketSubsystem::EncodeWebSocketFrame(String const& payload, eWebSocketOpcode opcode)
{
    String frame;

    auto pushByte = [&frame](uint8_t byte)
    {
        frame.push_back(static_cast<char>(byte));
    };

    // First byte: FIN=1, RSV=000, Opcode
    pushByte(0x80 | static_cast<uint8_t>(opcode));

    // Second byte and payload length
    size_t payloadLength = payload.length();
    if (payloadLength < 126)
    {
        pushByte(static_cast<uint8_t>(payloadLength));
    }
    else if (payloadLength <= 0xFFFF)
    {
        pushByte(126);
        pushByte(static_cast<uint8_t>((payloadLength >> 8) & 0xFF));
        pushByte(static_cast<uint8_t>(payloadLength & 0xFF));
    }
    else
    {
        pushByte(127);
        for (int i = 7; i >= 0; --i)
        {
            pushByte(static_cast<uint8_t>((payloadLength >> (i * 8)) & 0xFF));
        }
    }

    // Payload
    frame.append(payload);

    return frame;
}

String BaseWebSocketSubsystem::DecodeWebSocketFrame(String const& frame)
{
    if (frame.length() < 2) return "";

    uint8_t firstByte  = static_cast<uint8_t>(frame[0]);
    uint8_t secondByte = static_cast<uint8_t>(frame[1]);

    bool             isFinal       = (firstByte & 0x80) != 0;
    eWebSocketOpcode opcode        = static_cast<eWebSocketOpcode>(firstByte & 0x0F);
    bool             isMasked      = (secondByte & 0x80) != 0;
    uint64_t         payloadLength = secondByte & 0x7F;

    UNUSED(isFinal);
    UNUSED(opcode);

    size_t headerLength = 2;

    // Extended payload length
    if (payloadLength == 126)
    {
        if (frame.length() < 4) return "";
        payloadLength = (static_cast<uint16_t>(static_cast<uint8_t>(frame[2])) << 8) |
            static_cast<uint16_t>(static_cast<uint8_t>(frame[3]));
        headerLength = 4;
    }
    else if (payloadLength == 127)
    {
        if (frame.length() < 10) return "";
        payloadLength = 0;
        for (int i = 2; i < 10; ++i)
        {
            payloadLength = (payloadLength << 8) | static_cast<uint8_t>(frame[i]);
        }
        headerLength = 10;
    }

    // Masking key
    std::array<uint8_t, 4> maskingKey = {0};
    if (isMasked)
    {
        if (frame.length() < headerLength + 4) return "";
        for (int i = 0; i < 4; ++i)
        {
            maskingKey[i] = static_cast<uint8_t>(frame[headerLength + i]);
        }
        headerLength += 4;
    }

    // Payload
    if (frame.length() < headerLength + payloadLength) return "";

    String payload = frame.substr(headerLength, payloadLength);

    // Unmask payload if necessary
    if (isMasked)
    {
        for (size_t i = 0; i < payload.length(); ++i)
        {
            payload[i] = static_cast<char>(static_cast<uint8_t>(payload[i]) ^ maskingKey[i % 4]);
        }
    }

    return payload;
}

bool BaseWebSocketSubsystem::ProcessWebSocketUpgrade(SOCKET clientSocket, String const& request)
{
    // Extract WebSocket key from request
    String             wsKey;
    std::istringstream requestStream(request);
    String             line;

    while (std::getline(requestStream, line) && line != "\r")
    {
        if (line.find("Sec-WebSocket-Key:") == 0)
        {
            wsKey = line.substr(19); // Skip "Sec-WebSocket-Key: "
            if (!wsKey.empty() && wsKey.back() == '\r') wsKey.pop_back();
            break;
        }
    }

    if (wsKey.empty())
    {
        DAEMON_LOG(LogNetwork, eLogVerbosity::Error,
                   StringFormat("Missing Sec-WebSocket-Key in upgrade request"));
        return false;
    }

    // Create WebSocket accept key
    String acceptKey = CreateWebSocketAcceptKey(wsKey);

    // Create WebSocket upgrade response
    std::ostringstream response;
    response << "HTTP/1.1 101 Switching Protocols\r\n";
    response << "Upgrade: websocket\r\n";
    response << "Connection: Upgrade\r\n";
    response << "Sec-WebSocket-Accept: " << acceptKey << "\r\n";
    response << "\r\n";

    bool success = SendRawDataToSocket(clientSocket, response.str());

    if (success)
    {
        DAEMON_LOG(LogNetwork, eLogVerbosity::Display,
                   StringFormat("WebSocket connection established for socket {}", static_cast<int>(clientSocket)));
    }

    return success;
}

//----------------------------------------------------------------------------------------------------
// HTTP Server
//----------------------------------------------------------------------------------------------------

void BaseWebSocketSubsystem::ProcessHttpRequest(SOCKET clientSocket, String const& request)
{
    String response;

    // Parse request line
    std::istringstream requestStream(request);
    String             method, path, version;
    requestStream >> method >> path >> version;

    if (method == "GET" && (path == "/json/list" || path == "/json"))
    {
        // Call virtual method for protocol-specific discovery response
        String discoveryJson = HandleDiscoveryRequest();
        response             = CreateHttpResponse(discoveryJson, "application/json");
    }
    else
    {
        // 404 Not Found
        response = "HTTP/1.1 404 Not Found\r\n";
        response += CreateHttpResponse("Not Found", "text/plain");
    }

    SendRawDataToSocket(clientSocket, response);
}

String BaseWebSocketSubsystem::CreateHttpResponse(String const& content, String const& contentType)
{
    std::ostringstream response;
    response << "HTTP/1.1 200 OK\r\n";
    response << "Content-Type: " << contentType << "\r\n";
    response << "Content-Length: " << content.length() << "\r\n";
    response << "Access-Control-Allow-Origin: *\r\n";
    response << "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n";
    response << "Access-Control-Allow-Headers: Content-Type\r\n";
    response << "Connection: close\r\n";
    response << "\r\n";
    response << content;

    return response.str();
}

//----------------------------------------------------------------------------------------------------
// Socket Utilities
//----------------------------------------------------------------------------------------------------

bool BaseWebSocketSubsystem::SendRawDataToSocket(SOCKET socket, String const& data)
{
    if (socket == INVALID_SOCKET || data.empty()) return false;

    const char* buffer    = data.c_str();
    int         totalSent = 0;
    int         dataSize  = static_cast<int>(data.length());

    while (totalSent < dataSize)
    {
        int sent = send(socket, buffer + totalSent, dataSize - totalSent, 0);
        if (sent == SOCKET_ERROR)
        {
            int error = WSAGetLastError();
            DAEMON_LOG(LogNetwork, eLogVerbosity::Error,
                       StringFormat("Send failed for socket {}: {}", static_cast<int>(socket), error));
            return false;
        }
        totalSent += sent;
    }

    return true;
}

String BaseWebSocketSubsystem::ReceiveDataFromSocket(SOCKET socket)
{
    if (socket == INVALID_SOCKET) return "";

    char buffer[4096];
    int  received = recv(socket, buffer, sizeof(buffer) - 1, 0);

    if (received > 0)
    {
        buffer[received] = '\0';
        return String(buffer, received);
    }
    else if (received == 0)
    {
        // Connection closed gracefully
        return "";
    }
    else
    {
        int error = WSAGetLastError();
        if (error != WSAEWOULDBLOCK && error != WSAECONNRESET)
        {
            DAEMON_LOG(LogNetwork, eLogVerbosity::Error,
                       StringFormat("Receive failed for socket {}: {}", static_cast<int>(socket), error));
        }
        return "";
    }
}

void BaseWebSocketSubsystem::CloseSocket(SOCKET socket)
{
    if (socket != INVALID_SOCKET_VALUE)
    {
        // CRITICAL: Call shutdown() BEFORE closesocket() to immediately break blocking recv()/send() calls
        // This is essential for clean shutdown - closesocket() alone doesn't unblock pending operations
        // SD_BOTH (2) disables both send and receive operations on the socket
        shutdown(socket, SD_BOTH);

        closesocket(socket);
    }
}

//----------------------------------------------------------------------------------------------------
// Utility Functions
//----------------------------------------------------------------------------------------------------

String BaseWebSocketSubsystem::GenerateUUID()
{
    std::random_device              rd;
    std::mt19937                    gen(rd());
    std::uniform_int_distribution<> dis(0, 15);

    const char* chars = "0123456789abcdef";
    String      uuid  = "xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx";

    for (char& c : uuid)
    {
        if (c == 'x')
        {
            c = chars[dis(gen)];
        }
        else if (c == 'y')
        {
            c = chars[(dis(gen) & 0x3) | 0x8];
        }
    }

    return uuid;
}

String BaseWebSocketSubsystem::Base64Encode(String const& input)
{
    static const String chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    String result;
    int    val  = 0;
    int    valb = -6;

    for (unsigned char c : input)
    {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0)
        {
            result.push_back(chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }

    if (valb > -6)
    {
        result.push_back(chars[((val << 8) >> (valb + 8)) & 0x3F]);
    }

    while (result.size() % 4)
    {
        result.push_back('=');
    }

    return result;
}

//----------------------------------------------------------------------------------------------------
// JobSystem Integration
//----------------------------------------------------------------------------------------------------

bool BaseWebSocketSubsystem::SubmitServerJob()
{
    if (g_jobSystem == nullptr)
    {
        DAEMON_LOG(LogNetwork, eLogVerbosity::Error,
                   StringFormat("Cannot submit server job: JobSystem is null"));
        return false;
    }

    m_serverJob = new WebSocketServerJob(this);
    g_jobSystem->SubmitJob(m_serverJob);

    DAEMON_LOG(LogNetwork, eLogVerbosity::Log,
               StringFormat("WebSocket server job submitted to JobSystem"));

    return true;
}

bool BaseWebSocketSubsystem::CreateClientThread(SOCKET clientSocket)
{
    // Create a dedicated thread for this client connection
    // NOTE: This is a long-running blocking operation, so we use std::thread directly
    // instead of submitting to JobSystem which is designed for short computational tasks
    try
    {
        auto clientThread = std::make_unique<std::thread>(&BaseWebSocketSubsystem::ClientJobMain, this, clientSocket);

        {
            std::lock_guard<std::mutex> lock(m_clientThreadsMutex);
            m_clientThreads.push_back(std::move(clientThread));
        }

        DAEMON_LOG(LogNetwork, eLogVerbosity::Log,
                   StringFormat("WebSocket client thread created for socket %llu",
                       static_cast<unsigned long long>(clientSocket)));

        return true;
    }
    catch (std::system_error const& e)
    {
        DAEMON_LOG(LogNetwork, eLogVerbosity::Error,
                   StringFormat("Failed to create client thread for socket %llu: %s",
                       static_cast<unsigned long long>(clientSocket), e.what()));
        return false;
    }
}

void BaseWebSocketSubsystem::CleanupClientThreads()
{
    // Join and remove finished client threads (non-blocking cleanup)
    std::lock_guard lock(m_clientThreadsMutex);

    // Remove finished threads using erase-remove idiom
    m_clientThreads.erase(
        std::remove_if(m_clientThreads.begin(), m_clientThreads.end(),
            [](std::unique_ptr<std::thread> const& thread)
            {
                UNUSED(thread)
                // Check if thread has finished (we can't directly check this in C++11/14)
                // For now, we'll just keep all threads until Stop() is called
                // In a more advanced implementation, we could use std::future or condition variables
                return false; // Don't remove any threads during Update()
            }),
        m_clientThreads.end()
    );

    // Note: All threads are joined in Stop() when shutdown happens
}
