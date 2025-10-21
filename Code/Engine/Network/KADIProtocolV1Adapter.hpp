//----------------------------------------------------------------------------------------------------
// KADIProtocolV1Adapter.hpp
// JSON-RPC 2.0 implementation of KADI protocol adapter
// Implements KADI broker protocol version 1.0 (current specification)
//----------------------------------------------------------------------------------------------------

#pragma once

#include "Engine/Network/IKADIProtocolAdapter.hpp"
#include "Engine/Core/StringUtils.hpp"

#include <map>  // For ID mapping (string ID to int hash)

//----------------------------------------------------------------------------------------------------
// KADI Protocol V1 Adapter (JSON-RPC 2.0 over WebSocket)
//----------------------------------------------------------------------------------------------------
class KADIProtocolV1Adapter : public IKADIProtocolAdapter
{
public:
	KADIProtocolV1Adapter();
	~KADIProtocolV1Adapter() override = default;

	//----------------------------------------------------------------------------------------------------
	// Serialization (Outgoing Messages)
	//----------------------------------------------------------------------------------------------------

	std::string SerializeHello() override;

	std::string SerializeAuthenticate(std::string const& publicKey,
	                                   std::string const& signature,
	                                   std::string const& nonce,
	                                   bool wantNewId = true) override;

	std::string SerializeToolRegistration(nlohmann::json const& tools) override;

	std::string SerializeToolResult(int requestId, nlohmann::json const& result) override;

	std::string SerializeToolError(int requestId, std::string const& errorMessage) override;

	std::string SerializeEventPublish(std::string const& channel,
	                                   nlohmann::json const& data) override;

	std::string SerializeEventSubscribe(std::vector<std::string> const& channels) override;

	std::string SerializePing() override;

	//----------------------------------------------------------------------------------------------------
	// Deserialization (Incoming Messages)
	//----------------------------------------------------------------------------------------------------

	bool ParseMessage(std::string const& message, sKADIMessage& out) override;

private:
	//----------------------------------------------------------------------------------------------------
	// Helper Methods
	//----------------------------------------------------------------------------------------------------

	/// @brief Create standard JSON-RPC 2.0 request
	/// @param method Method name (e.g., "kadi.session.hello")
	/// @param params Method parameters (JSON object or array)
	/// @return JSON-RPC request object
	nlohmann::json CreateRequest(std::string const& method, nlohmann::json const& params);

	/// @brief Create standard JSON-RPC 2.0 response
	/// @param id Request ID
	/// @param result Response result (JSON object or primitive)
	/// @return JSON-RPC response object
	nlohmann::json CreateResponse(int id, nlohmann::json const& result);

	/// @brief Create standard JSON-RPC 2.0 error response
	/// @param id Request ID
	/// @param errorCode Error code (e.g., -32600 for invalid request)
	/// @param errorMessage Error description
	/// @return JSON-RPC error response object
	nlohmann::json CreateError(int id, int errorCode, std::string const& errorMessage);

	/// @brief Parse JSON-RPC method name to message type
	/// @param method Method name string
	/// @return Corresponding eKADIMessageType
	eKADIMessageType ParseMethodToType(std::string const& method);

	//----------------------------------------------------------------------------------------------------
	// ID Mapping (for broker compatibility with string IDs)
	//----------------------------------------------------------------------------------------------------
	std::map<int, std::string> m_idMapping;  // Maps hashed int IDs back to original string IDs
};
