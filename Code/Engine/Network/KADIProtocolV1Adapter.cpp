//----------------------------------------------------------------------------------------------------
// KADIProtocolV1Adapter.cpp
// JSON-RPC 2.0 implementation of KADI protocol adapter
//----------------------------------------------------------------------------------------------------

#include "Engine/Network/KADIProtocolV1Adapter.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"

//----------------------------------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------------------------------
KADIProtocolV1Adapter::KADIProtocolV1Adapter()
{
	m_nextId = 1;
	m_agentId = "";
}

//----------------------------------------------------------------------------------------------------
// Serialization Methods
//----------------------------------------------------------------------------------------------------

std::string KADIProtocolV1Adapter::SerializeHello()
{
	nlohmann::json params = {
		{"role", "agent"}
	};

	nlohmann::json request = CreateRequest("kadi.session.hello", params);
	return request.dump();
}

std::string KADIProtocolV1Adapter::SerializeAuthenticate(std::string const& publicKey,
                                                          std::string const& signature)
{
	nlohmann::json params = {
		{"publicKey", publicKey},
		{"signature", signature}
	};

	nlohmann::json request = CreateRequest("kadi.session.authenticate", params);
	return request.dump();
}

std::string KADIProtocolV1Adapter::SerializeToolRegistration(nlohmann::json const& tools)
{
	nlohmann::json params = {
		{"agentId", m_agentId},
		{"tools", tools},
		{"networks", nlohmann::json::array({"global"})}
	};

	nlohmann::json request = CreateRequest("kadi.capabilities.register", params);
	return request.dump();
}

std::string KADIProtocolV1Adapter::SerializeToolResult(int requestId, nlohmann::json const& result)
{
	nlohmann::json params = {
		{"requestId", requestId},
		{"result", result}
	};

	nlohmann::json request = CreateRequest("kadi.ability.result", params);
	return request.dump();
}

std::string KADIProtocolV1Adapter::SerializeToolError(int requestId, std::string const& errorMessage)
{
	nlohmann::json params = {
		{"requestId", requestId},
		{"error", errorMessage}
	};

	nlohmann::json request = CreateRequest("kadi.ability.error", params);
	return request.dump();
}

std::string KADIProtocolV1Adapter::SerializeEventPublish(std::string const& channel,
                                                          nlohmann::json const& data)
{
	nlohmann::json params = {
		{"channel", channel},
		{"data", data}
	};

	nlohmann::json request = CreateRequest("kadi.event.publish", params);
	return request.dump();
}

std::string KADIProtocolV1Adapter::SerializeEventSubscribe(std::vector<std::string> const& channels)
{
	nlohmann::json channelsArray = nlohmann::json::array();
	for (std::string const& channel : channels)
	{
		channelsArray.push_back(channel);
	}

	nlohmann::json params = {
		{"channels", channelsArray},
		{"networkId", "global"}
	};

	nlohmann::json request = CreateRequest("kadi.event.subscribe", params);
	return request.dump();
}

std::string KADIProtocolV1Adapter::SerializePing()
{
	nlohmann::json request = CreateRequest("kadi.session.ping", nlohmann::json::object());
	return request.dump();
}

//----------------------------------------------------------------------------------------------------
// Deserialization Methods
//----------------------------------------------------------------------------------------------------

bool KADIProtocolV1Adapter::ParseMessage(std::string const& message, sKADIMessage& out)
{
	try
	{
		// Parse JSON
		nlohmann::json j = nlohmann::json::parse(message);

		// Validate JSON-RPC 2.0 structure
		if (!j.contains("jsonrpc") || j["jsonrpc"] != "2.0")
		{
			DebuggerPrintf("KADIProtocolV1: Invalid JSON-RPC version or missing jsonrpc field\n");
			return false;
		}

		// Extract message ID (optional for notifications)
		out.id = j.value("id", -1);

		// Determine message type based on structure
		if (j.contains("method"))
		{
			// Request or Notification
			std::string method = j["method"];
			out.type = ParseMethodToType(method);
			out.payload = j.value("params", nlohmann::json::object());

			return true;
		}
		else if (j.contains("result"))
		{
			// Response (success)
			out.payload = j["result"];

			// Phase 4: Detect PONG responses (result is string "pong")
			if (out.payload.is_string() && out.payload.get<std::string>() == "pong")
			{
				out.type = eKADIMessageType::PONG;
				return true;
			}

			// Determine response type based on ID
			// ID 1 = hello response, ID 2 = authenticate response
			// This is a simplified approach; production code should track pending requests
			if (out.id == 1)
			{
				out.type = eKADIMessageType::HELLO_RESPONSE;
			}
			else if (out.id == 2)
			{
				out.type = eKADIMessageType::AUTHENTICATE_RESPONSE;
			}
			else
			{
				// Unknown response ID - default to generic response
				DebuggerPrintf("KADIProtocolV1: Unknown response ID %d, defaulting to HELLO_RESPONSE\n", out.id);
				out.type = eKADIMessageType::HELLO_RESPONSE;
			}

			return true;
		}
		else if (j.contains("error"))
		{
			// Error Response
			out.type = eKADIMessageType::ERROR_RESPONSE;
			out.payload = j["error"];

			return true;
		}

		// Unknown message structure
		DebuggerPrintf("KADIProtocolV1: Unknown message structure\n");
		return false;
	}
	catch (nlohmann::json::parse_error const& e)
	{
		DebuggerPrintf("KADIProtocolV1: JSON parse error: %s\n", e.what());
		return false;
	}
	catch (std::exception const& e)
	{
		DebuggerPrintf("KADIProtocolV1: Exception during message parsing: %s\n", e.what());
		return false;
	}
}

//----------------------------------------------------------------------------------------------------
// Helper Methods
//----------------------------------------------------------------------------------------------------

nlohmann::json KADIProtocolV1Adapter::CreateRequest(std::string const& method,
                                                      nlohmann::json const& params)
{
	nlohmann::json request = {
		{"jsonrpc", "2.0"},
		{"method", method},
		{"params", params},
		{"id", m_nextId++}
	};

	return request;
}

nlohmann::json KADIProtocolV1Adapter::CreateResponse(int id, nlohmann::json const& result)
{
	nlohmann::json response = {
		{"jsonrpc", "2.0"},
		{"result", result},
		{"id", id}
	};

	return response;
}

nlohmann::json KADIProtocolV1Adapter::CreateError(int id, int errorCode,
                                                    std::string const& errorMessage)
{
	nlohmann::json error = {
		{"jsonrpc", "2.0"},
		{"error", {
			{"code", errorCode},
			{"message", errorMessage}
		}},
		{"id", id}
	};

	return error;
}

eKADIMessageType KADIProtocolV1Adapter::ParseMethodToType(std::string const& method)
{
	// Session Management
	if (method == "kadi.session.hello") return eKADIMessageType::HELLO_REQUEST;
	if (method == "kadi.session.authenticate") return eKADIMessageType::AUTHENTICATE_REQUEST;
	if (method == "kadi.session.ping") return eKADIMessageType::PING;
	if (method == "kadi.session.pong") return eKADIMessageType::PONG;

	// Tool Management
	if (method == "kadi.capabilities.register") return eKADIMessageType::REGISTER_TOOLS;
	if (method == "kadi.ability.invoke") return eKADIMessageType::TOOL_INVOKE;
	if (method == "kadi.ability.result") return eKADIMessageType::TOOL_RESULT;
	if (method == "kadi.ability.error") return eKADIMessageType::TOOL_ERROR;
	if (method == "kadi.ability.cancel") return eKADIMessageType::TOOL_CANCEL;

	// Event System
	if (method == "kadi.event.publish") return eKADIMessageType::EVENT_PUBLISH;
	if (method == "kadi.event.subscribe") return eKADIMessageType::EVENT_SUBSCRIBE;
	if (method == "kadi.event.unsubscribe") return eKADIMessageType::EVENT_UNSUBSCRIBE;
	if (method == "kadi.event.delivery") return eKADIMessageType::EVENT_DELIVERY;

	// Unknown
	DebuggerPrintf("KADIProtocolV1: Unknown method '%s'\n", method.c_str());
	return eKADIMessageType::UNKNOWN;
}
