//----------------------------------------------------------------------------------------------------
// IKADIProtocolAdapter.hpp
// Abstract protocol adapter interface for KADI broker communication
// Enables protocol version swapping without core architecture changes
//----------------------------------------------------------------------------------------------------

#pragma once

#include "Engine/Core/EngineCommon.hpp"
#include "ThirdParty/json/json.hpp"

#include <string>
#include <vector>

//----------------------------------------------------------------------------------------------------
// KADI Message Types (protocol-agnostic)
//----------------------------------------------------------------------------------------------------
enum class eKADIMessageType : uint8_t
{
    // Session Management
    HELLO_REQUEST,
    HELLO_RESPONSE,
    AUTHENTICATE_REQUEST,
    AUTHENTICATE_RESPONSE,
    PING,
    PONG,

    // Tool Management
    REGISTER_TOOLS,
    REGISTER_TOOLS_RESPONSE,
    TOOL_INVOKE,
    TOOL_RESULT,
    TOOL_ERROR,
    TOOL_CANCEL,

    // Event System
    EVENT_PUBLISH,
    EVENT_SUBSCRIBE,
    EVENT_UNSUBSCRIBE,
    EVENT_DELIVERY,

    // Error and Unknown
    ERROR_RESPONSE,
    UNKNOWN
};

//----------------------------------------------------------------------------------------------------
// Parsed KADI Message (protocol-agnostic representation)
//----------------------------------------------------------------------------------------------------
struct sKADIMessage
{
    eKADIMessageType type = eKADIMessageType::UNKNOWN;
    nlohmann::json   payload;  // Contains method-specific data
    int              id = -1;  // JSON-RPC message ID (-1 for notifications)

    // Convenience accessors
    bool IsRequest() const { return id >= 0 && type != eKADIMessageType::UNKNOWN; }

    bool IsResponse() const
    {
        return id >= 0 && (type == eKADIMessageType::HELLO_RESPONSE ||
            type == eKADIMessageType::AUTHENTICATE_RESPONSE ||
            type == eKADIMessageType::REGISTER_TOOLS_RESPONSE);
    }

    bool IsNotification() const { return id < 0; }
};

//----------------------------------------------------------------------------------------------------
// Abstract Protocol Adapter Interface
// Implementations: KADIProtocolV1Adapter (JSON-RPC 2.0), MockKADIProtocolAdapter (testing)
//----------------------------------------------------------------------------------------------------
class IKADIProtocolAdapter
{
public:
    virtual ~IKADIProtocolAdapter() = default;

    //----------------------------------------------------------------------------------------------------
    // Serialization (Outgoing Messages) - Convert structured data to protocol format
    //----------------------------------------------------------------------------------------------------

    /// @brief Serialize session hello message (initial handshake)
    /// @return Protocol-formatted hello message string
    virtual std::string SerializeHello() = 0;

    /// @brief Serialize authentication message with public key and signature
    /// @param publicKey Ed25519 public key (hex-encoded)
    /// @param signature Signed nonce (hex-encoded)
    /// @return Protocol-formatted authentication message
    virtual std::string SerializeAuthenticate(std::string const& publicKey,
                                              std::string const& signature) = 0;

    /// @brief Serialize tool registration message
    /// @param tools JSON array of tool definitions (name, description, inputSchema)
    /// @return Protocol-formatted tool registration message
    virtual std::string SerializeToolRegistration(nlohmann::json const& tools) = 0;

    /// @brief Serialize tool execution result
    /// @param requestId Original request ID from tool invocation
    /// @param result Tool execution result (JSON object)
    /// @return Protocol-formatted tool result message
    virtual std::string SerializeToolResult(int requestId, nlohmann::json const& result) = 0;

    /// @brief Serialize tool execution error
    /// @param requestId Original request ID from tool invocation
    /// @param errorMessage Error description
    /// @return Protocol-formatted tool error message
    virtual std::string SerializeToolError(int requestId, std::string const& errorMessage) = 0;

    /// @brief Serialize event publish message
    /// @param channel Event channel name (e.g., "game.player.moved")
    /// @param data Event payload (JSON object)
    /// @return Protocol-formatted event publish message
    virtual std::string SerializeEventPublish(std::string const&    channel,
                                              nlohmann::json const& data) = 0;

    /// @brief Serialize event subscription message
    /// @param channels Array of channel patterns to subscribe to (supports wildcards)
    /// @return Protocol-formatted event subscribe message
    virtual std::string SerializeEventSubscribe(std::vector<std::string> const& channels) = 0;

    /// @brief Serialize heartbeat ping message
    /// @return Protocol-formatted ping message
    virtual std::string SerializePing() = 0;

    //----------------------------------------------------------------------------------------------------
    // Deserialization (Incoming Messages) - Parse protocol format to structured data
    //----------------------------------------------------------------------------------------------------

    /// @brief Parse incoming message and extract type + payload
    /// @param message Raw protocol message string
    /// @param out Parsed message structure (output parameter)
    /// @return true if parsing succeeded, false otherwise
    virtual bool ParseMessage(std::string const& message, sKADIMessage& out) = 0;

    //----------------------------------------------------------------------------------------------------
    // Protocol State Management
    //----------------------------------------------------------------------------------------------------

    /// @brief Set agent ID (received after authentication)
    /// @param agentId Unique agent identifier assigned by broker
    virtual void SetAgentId(std::string const& agentId) { m_agentId = agentId; }

    /// @brief Get current agent ID
    /// @return Agent ID string (empty if not authenticated)
    virtual std::string GetAgentId() const { return m_agentId; }

protected:
    std::string m_agentId;  // Agent identifier (set after authentication)
    int         m_nextId = 1;  // Message ID counter (for request/response pairing)
};
