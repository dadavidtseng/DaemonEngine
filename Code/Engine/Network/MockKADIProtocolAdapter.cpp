//----------------------------------------------------------------------------------------------------
// MockKADIProtocolAdapter.cpp
// Mock protocol adapter implementation for unit testing
//----------------------------------------------------------------------------------------------------

#include "Engine/Network/MockKADIProtocolAdapter.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"

//----------------------------------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------------------------------
MockKADIProtocolAdapter::MockKADIProtocolAdapter()
{
	m_nextId = 1;
	m_agentId = "";
	m_mockNonce = "mock-nonce-12345";
	m_mockAgentId = "mock-agent-001";
}

//----------------------------------------------------------------------------------------------------
// Serialization Methods (Simplified - just track that they were called)
//----------------------------------------------------------------------------------------------------

std::string MockKADIProtocolAdapter::SerializeHello()
{
	std::string mockMessage = R"({"method":"kadi.session.hello"})";
	m_sentMessages.push_back("HELLO");
	return mockMessage;
}

std::string MockKADIProtocolAdapter::SerializeAuthenticate(std::string const& publicKey,
                                                            std::string const& signature,
                                                            std::string const& nonce,
                                                            bool wantNewId)
{
	UNUSED(publicKey);
	UNUSED(signature);
	UNUSED(nonce);
	UNUSED(wantNewId);

	std::string mockMessage = R"({"method":"kadi.session.authenticate"})";
	m_sentMessages.push_back("AUTHENTICATE");
	return mockMessage;
}

std::string MockKADIProtocolAdapter::SerializeToolRegistration(nlohmann::json const& tools)
{
	UNUSED(tools);

	std::string mockMessage = R"({"method":"kadi.capabilities.register"})";
	m_sentMessages.push_back("REGISTER_TOOLS");
	return mockMessage;
}

std::string MockKADIProtocolAdapter::SerializeToolResult(int requestId, nlohmann::json const& result)
{
	UNUSED(requestId);
	UNUSED(result);

	std::string mockMessage = R"({"method":"kadi.ability.result"})";
	m_sentMessages.push_back("TOOL_RESULT");
	return mockMessage;
}

std::string MockKADIProtocolAdapter::SerializeToolError(int requestId, std::string const& errorMessage)
{
	UNUSED(requestId);
	UNUSED(errorMessage);

	std::string mockMessage = R"({"method":"kadi.ability.error"})";
	m_sentMessages.push_back("TOOL_ERROR");
	return mockMessage;
}

std::string MockKADIProtocolAdapter::SerializeEventPublish(std::string const& channel,
                                                            nlohmann::json const& data)
{
	UNUSED(channel);
	UNUSED(data);

	std::string mockMessage = R"({"method":"kadi.event.publish"})";
	m_sentMessages.push_back("EVENT_PUBLISH");
	return mockMessage;
}

std::string MockKADIProtocolAdapter::SerializeEventSubscribe(std::vector<std::string> const& channels)
{
	UNUSED(channels);

	std::string mockMessage = R"({"method":"kadi.event.subscribe"})";
	m_sentMessages.push_back("EVENT_SUBSCRIBE");
	return mockMessage;
}

std::string MockKADIProtocolAdapter::SerializePing()
{
	std::string mockMessage = R"({"method":"kadi.session.ping"})";
	m_sentMessages.push_back("PING");
	return mockMessage;
}

//----------------------------------------------------------------------------------------------------
// Deserialization Methods (Returns injected mock responses)
//----------------------------------------------------------------------------------------------------

bool MockKADIProtocolAdapter::ParseMessage(std::string const& message, sKADIMessage& out)
{
	m_receivedMessages.push_back(message);

	// Return injected mock hello response
	if (m_returnMockHelloResponse)
	{
		out.type = eKADIMessageType::HELLO_RESPONSE;
		out.payload = {
			{"nonce", m_mockNonce},
			{"version", "1.0.0"}
		};
		out.id = 1;

		m_returnMockHelloResponse = false;  // One-time response
		return true;
	}

	// Return injected mock authenticate response
	if (m_returnMockAuthResponse)
	{
		out.type = eKADIMessageType::AUTHENTICATE_RESPONSE;
		out.payload = {
			{"agentId", m_mockAgentId}
		};
		out.id = 2;

		m_agentId = m_mockAgentId;  // Update internal state
		m_returnMockAuthResponse = false;  // One-time response
		return true;
	}

	// Return injected mock tool invocation
	if (m_currentToolInvokeIndex < m_mockToolInvokes.size())
	{
		MockToolInvoke const& invoke = m_mockToolInvokes[m_currentToolInvokeIndex];

		out.type = eKADIMessageType::TOOL_INVOKE;
		out.payload = {
			{"toolName", invoke.toolName},
			{"arguments", invoke.arguments},
			{"requestId", invoke.requestId}
		};
		out.id = 999;

		m_currentToolInvokeIndex++;
		return true;
	}

	// Default: unknown message
	DebuggerPrintf("MockKADIProtocolAdapter: No mock response configured, returning false\n");
	return false;
}
