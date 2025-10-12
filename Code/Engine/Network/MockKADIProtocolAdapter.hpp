//----------------------------------------------------------------------------------------------------
// MockKADIProtocolAdapter.hpp
// Mock protocol adapter for unit testing (simplified protocol implementation)
// Provides test-friendly interface with message tracking and injectable responses
//----------------------------------------------------------------------------------------------------

#pragma once

#include "Engine/Network/IKADIProtocolAdapter.hpp"

#include <vector>

//----------------------------------------------------------------------------------------------------
// Mock KADI Protocol Adapter (for Unit Testing)
//----------------------------------------------------------------------------------------------------
class MockKADIProtocolAdapter : public IKADIProtocolAdapter
{
public:
	MockKADIProtocolAdapter();
	~MockKADIProtocolAdapter() override = default;

	//----------------------------------------------------------------------------------------------------
	// Serialization (Outgoing Messages) - Simplified for testing
	//----------------------------------------------------------------------------------------------------

	std::string SerializeHello() override;
	std::string SerializeAuthenticate(std::string const& publicKey,
	                                   std::string const& signature) override;
	std::string SerializeToolRegistration(nlohmann::json const& tools) override;
	std::string SerializeToolResult(int requestId, nlohmann::json const& result) override;
	std::string SerializeToolError(int requestId, std::string const& errorMessage) override;
	std::string SerializeEventPublish(std::string const& channel,
	                                   nlohmann::json const& data) override;
	std::string SerializeEventSubscribe(std::vector<std::string> const& channels) override;
	std::string SerializePing() override;

	//----------------------------------------------------------------------------------------------------
	// Deserialization (Incoming Messages) - Returns injectable test responses
	//----------------------------------------------------------------------------------------------------

	bool ParseMessage(std::string const& message, sKADIMessage& out) override;

	//----------------------------------------------------------------------------------------------------
	// Test Helpers (for unit test assertions)
	//----------------------------------------------------------------------------------------------------

	/// @brief Get list of all sent messages (for verification in tests)
	/// @return Vector of sent message strings
	std::vector<std::string> const& GetSentMessages() const { return m_sentMessages; }

	/// @brief Get list of all received messages (for verification in tests)
	/// @return Vector of received message strings
	std::vector<std::string> const& GetReceivedMessages() const { return m_receivedMessages; }

	/// @brief Clear all message history (reset state between tests)
	void ClearMessageHistory()
	{
		m_sentMessages.clear();
		m_receivedMessages.clear();
	}

	/// @brief Inject a mock tool invocation response
	/// @param toolName Name of tool to invoke
	/// @param arguments Tool arguments (JSON object)
	/// @param requestId Request ID for response tracking
	void InjectMockToolInvoke(std::string const& toolName,
	                          nlohmann::json const& arguments,
	                          int requestId)
	{
		m_mockToolInvokes.push_back({toolName, arguments, requestId});
	}

	/// @brief Inject a mock hello response
	/// @param nonce Nonce string to return
	void InjectMockHelloResponse(std::string const& nonce)
	{
		m_mockNonce = nonce;
		m_returnMockHelloResponse = true;
	}

	/// @brief Inject a mock authenticate response
	/// @param agentId Agent ID to assign
	void InjectMockAuthResponse(std::string const& agentId)
	{
		m_mockAgentId = agentId;
		m_returnMockAuthResponse = true;
	}

private:
	//----------------------------------------------------------------------------------------------------
	// Test State
	//----------------------------------------------------------------------------------------------------

	std::vector<std::string> m_sentMessages;      // Sent messages for verification
	std::vector<std::string> m_receivedMessages;  // Received messages for verification

	// Injectable mock responses
	struct MockToolInvoke
	{
		std::string    toolName;
		nlohmann::json arguments;
		int            requestId;
	};

	std::vector<MockToolInvoke> m_mockToolInvokes;
	size_t                      m_currentToolInvokeIndex = 0;

	std::string m_mockNonce;
	std::string m_mockAgentId;
	bool        m_returnMockHelloResponse = false;
	bool        m_returnMockAuthResponse = false;
};
