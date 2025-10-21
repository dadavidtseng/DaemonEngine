//----------------------------------------------------------------------------------------------------
// KADIProtocolV1Adapter.cpp
// JSON-RPC 2.0 implementation of KADI protocol adapter
//----------------------------------------------------------------------------------------------------

#include "Engine/Network/KADIProtocolV1Adapter.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"

#include <functional>  // For std::hash (string ID to int conversion)

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
                                                          std::string const& signature,
                                                          std::string const& nonce,
                                                          bool wantNewId)
{
	nlohmann::json params = {
		{"publicKey", publicKey},
		{"signature", signature},
		{"nonce", nonce},
		{"wantNewId", wantNewId}
	};

	nlohmann::json request = CreateRequest("kadi.session.authenticate", params);
	return request.dump();
}

std::string KADIProtocolV1Adapter::SerializeToolRegistration(nlohmann::json const& tools)
{
	nlohmann::json params = {
		{"tools", tools},
		{"networks", nlohmann::json::array({"global"})},
		{"displayName", "ProtogameJS3D Agent"}
	};

	nlohmann::json request = CreateRequest("kadi.agent.register", params);
	return request.dump();
}

std::string KADIProtocolV1Adapter::SerializeToolResult(int requestId, nlohmann::json const& result)
{
	// Check if we have the original string ID for this hashed int ID
	nlohmann::json responseIdJson;
	auto it = m_idMapping.find(requestId);
	if (it != m_idMapping.end())
	{
		responseIdJson = it->second;  // Use original string ID
		m_idMapping.erase(it);        // Clean up mapping after use
	}
	else
	{
		responseIdJson = requestId;   // Use integer ID as-is
	}

	// Send JSON-RPC RESPONSE, not a request
	// The broker sent us a request with an ID, we respond directly with that ID
	nlohmann::json response = {
		{"jsonrpc", "2.0"},
		{"id", responseIdJson},
		{"result", result}
	};

	return response.dump();
}

std::string KADIProtocolV1Adapter::SerializeToolError(int requestId, std::string const& errorMessage)
{
	// Check if we have the original string ID for this hashed int ID
	nlohmann::json responseIdJson;
	auto it = m_idMapping.find(requestId);
	if (it != m_idMapping.end())
	{
		responseIdJson = it->second;  // Use original string ID
		m_idMapping.erase(it);        // Clean up mapping after use
	}
	else
	{
		responseIdJson = requestId;   // Use integer ID as-is
	}

	// Send JSON-RPC ERROR RESPONSE, not a request
	// JSON-RPC 2.0 error response format
	nlohmann::json response = {
		{"jsonrpc", "2.0"},
		{"id", responseIdJson},
		{"error", {
			{"code", -32000},  // Server error code
			{"message", errorMessage}
		}}
	};

	return response.dump();
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

		// Validate JSON-RPC 2.0 structure (optional for KADI methods)
		if (j.contains("jsonrpc") && j["jsonrpc"] != "2.0")
		{
			DebuggerPrintf("KADIProtocolV1: Invalid JSON-RPC version\n");
			return false;
		}
		// If jsonrpc is missing, we tolerate it for KADI broker compatibility

		// Extract message ID (optional for notifications)
		// JSON-RPC 2.0 allows ID to be string, number, or null
		if (j.contains("id"))
		{
			if (j["id"].is_number_integer())
			{
				out.id = j["id"].get<int>();
			}
			else if (j["id"].is_string())
			{
				// Broker sends string IDs (e.g., "mcp-uuid-timestamp")
				// Hash to integer for compatibility with existing int-based ID system
				std::string idStr = j["id"].get<std::string>();
				out.id = static_cast<int>(std::hash<std::string>{}(idStr) & 0x7FFFFFFF);

				// Store mapping so we can send back the original string ID
				m_idMapping[out.id] = idStr;
			}
			else
			{
				out.id = -1;  // null or other type = notification
			}
		}
		else
		{
			out.id = -1;  // Missing ID = notification
		}

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
	if (method == "kadi.agent.register") return eKADIMessageType::REGISTER_TOOLS;
	if (method == "kadi.ability.request") return eKADIMessageType::TOOL_INVOKE;  // Broker sends 'request', not 'invoke'
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
