//----------------------------------------------------------------------------------------------------
// KADIWebSocketSubsystem.hpp
// WebSocket client subsystem for KADI broker connectivity
//----------------------------------------------------------------------------------------------------

#pragma once

#include "Engine/Network/IKADIProtocolAdapter.hpp"
#include "ThirdParty/json/json.hpp"
#include <string>
#include <queue>
#include <mutex>
#include <thread>
#include <memory>
#include <functional>
#include <atomic>

// Windows socket types (forward declarations to avoid header conflicts)
using SOCKET = uintptr_t;

//----------------------------------------------------------------------------------------------------
// Forward Declarations
//----------------------------------------------------------------------------------------------------
class KADIProtocolV1Adapter;

//----------------------------------------------------------------------------------------------------
// WebSocket frame types (RFC 6455) - Client implementation
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
// Connection State Enum
//----------------------------------------------------------------------------------------------------
enum class eKADIConnectionState
{
    DISCONNECTED,      // Not connected to broker
    CONNECTING,        // Connection in progress
    CONNECTED,         // WebSocket connection established, not authenticated
    AUTHENTICATING,    // Sending hello and authentication
    AUTHENTICATED,     // Authentication complete
    REGISTERING_TOOLS, // Registering capabilities
    READY              // Fully connected and operational
};

//----------------------------------------------------------------------------------------------------
// Outgoing Message Structure
//----------------------------------------------------------------------------------------------------
struct sKADIOutgoingMessage
{
    std::string payload;
    int         messageId;
};

//----------------------------------------------------------------------------------------------------
// Callback Types
//----------------------------------------------------------------------------------------------------
using KADIToolInvokeCallback      = std::function<void(int requestId, std::string const& toolName, nlohmann::json const& arguments)>;
using KADIEventDeliveryCallback   = std::function<void(std::string const& channel, nlohmann::json const& data)>;
using KADIConnectionStateCallback = std::function<void(eKADIConnectionState oldState, eKADIConnectionState newState)>;

//----------------------------------------------------------------------------------------------------
// KADIWebSocketSubsystem
// Manages WebSocket connection to KADI broker with authentication and message routing
//----------------------------------------------------------------------------------------------------
class KADIWebSocketSubsystem
{
public:
    KADIWebSocketSubsystem();
    ~KADIWebSocketSubsystem();

    // Lifecycle Management
    void Startup();
    void Shutdown();
    void BeginFrame();
    void EndFrame();

    // Connection Management
    void                 Connect(std::string const& brokerUrl, std::string const& publicKey, std::string const& privateKey);
    void                 Disconnect();
    bool                 IsConnected() const;
    eKADIConnectionState GetConnectionState() const { return m_connectionState; }

    // Tool Management
    void RegisterTools(nlohmann::json const& tools);
    void SendToolResult(int requestId, nlohmann::json const& result);
    void SendToolError(int requestId, std::string const& errorMessage);

    // Event System
    void SubscribeToEvents(std::vector<std::string> const& channels);
    void PublishEvent(std::string const& channel, nlohmann::json const& data);

    // Callback Registration
    void SetToolInvokeCallback(KADIToolInvokeCallback callback) { m_toolInvokeCallback = callback; }
    void SetEventDeliveryCallback(KADIEventDeliveryCallback callback) { m_eventDeliveryCallback = callback; }
    void SetConnectionStateCallback(KADIConnectionStateCallback callback) { m_connectionStateCallback = callback; }

    // Message Sending
    void QueueMessage(std::string const& message);

private:
    // Connection State Management
    void SetConnectionState(eKADIConnectionState newState);
    void HandleConnectionStateTransition(eKADIConnectionState oldState, eKADIConnectionState newState);

    // Protocol Flow
    void InitiateHelloSequence();
    void InitiateAuthentication(std::string const& nonce);
    void CompleteAuthentication(std::string const& agentId);

    // Message Processing
    void ProcessIncomingMessages();
    void HandleIncomingMessage(sKADIMessage const& message);
    void HandleHelloResponse(sKADIMessage const& message);
    void HandleAuthenticateResponse(sKADIMessage const& message);
    void HandleToolInvoke(sKADIMessage const& message);
    void HandleEventDelivery(sKADIMessage const& message);
    void HandleErrorResponse(sKADIMessage const& message);
    void HandlePongResponse(sKADIMessage const& message);

    // Heartbeat System (Phase 4)
    void SendPing();
    void UpdateHeartbeat();
    void HandleConnectionTimeout();

    // WebSocket Thread Management (Phase 1: Stub for future implementation)
    void WebSocketThreadMain();
    void SendMessageInternal(std::string const& message);
    void ReceiveMessageInternal(std::string const& message);

    //----------------------------------------------------------------------------------------------------
    // WebSocket Client Protocol Functions (Phase 2: Real WebSocket)
    //----------------------------------------------------------------------------------------------------

    // Client Connection Functions
    bool ConnectToServer(std::string const& host, int port);
    bool SendClientHandshake(std::string const& host, std::string const& path);
    bool ValidateServerHandshake(std::string const& response);

    // WebSocket Protocol (RFC 6455) - Copied from BaseWebSocketSubsystem
    std::string EncodeWebSocketFrame(std::string const& payload, eWebSocketOpcode opcode = eWebSocketOpcode::TEXT_FRAME);
    std::string DecodeWebSocketFrame(std::string const& frame);
    std::string Base64Encode(std::string const& input);
    std::string GenerateClientKey();
    std::string CreateWebSocketAcceptKey(std::string const& clientKey);

    // Socket Utilities
    bool        SendRawDataToSocket(SOCKET socket, std::string const& data);
    std::string ReceiveDataFromSocket(SOCKET socket);
    void        CloseSocket(SOCKET socket);

    // Member Variables
    std::unique_ptr<IKADIProtocolAdapter> m_protocolAdapter;
    eKADIConnectionState                  m_connectionState;

    // Connection Configuration
    std::string m_brokerUrl;
    std::string m_publicKey;
    std::string m_privateKey;
    std::string m_agentId;
    std::string m_pendingNonce;

    // Message Queues (Thread-Safe)
    std::queue<sKADIOutgoingMessage> m_outgoingMessages;
    std::queue<std::string>          m_incomingMessages;
    std::mutex                       m_outgoingMutex;
    std::mutex                       m_incomingMutex;

    // WebSocket Thread
    std::unique_ptr<std::thread> m_webSocketThread;
    std::atomic<bool>            m_threadRunning;

    // WebSocket Client Socket (Phase 2)
    SOCKET            m_clientSocket;
    std::atomic<bool> m_isWebSocketUpgraded;

    // Callbacks
    KADIToolInvokeCallback      m_toolInvokeCallback;
    KADIEventDeliveryCallback   m_eventDeliveryCallback;
    KADIConnectionStateCallback m_connectionStateCallback;

    // Pending Tool Registration
    nlohmann::json m_pendingToolRegistration;

    // Heartbeat System (Phase 4)
    double m_lastPingTime;
    double m_lastPongTime;
    static constexpr double HEARTBEAT_INTERVAL = 30.0; // Send ping every 30 seconds
    static constexpr double HEARTBEAT_TIMEOUT  = 90.0; // Timeout after 90 seconds without pong
};
