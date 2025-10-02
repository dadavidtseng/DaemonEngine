//----------------------------------------------------------------------------------------------------
// BaseWebSocketSubsystem.hpp
// Abstract base class for WebSocket-based protocol subsystems
//----------------------------------------------------------------------------------------------------

#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <queue>
#include <mutex>
#include <atomic>

#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "ThirdParty/json/json.hpp"

// Windows socket types (forward declarations to avoid header conflicts)
using SOCKET      = uintptr_t;
using SOCKADDR_IN = struct sockaddr_in;

//----------------------------------------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------------------------------------
class Job;
class JobSystem;

//----------------------------------------------------------------------------------------------------
// WebSocket frame types (RFC 6455)
//----------------------------------------------------------------------------------------------------
enum class eWebSocketOpcode : uint8_t
{
    CONTINUATION = 0x0,
    TEXT_FRAME = 0x1,
    BINARY_FRAME = 0x2,
    CLOSE = 0x8,
    PING = 0x9,
    PONG = 0xA
};

//----------------------------------------------------------------------------------------------------
// WebSocket connection state
//----------------------------------------------------------------------------------------------------
struct sWebSocketConnection
{
    SOCKET socket     = 0;
    bool   isUpgraded = false;
    String receivedData;
    bool   isActive = false;
};

//----------------------------------------------------------------------------------------------------
// WebSocket configuration (base for all protocol-specific configs)
//----------------------------------------------------------------------------------------------------
struct sBaseWebSocketConfig
{
    bool   enabled = true;
    String host = "127.0.0.1";
    int    port = 9229;
    int    maxConnections = 10;
    bool   enableLogging = true;

    // Validation
    bool IsValid() const;

    // JSON Parsing
    static sBaseWebSocketConfig FromJSON(nlohmann::json const& j);
};

//----------------------------------------------------------------------------------------------------
// Base WebSocket Subsystem
// Provides generic WebSocket protocol implementation for specialized protocol subsystems
// Uses Template Method pattern: base class controls lifecycle, derived classes implement protocol handlers
//----------------------------------------------------------------------------------------------------
class BaseWebSocketSubsystem
{
public:
    explicit BaseWebSocketSubsystem(sBaseWebSocketConfig config);
    virtual  ~BaseWebSocketSubsystem();

    // Prevent copying and assignment
    BaseWebSocketSubsystem(BaseWebSocketSubsystem const&)            = delete;
    BaseWebSocketSubsystem& operator=(BaseWebSocketSubsystem const&) = delete;
    BaseWebSocketSubsystem(BaseWebSocketSubsystem&&)                 = delete;
    BaseWebSocketSubsystem& operator=(BaseWebSocketSubsystem&&)      = delete;

    //----------------------------------------------------------------------------------------------------
    // Lifecycle (Template Method pattern - final implementations)
    //----------------------------------------------------------------------------------------------------
    bool Start();
    void Update();
    void Stop();

    // Status
    bool IsRunning() const;
    bool HasActiveConnections() const;
    int  GetPort() const;

    //----------------------------------------------------------------------------------------------------
    // Public Job Entry Points (called by Job::Execute() in JobSystem worker threads)
    // These must be public so Job classes can access them
    //----------------------------------------------------------------------------------------------------

    /// @brief Server job main loop - accepts incoming connections
    /// @remark Runs in JobSystem worker thread, loops until m_shouldStop is set
    void ServerJobMain();

    /// @brief Client handler job main loop - processes WebSocket communication for one client
    /// @param clientSocket The client socket to handle
    /// @remark Runs in JobSystem worker thread, loops until client disconnects or shutdown
    void ClientJobMain(SOCKET clientSocket);

protected:
    //----------------------------------------------------------------------------------------------------
    // Pure Virtual Methods - Protocol-Specific Handlers (must be implemented by derived classes)
    //----------------------------------------------------------------------------------------------------

    /// @brief Called when a client connects (after socket acceptance, before WebSocket upgrade)
    /// @param clientSocket The socket handle for the connected client
    virtual void OnClientConnected(SOCKET clientSocket) = 0;

    /// @brief Called when a client disconnects or connection is lost
    /// @param clientSocket The socket handle for the disconnected client
    virtual void OnClientDisconnected(SOCKET clientSocket) = 0;

    /// @brief Called when a WebSocket message is received from a client
    /// @param clientSocket The socket handle for the client that sent the message
    /// @param message The decoded WebSocket message content
    virtual void OnClientMessage(SOCKET clientSocket, String const& message) = 0;

    /// @brief Generate HTTP discovery endpoint response (e.g., /json/list for Chrome DevTools)
    /// @return JSON string containing protocol-specific discovery information
    virtual String HandleDiscoveryRequest() = 0;

    /// @brief Process queued messages on main thread (for thread-safe protocol operations)
    /// @remark Called automatically by Update(), derived classes implement protocol-specific processing
    virtual void ProcessQueuedMessages() = 0;

    //----------------------------------------------------------------------------------------------------
    // Virtual Methods with Default Implementations (can be overridden)
    //----------------------------------------------------------------------------------------------------

    /// @brief Called after successful WebSocket upgrade handshake
    /// @param clientSocket The socket handle for the upgraded connection
    /// @remark Default implementation does nothing, override for protocol-specific initialization
    virtual void OnWebSocketUpgraded(SOCKET clientSocket) { UNUSED(clientSocket); }

    //----------------------------------------------------------------------------------------------------
    // Protected Helper Methods (for derived class use)
    //----------------------------------------------------------------------------------------------------

    /// @brief Send a WebSocket message to a specific client
    /// @param clientSocket Target client socket
    /// @param message Message content to send
    /// @return true if send succeeded, false otherwise
    bool SendToClient(SOCKET clientSocket, String const& message);

    /// @brief Broadcast a WebSocket message to all connected clients
    /// @param message Message content to broadcast
    void BroadcastToAllClients(String const& message);

    /// @brief Queue a message for main-thread processing (thread-safe)
    /// @param sourceSocket Socket that sent the message
    /// @param message Message content
    void QueueIncomingMessage(SOCKET sourceSocket, String const& message);

    /// @brief Check if a client socket is currently connected
    /// @param clientSocket Socket to check
    /// @return true if connected, false otherwise
    bool IsClientConnected(SOCKET clientSocket) const;

    /// @brief Get list of all active connection sockets
    /// @return Vector of active SOCKET handles
    std::vector<SOCKET> GetActiveConnections() const;

    /// @brief Generate a UUID string (accessible to derived classes)
    /// @return UUID in standard format (e.g., "550e8400-e29b-41d4-a716-446655440000")
    String GenerateUUID();

private:
    //----------------------------------------------------------------------------------------------------
    // WebSocket Protocol Implementation (RFC 6455) - Final implementations
    //----------------------------------------------------------------------------------------------------

    /// @brief Create WebSocket accept key for handshake response
    /// @param clientKey The Sec-WebSocket-Key from client request
    /// @return Base64-encoded SHA1 hash for Sec-WebSocket-Accept header
    String CreateWebSocketAcceptKey(String const& clientKey);

    /// @brief Encode data into WebSocket frame format
    /// @param payload Data to encode
    /// @param opcode Frame opcode (TEXT_FRAME, BINARY_FRAME, etc.)
    /// @return Encoded WebSocket frame
    String EncodeWebSocketFrame(String const& payload, eWebSocketOpcode opcode = eWebSocketOpcode::TEXT_FRAME);

    /// @brief Decode WebSocket frame and extract payload
    /// @param frame Raw WebSocket frame data
    /// @return Decoded payload data
    String DecodeWebSocketFrame(String const& frame);

    /// @brief Process HTTP to WebSocket upgrade request
    /// @param clientSocket Client requesting upgrade
    /// @param request HTTP upgrade request
    /// @return true if upgrade succeeded, false otherwise
    bool ProcessWebSocketUpgrade(SOCKET clientSocket, String const& request);

    //----------------------------------------------------------------------------------------------------
    // HTTP Server (for discovery endpoints)
    //----------------------------------------------------------------------------------------------------

    /// @brief Process HTTP request (non-WebSocket)
    /// @param clientSocket Client socket
    /// @param request HTTP request
    void ProcessHttpRequest(SOCKET clientSocket, String const& request);

    /// @brief Create HTTP response
    /// @param content Response body
    /// @param contentType Content-Type header value
    /// @return Complete HTTP response string
    String CreateHttpResponse(String const& content, String const& contentType = "application/json");

    //----------------------------------------------------------------------------------------------------
    // Socket Utilities
    //----------------------------------------------------------------------------------------------------

    /// @brief Send raw data to socket
    /// @param socket Target socket
    /// @param data Data to send
    /// @return true if send succeeded
    bool SendRawDataToSocket(SOCKET socket, String const& data);

    /// @brief Receive data from socket
    /// @param socket Source socket
    /// @return Received data (empty if connection closed or error)
    String ReceiveDataFromSocket(SOCKET socket);

    /// @brief Close a socket
    /// @param socket Socket to close
    void CloseSocket(SOCKET socket);

    //----------------------------------------------------------------------------------------------------
    // Utility Functions
    //----------------------------------------------------------------------------------------------------

    /// @brief Encode string to Base64
    /// @param input String to encode
    /// @return Base64-encoded string
    String Base64Encode(String const& input);

    //----------------------------------------------------------------------------------------------------
    // JobSystem Integration
    //----------------------------------------------------------------------------------------------------

    /// @brief Submit server job to JobSystem
    /// @return true if job submitted successfully
    bool SubmitServerJob();

    /// @brief Submit client handler job to JobSystem
    /// @param clientSocket Client socket to handle
    /// @return true if job submitted successfully
    bool SubmitClientJob(SOCKET clientSocket);

    /// @brief Retrieve and cleanup completed jobs from JobSystem
    void RetrieveCompletedJobs();

    //----------------------------------------------------------------------------------------------------
    // Member Variables
    //----------------------------------------------------------------------------------------------------

    sBaseWebSocketConfig m_config;
    SOCKET           m_serverSocket;

    std::atomic<bool> m_isRunning{false};
    std::atomic<bool> m_shouldStop{false};

    // JobSystem integration
    Job*              m_serverJob = nullptr;
    std::vector<Job*> m_clientJobs;
    std::mutex        m_clientJobsMutex;

    // Connection management
    std::unordered_map<SOCKET, sWebSocketConnection> m_connections;
    std::vector<SOCKET>                              m_activeConnections;
    mutable std::mutex                               m_connectionsMutex;

    // Message queue (for main-thread processing)
    struct QueuedMessage
    {
        SOCKET sourceSocket;
        String message;
    };

    std::queue<QueuedMessage> m_incomingMessageQueue;
    std::mutex                m_messageQueueMutex;
};
