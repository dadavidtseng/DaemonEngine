//----------------------------------------------------------------------------------------------------
// KADIWebSocketSubsystem.cpp
// WebSocket client subsystem implementation for KADI broker connectivity
//----------------------------------------------------------------------------------------------------

#include "Engine/Network/KADIWebSocketSubsystem.hpp"
#include "Engine/Network/KADIProtocolV1Adapter.hpp"
#include "Engine/Network/KADIAuthenticationUtility.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include <sstream>
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

#include "Engine/Core/Time.hpp"

#pragma comment(lib, "ws2_32.lib")

#pragma warning(pop)

//----------------------------------------------------------------------------------------------------
// WebSocket Magic String for handshake (RFC 6455)
//----------------------------------------------------------------------------------------------------
static const std::string WEBSOCKET_MAGIC      = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
static constexpr SOCKET  INVALID_SOCKET_VALUE = INVALID_SOCKET;

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
// Constructor
//----------------------------------------------------------------------------------------------------
KADIWebSocketSubsystem::KADIWebSocketSubsystem()
    : m_protocolAdapter(std::make_unique<KADIProtocolV1Adapter>())
      , m_connectionState(eKADIConnectionState::DISCONNECTED)
      , m_threadRunning(false)
      , m_clientSocket(INVALID_SOCKET_VALUE)
      , m_isWebSocketUpgraded(false)
      , m_lastPingTime(0.0)
      , m_lastPongTime(0.0)
{
}

//----------------------------------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------------------------------
KADIWebSocketSubsystem::~KADIWebSocketSubsystem()
{
    Shutdown();
}

//----------------------------------------------------------------------------------------------------
// EngineSubsystem Interface Implementation
//----------------------------------------------------------------------------------------------------

void KADIWebSocketSubsystem::Startup()
{
    DebuggerPrintf("KADIWebSocketSubsystem: Startup\n");
}

void KADIWebSocketSubsystem::Shutdown()
{
    DebuggerPrintf("KADIWebSocketSubsystem: Shutdown\n");
    Disconnect();
}

void KADIWebSocketSubsystem::BeginFrame()
{
    // Debug: Log when BeginFrame is called during CONNECTING state
    static bool s_loggedConnectingState = false;
    if (m_connectionState == eKADIConnectionState::CONNECTING)
    {
        if (!s_loggedConnectingState)
        {
            DebuggerPrintf("KADIWebSocketSubsystem: BeginFrame called while CONNECTING (m_isWebSocketUpgraded = %s, m_threadRunning = %s)\n",
                           m_isWebSocketUpgraded ? "true" : "false",
                           m_threadRunning ? "true" : "false");
            s_loggedConnectingState = true;
        }
    }
    else
    {
        s_loggedConnectingState = false; // Reset when not connecting
    }

    // Check if WebSocket handshake completed and transition to CONNECTED
    if (m_connectionState == eKADIConnectionState::CONNECTING && m_isWebSocketUpgraded)
    {
        DebuggerPrintf("KADIWebSocketSubsystem: BeginFrame detected handshake completion, transitioning to CONNECTED\n");
        SetConnectionState(eKADIConnectionState::CONNECTED);
        InitiateHelloSequence();
    }

    // Process incoming messages from WebSocket thread
    ProcessIncomingMessages();

    // Phase 4: Update heartbeat system
    UpdateHeartbeat();
}

void KADIWebSocketSubsystem::EndFrame()
{
    // No per-frame cleanup needed for Phase 1
}

//----------------------------------------------------------------------------------------------------
// Connection Management
//----------------------------------------------------------------------------------------------------

void KADIWebSocketSubsystem::Connect(std::string const& brokerUrl,
                                     std::string const& publicKey,
                                     std::string const& privateKey)
{
    if (m_connectionState != eKADIConnectionState::DISCONNECTED)
    {
        DebuggerPrintf("KADIWebSocketSubsystem: Already connected or connecting\n");
        return;
    }

    DebuggerPrintf("KADIWebSocketSubsystem: Connecting to %s\n", brokerUrl.c_str());

    // Store connection parameters
    m_brokerUrl  = brokerUrl;
    m_publicKey  = publicKey;
    m_privateKey = privateKey;

	// Transition to CONNECTING state
	SetConnectionState(eKADIConnectionState::CONNECTING);

	// Phase 2-4: Start WebSocket thread (real WebSocket client)
	// The thread will handle connection, handshake, and signal when ready
	m_threadRunning   = true;
	m_webSocketThread = std::make_unique<std::thread>(&KADIWebSocketSubsystem::WebSocketThreadMain, this);

	// Note: Connection state will transition to CONNECTED after WebSocket handshake completes
	// The hello sequence will be initiated after successful WebSocket upgrade
}
void KADIWebSocketSubsystem::Disconnect()
{
    if (m_connectionState == eKADIConnectionState::DISCONNECTED)
    {
        return;
    }

    DebuggerPrintf("KADIWebSocketSubsystem: Disconnecting\n");

    // Phase 3: Send CLOSE frame before disconnecting (RFC 6455)
    if (m_clientSocket != INVALID_SOCKET_VALUE && m_isWebSocketUpgraded)
    {
        // Send CLOSE frame (opcode 0x8, no payload)
        std::string closeFrame = EncodeWebSocketFrame("", eWebSocketOpcode::CLOSE);
        SendRawDataToSocket(m_clientSocket, closeFrame);
        DebuggerPrintf("KADIWebSocketSubsystem: Sent CLOSE frame\n");

        // Wait briefly for server to acknowledge close
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // Stop WebSocket thread
    m_threadRunning = false;
    if (m_webSocketThread && m_webSocketThread->joinable())
    {
        m_webSocketThread->join();
    }

    // Close WebSocket client socket (Phase 2)
    if (m_clientSocket != INVALID_SOCKET_VALUE)
    {
        CloseSocket(m_clientSocket);
        m_clientSocket        = INVALID_SOCKET_VALUE;
        m_isWebSocketUpgraded = false;
        WSACleanup();
    }

    // Clear queues
    {
        std::lock_guard<std::mutex> lock(m_outgoingMutex);
        while (!m_outgoingMessages.empty())
        {
            m_outgoingMessages.pop();
        }
    }

    {
        std::lock_guard<std::mutex> lock(m_incomingMutex);
        while (!m_incomingMessages.empty())
        {
            m_incomingMessages.pop();
        }
    }

    // Transition to DISCONNECTED
    SetConnectionState(eKADIConnectionState::DISCONNECTED);
}

bool KADIWebSocketSubsystem::IsConnected() const
{
    return m_connectionState >= eKADIConnectionState::CONNECTED;
}

//----------------------------------------------------------------------------------------------------
// Connection State Management
//----------------------------------------------------------------------------------------------------

void KADIWebSocketSubsystem::SetConnectionState(eKADIConnectionState newState)
{
    if (m_connectionState == newState)
    {
        return;
    }

    eKADIConnectionState oldState = m_connectionState;
    m_connectionState             = newState;

    DebuggerPrintf("KADIWebSocketSubsystem: State transition %d -> %d\n", (int)oldState, (int)newState);

    // Phase 2: Invoke callback immediately for main thread safety
    // State changes from Connect()/Disconnect() are always on main thread (V8-safe)
    // State changes from background thread would queue, but we don't call SetConnectionState from bg thread
    if (m_connectionStateCallback)
    {
        m_connectionStateCallback(oldState, newState);
    }

    // Handle state-specific transitions
    HandleConnectionStateTransition(oldState, newState);
}

void KADIWebSocketSubsystem::HandleConnectionStateTransition(eKADIConnectionState oldState,
                                                             eKADIConnectionState newState)
{
    UNUSED(oldState);

    switch (newState)
    {
    case eKADIConnectionState::AUTHENTICATED:
        // Automatically register tools after authentication
        DebuggerPrintf("KADIWebSocketSubsystem: AUTHENTICATED state reached, checking pending tools...\n");
        DebuggerPrintf("  m_pendingToolRegistration.is_array() = %s\n", m_pendingToolRegistration.is_array() ? "true" : "false");
        DebuggerPrintf("  m_pendingToolRegistration.empty() = %s\n", m_pendingToolRegistration.empty() ? "true" : "false");
        DebuggerPrintf("  m_pendingToolRegistration dump: %s\n", m_pendingToolRegistration.dump().c_str());

        if (m_pendingToolRegistration.is_array() && !m_pendingToolRegistration.empty())
        {
            DebuggerPrintf("KADIWebSocketSubsystem: Automatically registering %d pending tools\n", (int)m_pendingToolRegistration.size());
            SetConnectionState(eKADIConnectionState::REGISTERING_TOOLS);
            RegisterTools(m_pendingToolRegistration);
        }
        else
        {
            // No tools to register, go directly to READY
            DebuggerPrintf("KADIWebSocketSubsystem: No pending tools to register, transitioning directly to READY\n");
            SetConnectionState(eKADIConnectionState::READY);
        }
        break;

    case eKADIConnectionState::READY:
        DebuggerPrintf("KADIWebSocketSubsystem: Connection fully established and ready\n");
        break;

    default:
        break;
    }
}

//----------------------------------------------------------------------------------------------------
// Protocol Flow
//----------------------------------------------------------------------------------------------------

void KADIWebSocketSubsystem::InitiateHelloSequence()
{
    DebuggerPrintf("KADIWebSocketSubsystem: Sending hello\n");

    std::string helloMessage = m_protocolAdapter->SerializeHello();
    QueueMessage(helloMessage);

    SetConnectionState(eKADIConnectionState::AUTHENTICATING);
}

void KADIWebSocketSubsystem::InitiateAuthentication(std::string const& nonce)
{
    DebuggerPrintf("KADIWebSocketSubsystem: Authenticating with nonce: %s\n", nonce.c_str());

    // Phase 3: Real Ed25519 authentication
    // Decode base64 private key to binary
    std::vector<unsigned char> privateKeyBinary = KADIAuthenticationUtility::Base64Decode(m_privateKey);

    // Sign nonce with Ed25519
    std::vector<unsigned char> signatureBinary;
    bool                       success = KADIAuthenticationUtility::SignNonce(nonce, privateKeyBinary, signatureBinary);

    if (!success)
    {
        DebuggerPrintf("KADIWebSocketSubsystem: Failed to sign nonce\n");
        return;
    }

    // Encode signature to base64
    std::string signature = KADIAuthenticationUtility::Base64Encode(signatureBinary);

    std::string authMessage = m_protocolAdapter->SerializeAuthenticate(m_publicKey, signature);
    QueueMessage(authMessage);
}

void KADIWebSocketSubsystem::CompleteAuthentication(std::string const& agentId)
{
    DebuggerPrintf("KADIWebSocketSubsystem: Authentication complete, agentId: %s\n", agentId.c_str());

    m_agentId = agentId;
    m_protocolAdapter->SetAgentId(agentId);

    SetConnectionState(eKADIConnectionState::AUTHENTICATED);
}

//----------------------------------------------------------------------------------------------------
// Tool Management
//----------------------------------------------------------------------------------------------------

void KADIWebSocketSubsystem::RegisterTools(nlohmann::json const& tools)
{
    if (m_connectionState < eKADIConnectionState::AUTHENTICATED)
    {
        // Store for later registration after authentication
        m_pendingToolRegistration = tools;
        DebuggerPrintf("KADIWebSocketSubsystem: Tools queued for registration after authentication\n");
        return;
    }

    DebuggerPrintf("KADIWebSocketSubsystem: Registering %d tools\n", (int)tools.size());

    std::string registerMessage = m_protocolAdapter->SerializeToolRegistration(tools);
    QueueMessage(registerMessage);

    // After registration, transition to READY
    SetConnectionState(eKADIConnectionState::READY);
}

void KADIWebSocketSubsystem::SendToolResult(int requestId, nlohmann::json const& result)
{
    if (m_connectionState != eKADIConnectionState::READY)
    {
        DebuggerPrintf("KADIWebSocketSubsystem: Cannot send tool result, not ready\n");
        return;
    }

    DebuggerPrintf("KADIWebSocketSubsystem: Sending tool result for requestId %d\n", requestId);

    std::string resultMessage = m_protocolAdapter->SerializeToolResult(requestId, result);
    QueueMessage(resultMessage);
}

void KADIWebSocketSubsystem::SendToolError(int requestId, std::string const& errorMessage)
{
    if (m_connectionState != eKADIConnectionState::READY)
    {
        DebuggerPrintf("KADIWebSocketSubsystem: Cannot send tool error, not ready\n");
        return;
    }

    DebuggerPrintf("KADIWebSocketSubsystem: Sending tool error for requestId %d: %s\n", requestId, errorMessage.c_str());

    std::string errorMsg = m_protocolAdapter->SerializeToolError(requestId, errorMessage);
    QueueMessage(errorMsg);
}

//----------------------------------------------------------------------------------------------------
// Event System
//----------------------------------------------------------------------------------------------------

void KADIWebSocketSubsystem::SubscribeToEvents(std::vector<std::string> const& channels)
{
    if (m_connectionState < eKADIConnectionState::AUTHENTICATED)
    {
        DebuggerPrintf("KADIWebSocketSubsystem: Cannot subscribe to events, not authenticated\n");
        return;
    }

    DebuggerPrintf("KADIWebSocketSubsystem: Subscribing to %d event channels\n", (int)channels.size());

    std::string subscribeMessage = m_protocolAdapter->SerializeEventSubscribe(channels);
    QueueMessage(subscribeMessage);
}

void KADIWebSocketSubsystem::PublishEvent(std::string const& channel, nlohmann::json const& data)
{
    if (m_connectionState != eKADIConnectionState::READY)
    {
        DebuggerPrintf("KADIWebSocketSubsystem: Cannot publish event, not ready\n");
        return;
    }

    DebuggerPrintf("KADIWebSocketSubsystem: Publishing event to channel: %s\n", channel.c_str());

    std::string eventMessage = m_protocolAdapter->SerializeEventPublish(channel, data);
    QueueMessage(eventMessage);
}

//----------------------------------------------------------------------------------------------------
// Message Sending
//----------------------------------------------------------------------------------------------------

void KADIWebSocketSubsystem::QueueMessage(std::string const& message)
{
    sKADIOutgoingMessage outgoing;
    outgoing.payload   = message;
    outgoing.messageId = static_cast<int>(m_outgoingMessages.size());

    {
        std::lock_guard<std::mutex> lock(m_outgoingMutex);
        m_outgoingMessages.push(outgoing);
    }

    DebuggerPrintf("KADIWebSocketSubsystem: Queued outgoing message: %s\n", message.c_str());
}

//----------------------------------------------------------------------------------------------------
// Message Processing
//----------------------------------------------------------------------------------------------------

void KADIWebSocketSubsystem::ProcessIncomingMessages()
{
    std::lock_guard<std::mutex> lock(m_incomingMutex);

    size_t messageCount = m_incomingMessages.size();
    if (messageCount > 0)
    {
        DebuggerPrintf("KADIWebSocketSubsystem: ProcessIncomingMessages - %zu messages in queue\n", messageCount);
    }

    while (!m_incomingMessages.empty())
    {
        std::string message = m_incomingMessages.front();
        m_incomingMessages.pop();

        DebuggerPrintf("KADIWebSocketSubsystem: Processing message: %s\n", message.c_str());

        // Parse message using protocol adapter
        sKADIMessage parsedMessage;
        if (m_protocolAdapter->ParseMessage(message, parsedMessage))
        {
            DebuggerPrintf("  Message parsed successfully\n");
            HandleIncomingMessage(parsedMessage);
        }
        else
        {
            DebuggerPrintf("KADIWebSocketSubsystem: Failed to parse incoming message: %s\n", message.c_str());
        }
    }
}

void KADIWebSocketSubsystem::HandleIncomingMessage(sKADIMessage const& message)
{
    DebuggerPrintf("KADIWebSocketSubsystem: HandleIncomingMessage - message type = %d\n", (int)message.type);

    switch (message.type)
    {
    case eKADIMessageType::HELLO_RESPONSE:
        DebuggerPrintf("  Routing to HandleHelloResponse\n");
        HandleHelloResponse(message);
        break;

    case eKADIMessageType::AUTHENTICATE_RESPONSE:
        DebuggerPrintf("  Routing to HandleAuthenticateResponse\n");
        HandleAuthenticateResponse(message);
        break;

    case eKADIMessageType::TOOL_INVOKE:
        DebuggerPrintf("  Routing to HandleToolInvoke\n");
        HandleToolInvoke(message);
        break;

    case eKADIMessageType::EVENT_DELIVERY:
        DebuggerPrintf("  Routing to HandleEventDelivery\n");
        HandleEventDelivery(message);
        break;

    case eKADIMessageType::PONG:
        DebuggerPrintf("  Routing to HandlePongResponse\n");
        HandlePongResponse(message);
        break;

    case eKADIMessageType::ERROR_RESPONSE:
        DebuggerPrintf("  Routing to HandleErrorResponse\n");
        HandleErrorResponse(message);
        break;

    default:
        DebuggerPrintf("KADIWebSocketSubsystem: Unhandled message type: %d\n", (int)message.type);
        break;
    }
}

void KADIWebSocketSubsystem::HandleHelloResponse(sKADIMessage const& message)
{
    DebuggerPrintf("KADIWebSocketSubsystem: Received hello response\n");

    // Extract nonce from response
    if (message.payload.contains("nonce"))
    {
        std::string nonce = message.payload["nonce"];
        m_pendingNonce    = nonce;

        // Initiate authentication
        InitiateAuthentication(nonce);
    }
    else
    {
        DebuggerPrintf("KADIWebSocketSubsystem: Hello response missing nonce\n");
    }
}

void KADIWebSocketSubsystem::HandleAuthenticateResponse(sKADIMessage const& message)
{
    DebuggerPrintf("KADIWebSocketSubsystem: *** HandleAuthenticateResponse CALLED ***\n");
    DebuggerPrintf("KADIWebSocketSubsystem: Received authenticate response\n");
    DebuggerPrintf("  Message payload: %s\n", message.payload.dump().c_str());

    // Extract agentId from response
    if (message.payload.contains("agentId"))
    {
        std::string agentId = message.payload["agentId"];
        DebuggerPrintf("  Extracted agentId: %s\n", agentId.c_str());
        CompleteAuthentication(agentId);
    }
    else
    {
        DebuggerPrintf("KADIWebSocketSubsystem: Authenticate response missing agentId\n");
        DebuggerPrintf("  Available keys in payload:\n");
        for (auto it = message.payload.begin(); it != message.payload.end(); ++it)
        {
            DebuggerPrintf("    - %s\n", it.key().c_str());
        }
    }
}

void KADIWebSocketSubsystem::HandleToolInvoke(sKADIMessage const& message)
{
    DebuggerPrintf("KADIWebSocketSubsystem: Received tool invocation\n");

    if (m_toolInvokeCallback)
    {
        // Extract requestId - could be string or int depending on broker implementation
        int requestId = -1;
        if (message.payload.contains("requestId"))
        {
            if (message.payload["requestId"].is_number())
            {
                requestId = message.payload["requestId"].get<int>();
            }
            else if (message.payload["requestId"].is_string())
            {
                // Hash string requestId to int for callback compatibility
                // Note: In production, you'd want a better mapping or change callback signature
                std::string requestIdStr = message.payload["requestId"].get<std::string>();
                requestId = static_cast<int>(std::hash<std::string>{}(requestIdStr));
                DebuggerPrintf("  Converted string requestId '%s' to int %d\n", requestIdStr.c_str(), requestId);
            }
        }

        std::string    toolName  = message.payload.value("toolName", "");
        nlohmann::json arguments = message.payload.value("arguments", nlohmann::json::object());

        DebuggerPrintf("  Tool: %s, RequestId: %d\n", toolName.c_str(), requestId);

        m_toolInvokeCallback(requestId, toolName, arguments);
    }
    else
    {
        DebuggerPrintf("KADIWebSocketSubsystem: Tool invoke received but no callback registered\n");
    }
}

void KADIWebSocketSubsystem::HandleEventDelivery(sKADIMessage const& message)
{
    DebuggerPrintf("KADIWebSocketSubsystem: Received event delivery\n");

    if (m_eventDeliveryCallback)
    {
        std::string    channel = message.payload.value("channel", "");
        nlohmann::json data    = message.payload.value("data", nlohmann::json::object());

        m_eventDeliveryCallback(channel, data);
    }
    else
    {
        DebuggerPrintf("KADIWebSocketSubsystem: Event delivery received but no callback registered\n");
    }
}

void KADIWebSocketSubsystem::HandleErrorResponse(sKADIMessage const& message)
{
    DebuggerPrintf("KADIWebSocketSubsystem: Received error response: %s\n", message.payload.dump().c_str());
}

void KADIWebSocketSubsystem::HandlePongResponse(sKADIMessage const& message)
{
    UNUSED(message);

    // Update last pong time to current time
    // Note: We need a way to get current time. For now, we'll use a simple approach
    m_lastPongTime = GetCurrentTimeSeconds();

    DebuggerPrintf("KADIWebSocketSubsystem: Received PONG response\n");
}

//----------------------------------------------------------------------------------------------------
// Heartbeat System (Phase 4)
//----------------------------------------------------------------------------------------------------

void KADIWebSocketSubsystem::SendPing()
{
    if (m_connectionState < eKADIConnectionState::AUTHENTICATED)
    {
        return; // Only send pings when authenticated
    }

    DebuggerPrintf("KADIWebSocketSubsystem: Sending heartbeat PING\n");

    std::string pingMessage = m_protocolAdapter->SerializePing();
    QueueMessage(pingMessage);

    m_lastPingTime = GetCurrentTimeSeconds();
}

void KADIWebSocketSubsystem::UpdateHeartbeat()
{
    if (m_connectionState < eKADIConnectionState::AUTHENTICATED)
    {
        return; // Only monitor heartbeat when authenticated
    }

    double currentTime = GetCurrentTimeSeconds();

    // Send periodic pings
    if (currentTime - m_lastPingTime >= HEARTBEAT_INTERVAL)
    {
        SendPing();
    }

    // Check for connection timeout
    if (m_lastPongTime > 0.0 && currentTime - m_lastPongTime >= HEARTBEAT_TIMEOUT)
    {
        HandleConnectionTimeout();
    }
}

void KADIWebSocketSubsystem::HandleConnectionTimeout()
{
    DebuggerPrintf("KADIWebSocketSubsystem: Connection timeout detected (no PONG received)\n");

    // Disconnect and trigger reconnection logic (for future phases)
    Disconnect();

    // In Phase 4, we just disconnect. Future phases could add automatic reconnection.
}

//----------------------------------------------------------------------------------------------------
// WebSocket Thread Management (Phase 2: Real WebSocket Client)
//----------------------------------------------------------------------------------------------------

void KADIWebSocketSubsystem::WebSocketThreadMain()
{
    DebuggerPrintf("KADIWebSocketSubsystem: WebSocket thread started\n");

    // Parse broker URL (format: "ws://host:port" or "ws://host:port/path")
    std::string host = "localhost";
    int         port = 8080;
    std::string path = "/";

    // Simple URL parsing (ws://host:port/path)
    if (m_brokerUrl.find("ws://") == 0)
    {
        std::string urlWithoutProtocol = m_brokerUrl.substr(5); // Skip "ws://"

        size_t colonPos = urlWithoutProtocol.find(':');
        size_t slashPos = urlWithoutProtocol.find('/');

        if (colonPos != std::string::npos)
        {
            host = urlWithoutProtocol.substr(0, colonPos);

            std::string portAndPath;
            if (slashPos != std::string::npos)
            {
                portAndPath = urlWithoutProtocol.substr(colonPos + 1, slashPos - colonPos - 1);
                path        = urlWithoutProtocol.substr(slashPos);
            }
            else
            {
                portAndPath = urlWithoutProtocol.substr(colonPos + 1);
            }

            port = std::stoi(portAndPath);
        }
        else if (slashPos != std::string::npos)
        {
            host = urlWithoutProtocol.substr(0, slashPos);
            path = urlWithoutProtocol.substr(slashPos);
        }
        else
        {
            host = urlWithoutProtocol;
        }
    }

    DebuggerPrintf("KADIWebSocketSubsystem: Parsed URL - host: %s, port: %d, path: %s\n",
                   host.c_str(), port, path.c_str());

    // Phase 2: Real WebSocket client connection
    if (!ConnectToServer(host, port))
    {
        DebuggerPrintf("KADIWebSocketSubsystem: Failed to connect to server\n");
        m_threadRunning = false;
        return;
    }

    // Send WebSocket upgrade handshake
    if (!SendClientHandshake(host, path))
    {
        DebuggerPrintf("KADIWebSocketSubsystem: Failed to send WebSocket handshake\n");
        CloseSocket(m_clientSocket);
        m_clientSocket  = INVALID_SOCKET_VALUE;
        m_threadRunning = false;
        return;
    }

    // Wait for and validate handshake response
    std::string handshakeResponse;
    while (m_threadRunning)
    {
        std::string data = ReceiveDataFromSocket(m_clientSocket);
        if (!data.empty())
        {
            handshakeResponse += data;

            // Check for complete HTTP response (double CRLF)
            if (handshakeResponse.find("\r\n\r\n") != std::string::npos)
            {
                break;
            }
        }

        // Small sleep to prevent busy-waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    if (!ValidateServerHandshake(handshakeResponse))
    {
        DebuggerPrintf("KADIWebSocketSubsystem: WebSocket handshake validation failed\n");
        CloseSocket(m_clientSocket);
        m_clientSocket  = INVALID_SOCKET_VALUE;
        m_threadRunning = false;
        return;
    }

    DebuggerPrintf("KADIWebSocketSubsystem: WebSocket connection established\n");

    // Main WebSocket communication loop
    std::string receivedData;
    while (m_threadRunning)
    {
        // Send outgoing messages
        {
            std::lock_guard<std::mutex> lock(m_outgoingMutex);
            while (!m_outgoingMessages.empty())
            {
                sKADIOutgoingMessage message = m_outgoingMessages.front();
                m_outgoingMessages.pop();

                // Encode message as WebSocket frame and send
                std::string wsFrame = EncodeWebSocketFrame(message.payload);
                if (!SendRawDataToSocket(m_clientSocket, wsFrame))
                {
                    DebuggerPrintf("KADIWebSocketSubsystem: Failed to send WebSocket message\n");
                    m_threadRunning = false;
                    break;
                }

                DebuggerPrintf("KADIWebSocketSubsystem: Sent: %s\n", message.payload.c_str());
            }
        }

        // Receive incoming messages
        std::string data = ReceiveDataFromSocket(m_clientSocket);
        if (!data.empty())
        {
            receivedData += data;

            // Try to decode WebSocket frames
            while (!receivedData.empty())
            {
                std::string decodedMessage = DecodeWebSocketFrame(receivedData);
                if (decodedMessage.empty())
                {
                    // Not enough data for complete frame yet
                    break;
                }

                // Queue message for main thread processing
                ReceiveMessageInternal(decodedMessage);

                // Remove processed frame from buffer
                // Note: This is simplified - a production implementation would track frame boundaries
                receivedData.clear();
            }
        }

        // Small sleep to prevent busy-waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Cleanup
    CloseSocket(m_clientSocket);
    m_clientSocket        = INVALID_SOCKET_VALUE;
    m_isWebSocketUpgraded = false;

    DebuggerPrintf("KADIWebSocketSubsystem: WebSocket thread stopped\n");
}

void KADIWebSocketSubsystem::SendMessageInternal(std::string const& message)
{
    // Phase 2: This method is no longer needed as messages are sent directly in WebSocketThreadMain
    // Keeping for backward compatibility
    UNUSED(message);
    DebuggerPrintf("KADIWebSocketSubsystem: SendMessageInternal called (deprecated in Phase 2)\n");
}

void KADIWebSocketSubsystem::ReceiveMessageInternal(std::string const& message)
{
    std::lock_guard<std::mutex> lock(m_incomingMutex);
    m_incomingMessages.push(message);

    DebuggerPrintf("KADIWebSocketSubsystem: Received: %s\n", message.c_str());
}

//====================================================================================================
// Phase 2: WebSocket Client Protocol Implementation
//====================================================================================================

//----------------------------------------------------------------------------------------------------
// Base64 Encoding (copied from BaseWebSocketSubsystem)
//----------------------------------------------------------------------------------------------------
std::string KADIWebSocketSubsystem::Base64Encode(std::string const& input)
{
    static const std::string chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::string result;
    int         val  = 0;
    int         valb = -6;

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
// Generate random client key for WebSocket handshake
//----------------------------------------------------------------------------------------------------
std::string KADIWebSocketSubsystem::GenerateClientKey()
{
    // Generate 16 random bytes and Base64 encode them
    std::string randomBytes;
    for (int i = 0; i < 16; ++i)
    {
        randomBytes += static_cast<char>(rand() % 256);
    }
    return Base64Encode(randomBytes);
}

//----------------------------------------------------------------------------------------------------
// Create WebSocket accept key for validation (copied from BaseWebSocketSubsystem)
//----------------------------------------------------------------------------------------------------
std::string KADIWebSocketSubsystem::CreateWebSocketAcceptKey(std::string const& clientKey)
{
    std::string combined = clientKey + WEBSOCKET_MAGIC;
    std::string hash     = SimpleSHA1::Hash(combined);
    return Base64Encode(hash);
}

//----------------------------------------------------------------------------------------------------
// Encode WebSocket frame (copied from BaseWebSocketSubsystem with client masking)
//----------------------------------------------------------------------------------------------------
std::string KADIWebSocketSubsystem::EncodeWebSocketFrame(std::string const& payload, eWebSocketOpcode opcode)
{
    std::string frame;

    auto pushByte = [&frame](uint8_t byte)
    {
        frame.push_back(static_cast<char>(byte));
    };

    // First byte: FIN=1, RSV=000, Opcode
    pushByte(0x80 | static_cast<uint8_t>(opcode));

    // Second byte: MASK=1 (client must mask), Payload length
    size_t payloadLength = payload.length();
    if (payloadLength < 126)
    {
        pushByte(0x80 | static_cast<uint8_t>(payloadLength));
    }
    else if (payloadLength <= 0xFFFF)
    {
        pushByte(0x80 | 126);
        pushByte(static_cast<uint8_t>((payloadLength >> 8) & 0xFF));
        pushByte(static_cast<uint8_t>(payloadLength & 0xFF));
    }
    else
    {
        pushByte(0x80 | 127);
        for (int i = 7; i >= 0; --i)
        {
            pushByte(static_cast<uint8_t>((payloadLength >> (i * 8)) & 0xFF));
        }
    }

    // Masking key (4 random bytes)
    uint8_t maskingKey[4];
    for (int i = 0; i < 4; ++i)
    {
        maskingKey[i] = static_cast<uint8_t>(rand() % 256);
        pushByte(maskingKey[i]);
    }

    // Masked payload
    for (size_t i = 0; i < payload.length(); ++i)
    {
        uint8_t maskedByte = static_cast<uint8_t>(payload[i]) ^ maskingKey[i % 4];
        frame.push_back(static_cast<char>(maskedByte));
    }

    return frame;
}

//----------------------------------------------------------------------------------------------------
// Decode WebSocket frame (copied from BaseWebSocketSubsystem)
//----------------------------------------------------------------------------------------------------
std::string KADIWebSocketSubsystem::DecodeWebSocketFrame(std::string const& frame)
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

    std::string payload = frame.substr(headerLength, payloadLength);

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

//----------------------------------------------------------------------------------------------------
// Socket Utilities
//----------------------------------------------------------------------------------------------------

bool KADIWebSocketSubsystem::SendRawDataToSocket(SOCKET socket, std::string const& data)
{
    if (socket == INVALID_SOCKET_VALUE || data.empty()) return false;

    const char* buffer    = data.c_str();
    int         totalSent = 0;
    int         dataSize  = static_cast<int>(data.length());

    while (totalSent < dataSize)
    {
        int sent = send(socket, buffer + totalSent, dataSize - totalSent, 0);
        if (sent == SOCKET_ERROR)
        {
            int error = WSAGetLastError();
            DebuggerPrintf("KADIWebSocketSubsystem: Send failed: %d\n", error);
            return false;
        }
        totalSent += sent;
    }

    return true;
}

std::string KADIWebSocketSubsystem::ReceiveDataFromSocket(SOCKET socket)
{
    if (socket == INVALID_SOCKET_VALUE) return "";

    char buffer[4096];
    int  received = recv(socket, buffer, sizeof(buffer) - 1, 0);

    if (received > 0)
    {
        buffer[received] = '\0';
        return std::string(buffer, received);
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
            DebuggerPrintf("KADIWebSocketSubsystem: Receive failed: %d\n", error);
        }
        return "";
    }
}

void KADIWebSocketSubsystem::CloseSocket(SOCKET socket)
{
    if (socket != INVALID_SOCKET_VALUE)
    {
        closesocket(socket);
    }
}

//----------------------------------------------------------------------------------------------------
// WebSocket Client Connection Functions
//----------------------------------------------------------------------------------------------------

bool KADIWebSocketSubsystem::ConnectToServer(std::string const& host, int port)
{
    // Initialize Winsock
    WSADATA wsaData;
    int     result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0)
    {
        DebuggerPrintf("KADIWebSocketSubsystem: WSAStartup failed: %d\n", result);
        return false;
    }

    // Create socket
    m_clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_clientSocket == INVALID_SOCKET)
    {
        DebuggerPrintf("KADIWebSocketSubsystem: Failed to create socket: %d\n", WSAGetLastError());
        WSACleanup();
        return false;
    }

    // Resolve server address using getaddrinfo (supports both IP addresses and hostnames)
    addrinfo hints    = {};
    hints.ai_family   = AF_INET;        // IPv4
    hints.ai_socktype = SOCK_STREAM;  // TCP
    hints.ai_protocol = IPPROTO_TCP;

    std::string portStr     = std::to_string(port);
    addrinfo*   result_info = nullptr;

    int gaResult = getaddrinfo(host.c_str(), portStr.c_str(), &hints, &result_info);
    if (gaResult != 0)
    {
        DebuggerPrintf("KADIWebSocketSubsystem: Failed to resolve hostname %s: %d\n", host.c_str(), gaResult);
        CloseSocket(m_clientSocket);
        m_clientSocket = INVALID_SOCKET_VALUE;
        WSACleanup();
        return false;
    }

    // Connect to server using resolved address
    bool connected = false;
    for (addrinfo* ptr = result_info; ptr != nullptr; ptr = ptr->ai_next)
    {
        if (connect(m_clientSocket, ptr->ai_addr, static_cast<int>(ptr->ai_addrlen)) == 0)
        {
            connected = true;
            break;
        }
    }

    freeaddrinfo(result_info);

    if (!connected)
    {
        DebuggerPrintf("KADIWebSocketSubsystem: Failed to connect to %s:%d, error: %d\n",
                       host.c_str(), port, WSAGetLastError());
        CloseSocket(m_clientSocket);
        m_clientSocket = INVALID_SOCKET_VALUE;
        WSACleanup();
        return false;
    }

    // Set socket to non-blocking mode
    u_long mode = 1; // 1 = non-blocking, 0 = blocking
    if (ioctlsocket(m_clientSocket, FIONBIO, &mode) != 0)
    {
        DebuggerPrintf("KADIWebSocketSubsystem: Failed to set non-blocking mode: %d\n", WSAGetLastError());
        CloseSocket(m_clientSocket);
        m_clientSocket = INVALID_SOCKET_VALUE;
        WSACleanup();
        return false;
    }

    DebuggerPrintf("KADIWebSocketSubsystem: TCP connection established to %s:%d\n", host.c_str(), port);
    return true;
}

bool KADIWebSocketSubsystem::SendClientHandshake(std::string const& host, std::string const& path)
{
    std::string clientKey = GenerateClientKey();

    // Build WebSocket upgrade request
    std::ostringstream request;
    request << "GET " << path << " HTTP/1.1\r\n";
    request << "Host: " << host << "\r\n";
    request << "Upgrade: websocket\r\n";
    request << "Connection: Upgrade\r\n";
    request << "Sec-WebSocket-Key: " << clientKey << "\r\n";
    request << "Sec-WebSocket-Version: 13\r\n";
    request << "\r\n";

    if (!SendRawDataToSocket(m_clientSocket, request.str()))
    {
        DebuggerPrintf("KADIWebSocketSubsystem: Failed to send WebSocket upgrade request\n");
        return false;
    }

    DebuggerPrintf("KADIWebSocketSubsystem: Sent WebSocket upgrade request\n");

    // Store client key for validation
    // TODO: Store clientKey in member variable for later validation if needed

    return true;
}

bool KADIWebSocketSubsystem::ValidateServerHandshake(std::string const& response)
{
    // Look for "HTTP/1.1 101 Switching Protocols"
    if (response.find("101 Switching Protocols") == std::string::npos)
    {
        DebuggerPrintf("KADIWebSocketSubsystem: Server did not accept WebSocket upgrade\n");
        return false;
    }

    // TODO: Validate Sec-WebSocket-Accept header matches expected value
    // For Phase 2, we'll accept any 101 response

    DebuggerPrintf("KADIWebSocketSubsystem: WebSocket upgrade successful\n");
    m_isWebSocketUpgraded = true;
    DebuggerPrintf("KADIWebSocketSubsystem: m_isWebSocketUpgraded set to TRUE (atomic), waiting for BeginFrame() to detect...\n");
    return true;
}
