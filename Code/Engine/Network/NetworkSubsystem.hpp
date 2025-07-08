//----------------------------------------------------------------------------------------------------
// NetworkSubsystem.hpp
//----------------------------------------------------------------------------------------------------

#pragma once

#include "Engine/Core/EngineCommon.hpp"

#include <string>
#include <deque>
#include <vector>

//----------------------------------------------------------------------------------------------------
// Forward declarations
class NamedStrings;

//----------------------------------------------------------------------------------------------------
struct NetworkSubsystemConfig
{
    std::string modeString          = "None";                    // "None", "Client", "Server"
    std::string hostAddressString   = "127.0.0.1:3100";  // IP:Port format
    int         sendBufferSize      = 2048;
    int         recvBufferSize      = 2048;
    int         maxClients          = 4;                                 // Server only: maximum number of clients
    bool        enableHeartbeat     = true;                        // Enable heartbeat system
    float       heartbeatInterval   = 2.0f;                     // Heartbeat interval in seconds
    bool        enableConsoleOutput = true;                    // Enable debug output to console
};

//----------------------------------------------------------------------------------------------------
enum class NetworkMode
{
    NONE = 0,
    CLIENT,
    SERVER
};

//----------------------------------------------------------------------------------------------------
enum class ConnectionState
{
    DISCONNECTED = 0,
    CONNECTING,
    CONNECTED,
    DISCONNECTING,
    ERROR_STATE,
    DISABLED
};

//----------------------------------------------------------------------------------------------------
// Client connection info for server mode
struct ClientConnection
{
    uintptr_t       socket;
    int             clientId;
    ConnectionState state;
    std::string     address;
    unsigned short  port;
    float           lastHeartbeatTime;
    std::string     recvQueue;

    ClientConnection()
        : socket((uintptr_t)~0ull),
          clientId(-1),
          state(ConnectionState::DISCONNECTED),
          port(0),
          lastHeartbeatTime(0.0f),
          recvQueue("")
    {
    }
};

//----------------------------------------------------------------------------------------------------
// Network message structure
struct NetworkMessage
{
    std::string messageType;
    std::string data;
    int         fromClientId;  // -1 for server messages, client ID for client messages

    NetworkMessage()
        : fromClientId(-1)
    {
    }

    NetworkMessage(const std::string& type, const std::string& msgData, int clientId = -1)
        : messageType(type), data(msgData), fromClientId(clientId)
    {
    }
};

//----------------------------------------------------------------------------------------------------
class NetworkSubsystem
{
public:
    explicit NetworkSubsystem(NetworkSubsystemConfig const& config);
    ~NetworkSubsystem();

    // Core lifecycle
    void StartUp();
    void BeginFrame();
    void EndFrame();
    void ShutDown();
    void Update(float deltaSeconds);

    // Status queries
    bool            IsEnabled() const;
    bool            IsServer() const;
    bool            IsClient() const;
    bool            IsConnected() const;
    NetworkMode     GetNetworkMode() const;
    ConnectionState GetConnectionState() const;

    // Server specific
    bool             StartServer(int port = -1);
    void             StopServer();
    int              GetConnectedClientCount() const;
    std::vector<int> GetConnectedClientIds() const;
    bool             SendMessageToClient(int clientId, const NetworkMessage& message);
    bool             SendMessageToAllClients(const NetworkMessage& message);

    // Client specific
    bool ConnectToServer(const std::string& address, int port);
    void DisconnectFromServer();
    bool SendMessageToServer(const NetworkMessage& message);

    // General messaging
    void SendRawData(const std::string& data);
    void SendGameData(const std::string& gameData, int targetClientId = -1);
    void SendChatMessage(const std::string& message, int targetClientId = -1);

    // Event-based message retrieval
    bool           HasPendingMessages() const;
    NetworkMessage GetNextMessage();
    void           ClearMessageQueue();

protected:
    // Core networking functions
    void InitializeWinsock();
    void CleanupWinsock();
    void CreateClientSocket();
    void CreateServerSocket();
    bool SetSocketNonBlocking(uintptr_t socket);

    // Message processing
    bool ProcessMessages();
    bool ProcessServerMessages();  // Server: handle multiple clients
    bool ProcessClientMessages();  // Client: handle server communication
    void ProcessIncomingConnections();  // Server: accept new connections
    void CheckClientConnections();      // Server: maintain client connections

    // Send/Receive primitives
    bool        SendRawDataToSocket(uintptr_t socket, const std::string& data);
    std::string ReceiveRawDataFromSocket(uintptr_t socket);

    // Message handling
    void ExecuteReceivedMessage(const std::string& message, int fromClientId = -1);
    void QueueIncomingMessage(const NetworkMessage& message);

    // Connection management
    bool DealWithSocketError(uintptr_t socket, int clientId = -1);
    void CloseClientConnection(int clientId);
    void CloseAllConnections();

    // Heartbeat system
    void ProcessHeartbeat(float deltaSeconds);
    void SendHeartbeat();
    void ProcessHeartbeatMessage(int fromClientId);

    // Utility functions
    std::string    SerializeMessage(const NetworkMessage& message);
    NetworkMessage DeserializeMessage(const std::string& data, int fromClientId = -1);
    void           ParseHostAddress(const std::string& hostString, std::string& outIP, unsigned short& outPort);
    void           LogMessage(const std::string& message);
    void           LogError(const std::string& error);

protected:
    NetworkSubsystemConfig m_config;
    NetworkMode            m_mode;
    ConnectionState        m_connectionState;
    ConnectionState        m_lastFrameConnectionState;

    // Socket handles (using uintptr_t to avoid Windows headers in .hpp)
    uintptr_t m_clientSocket;
    uintptr_t m_listenSocket;

    // Connection info
    unsigned long  m_hostAddress;
    unsigned short m_hostPort;

    // Buffers
    char* m_sendBuffer;
    char* m_recvBuffer;

    // Message queues
    std::deque<std::string>    m_sendQueue;
    std::string                m_recvQueue;
    std::deque<NetworkMessage> m_incomingMessages;

    // Server mode: client management
    std::vector<ClientConnection> m_clients;
    int                           m_nextClientId;

    // Heartbeat system
    float m_heartbeatTimer;
    float m_lastHeartbeatReceived;

    // Winsock initialization state
    bool m_winsockInitialized;

    // Statistics for debugging
    int m_messagesSent;
    int m_messagesReceived;
    int m_connectionsAccepted;
    int m_connectionsLost;
};
