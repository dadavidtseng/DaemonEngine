//----------------------------------------------------------------------------------------------------
// NetworkSubsystem.hpp
//----------------------------------------------------------------------------------------------------

#pragma once

#include <deque>
#include <string>
#include <vector>

#include "Engine/Core/Clock.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Network/NetworkCommon.hpp"

//----------------------------------------------------------------------------------------------------
struct sNetworkSubsystemConfig
{
    String modeString          = "None";            // "None", "Client", "Server"
    String hostAddressString   = "127.0.0.1";       // IP:Port format
    int    sendBufferSize      = 2048;
    int    recvBufferSize      = 2048;
    int    maxClients          = 4;                 // Server only: maximum number of clients
    bool   enableHeartbeat     = true;              // Enable heartbeat system
    float  heartbeatInterval   = 2.f;               // Heartbeat interval in seconds
    bool   enableConsoleOutput = true;              // Enable debug output to console
};

//----------------------------------------------------------------------------------------------------
class NetworkSubsystem
{
public:
    explicit NetworkSubsystem(sNetworkSubsystemConfig config);
    ~NetworkSubsystem();

    // Core lifecycle
    void StartUp();
    void BeginFrame();
    void Update();
    void EndFrame();
    void ShutDown();

    // Status queries
    bool             IsEnabled() const;
    bool             IsServer() const;
    bool             IsClient() const;
    bool             IsConnected() const;
    eNetworkMode     GetNetworkMode() const;
    eConnectionState GetConnectionState() const;

    // Server specific
    bool             StartServer(int port = -1);
    void             StopServer();
    int              GetConnectedClientCount() const;
    std::vector<int> GetConnectedClientIds() const;
    bool             SendMessageToClient(int clientId, sNetworkMessage const& message);
    bool             SendMessageToAllClients(sNetworkMessage const& message);

    // Client specific
    bool ConnectToServer(String const& address, int port);
    void DisconnectFromServer();
    bool SendMessageToServer(sNetworkMessage const& message);

    // General messaging
    void SendRawData(String const& data);
    void SendGameData(String const& gameData, int targetClientId = -1);
    void SendChatMessage(String const& message, int targetClientId = -1);

    // Event-based message retrieval
    bool            HasPendingMessages() const;
    sNetworkMessage GetNextMessage();
    void            ClearMessageQueue();

protected:
    // Core networking functions
    void InitializeWinsock();
    void CleanupWinsock();
    void CreateClientSocket();
    void CreateServerSocket();
    bool SetSocketNonBlocking(uintptr_t socket) const;

    // Message processing
    bool ProcessServerMessages();       // Server: handle multiple clients
    bool ProcessClientMessages();       // Client: handle server communication
    void ProcessIncomingConnections();  // Server: accept new connections
    void CheckClientConnections();      // Server: maintain client connections

    // Send/Receive primitives
    bool   SendRawDataToSocket(uintptr_t socket, String const& data);
    String ReceiveRawDataFromSocket(uintptr_t socket);

    // Message handling
    void ExecuteReceivedMessage(String const& message, int fromClientId = -1);
    void QueueIncomingMessage(sNetworkMessage const& message);

    // Connection management
    bool DealWithSocketError(uintptr_t socket, int clientId = -1);
    void CloseClientConnection(int clientId);
    void CloseAllConnections();

    // Heartbeat system
    void ProcessHeartbeat(float deltaSeconds);
    void SendHeartbeat();
    void ProcessHeartbeatMessage(int fromClientId);

    // Utility functions
    std::string     SerializeMessage(sNetworkMessage const& message);
    sNetworkMessage DeserializeMessage(String const& data, int fromClientId = -1);
    void            ParseHostAddress(String const& hostString, std::string& out_ip, unsigned short& out_port);
    void            LogMessage(String const& message);
    void            LogError(String const& error);

    sNetworkSubsystemConfig m_config;
    eNetworkMode            m_mode                     = eNetworkMode::NONE;
    eConnectionState        m_connectionState          = eConnectionState::DISCONNECTED;
    eConnectionState        m_lastFrameConnectionState = eConnectionState::DISCONNECTED;

    // Socket handles (using uintptr_t to avoid Windows headers in .hpp)
    uintptr_t m_clientSocket = ~0ull;
    uintptr_t m_listenSocket = ~0ull;

    // Connection info
    unsigned long  m_hostAddress = 0;
    unsigned short m_hostPort    = 0;

    // Buffers
    char* m_sendBuffer = nullptr;
    char* m_recvBuffer = nullptr;

    // Message queues
    std::deque<String>          m_sendQueue;
    std::string                 m_recvQueue;
    std::deque<sNetworkMessage> m_incomingMessages;

    // Server mode: client management
    std::vector<sClientConnection> m_clients;
    int                            m_nextClientId = 1;

    // Heartbeat system
    float m_heartbeatTimer        = 0.f;
    float m_lastHeartbeatReceived = 0.f;

    // Winsock initialization state
    bool m_winsockInitialized = false;

    // Statistics for debugging
    int m_messagesSent        = 0;
    int m_messagesReceived    = 0;
    int m_connectionsAccepted = 0;
    int m_connectionsLost     = 0;

    Clock* m_networkClock = nullptr;
};
