//----------------------------------------------------------------------------------------------------
// GenericCommandScriptInterface.cpp
// GenericCommand System - V8 JavaScript Bridge Implementation
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Script/GenericCommandScriptInterface.hpp"

#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/GenericCommand.hpp"
#include "Engine/Core/GenericCommandExecutor.hpp"
#include "Engine/Core/GenericCommandQueue.hpp"
#include "Engine/Core/LogSubsystem.hpp"
#include "Engine/Script/ScriptTypeExtractor.hpp"

#include "ThirdParty/json/json.hpp"

//----------------------------------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------------------------------
GenericCommandScriptInterface::GenericCommandScriptInterface(
	GenericCommandQueue*    commandQueue,
	GenericCommandExecutor* executor)
	: m_commandQueue(commandQueue)
	, m_executor(executor)
{
	GUARANTEE_OR_DIE(m_commandQueue != nullptr,
	                 "GenericCommandScriptInterface: commandQueue is nullptr!");
	GUARANTEE_OR_DIE(m_executor != nullptr,
	                 "GenericCommandScriptInterface: executor is nullptr!");

	GenericCommandScriptInterface::InitializeMethodRegistry();

	DAEMON_LOG(LogScript, eLogVerbosity::Log,
	           "GenericCommandScriptInterface: Initialized");
}

//----------------------------------------------------------------------------------------------------
// InitializeMethodRegistry
//----------------------------------------------------------------------------------------------------
void GenericCommandScriptInterface::InitializeMethodRegistry()
{
	// No additional method registry initialization required
}

//----------------------------------------------------------------------------------------------------
// GetAvailableMethods
//----------------------------------------------------------------------------------------------------
std::vector<ScriptMethodInfo> GenericCommandScriptInterface::GetAvailableMethods() const
{
	return {
		ScriptMethodInfo("submit",
		                 "Submit a GenericCommand to the queue",
		                 {"type:string", "payloadJson:string", "agentId:string", "callback:function?"},
		                 "number"),
		ScriptMethodInfo("registerHandler",
		                 "Register a handler for a command type",
		                 {"type:string", "handler:function"},
		                 "boolean"),
		ScriptMethodInfo("unregisterHandler",
		                 "Unregister a handler for a command type",
		                 {"type:string"},
		                 "boolean"),
		ScriptMethodInfo("getRegisteredTypes",
		                 "Get JSON array of registered command types",
		                 {},
		                 "string")
	};
}

//----------------------------------------------------------------------------------------------------
// GetAvailableProperties
//----------------------------------------------------------------------------------------------------
StringList GenericCommandScriptInterface::GetAvailableProperties() const
{
	return {};
}

//----------------------------------------------------------------------------------------------------
// CallMethod
//----------------------------------------------------------------------------------------------------
ScriptMethodResult GenericCommandScriptInterface::CallMethod(
	String const&     methodName,
	ScriptArgs const& args)
{
	try
	{
		if (methodName == "submit")
		{
			return ExecuteSubmit(args);
		}
		if (methodName == "registerHandler")
		{
			return ExecuteRegisterHandler(args);
		}
		if (methodName == "unregisterHandler")
		{
			return ExecuteUnregisterHandler(args);
		}
		if (methodName == "getRegisteredTypes")
		{
			return ExecuteGetRegisteredTypes(args);
		}

		return ScriptMethodResult::Error("Unknown method: " + methodName);
	}
	catch (std::exception const& e)
	{
		return ScriptMethodResult::Error(
			"Method execution exception in '" + methodName + "': " + String(e.what()));
	}
}

//----------------------------------------------------------------------------------------------------
// GetProperty
//----------------------------------------------------------------------------------------------------
std::any GenericCommandScriptInterface::GetProperty(String const& propertyName) const
{
	UNUSED(propertyName)
	return std::any{};
}

//----------------------------------------------------------------------------------------------------
// SetProperty
//----------------------------------------------------------------------------------------------------
bool GenericCommandScriptInterface::SetProperty(String const& propertyName,
                                                 std::any const& value)
{
	UNUSED(propertyName)
	UNUSED(value)
	return false;
}

//----------------------------------------------------------------------------------------------------
// ExecuteSubmit
//
// JS: commandQueue.submit(type, payloadJson, agentId, callback?)
//
// Inbound V8→std::any conversion:
//   - type: string → String
//   - payloadJson: string → std::any (wrapping JSON string for handler to parse)
//   - agentId: string → String
//   - callback: function → std::any (opaque, stored for later delivery)
//
// Returns: callbackId as double (0 if no callback)
//----------------------------------------------------------------------------------------------------
ScriptMethodResult GenericCommandScriptInterface::ExecuteSubmit(ScriptArgs const& args)
{
	// Validate: 3 args (no callback) or 4 args (with callback)
	if (args.size() != 3 && args.size() != 4)
	{
		return ScriptMethodResult::Error(
			"submit: Expected 3-4 arguments (type, payloadJson, agentId, callback?), got " +
			std::to_string(args.size()));
	}

	try
	{
		// Extract arguments (V8→std::any conversion boundary)
		String type      = std::any_cast<std::string>(args[0]);
		String payload   = std::any_cast<std::string>(args[1]);
		String agentId   = std::any_cast<std::string>(args[2]);

		// Handle optional callback
		uint64_t callbackId = 0;
		std::any callback;

		if (args.size() == 4)
		{
			callbackId = GenerateCallbackID();
			callback   = args[3];  // Store as-is (opaque std::any wrapping V8 function)

			// Store callback in executor for later retrieval
			m_executor->StoreCallback(callbackId, callback);
		}

		// Create GenericCommand with JSON payload as std::any
		GenericCommand command(
			std::move(type),
			std::any(std::move(payload)),  // Wrap JSON string in std::any
			std::move(agentId),
			callbackId,
			std::move(callback)
		);

		// Submit to queue
		bool submitted = m_commandQueue->Submit(command);

		if (!submitted)
		{
			// Queue full — clean up stored callback
			if (callbackId != 0)
			{
				m_executor->RetrieveCallback(callbackId);  // Remove stored callback
			}
			return ScriptMethodResult::Error("submit: Queue full, command rejected");
		}

		// Return callbackId as double (JavaScript numbers are IEEE-754 doubles)
		double callbackIdDouble = static_cast<double>(callbackId);
		return ScriptMethodResult::Success(callbackIdDouble);
	}
	catch (std::bad_any_cast const& e)
	{
		return ScriptMethodResult::Error(
			"submit: Type conversion error - " + String(e.what()));
	}
}

//----------------------------------------------------------------------------------------------------
// ExecuteRegisterHandler
//
// JS: commandQueue.registerHandler(type, handlerFunc)
//
// The handlerFunc is a JavaScript function that will be wrapped as a C++ HandlerFunc.
// The wrapper converts std::any payload (JSON string) back to the handler.
//
// Note: In the current architecture, handlers are registered from C++ side (App/APIs).
// This JS method is provided for future extensibility where JS can register handlers
// that process commands on the main thread.
//----------------------------------------------------------------------------------------------------
ScriptMethodResult GenericCommandScriptInterface::ExecuteRegisterHandler(ScriptArgs const& args)
{
	if (args.size() != 2)
	{
		return ScriptMethodResult::Error(
			"registerHandler: Expected 2 arguments (type, handler), got " +
			std::to_string(args.size()));
	}

	try
	{
		String type = std::any_cast<std::string>(args[0]);

		// Store the JS function as std::any — it will be invoked by the executor
		// The handler receives std::any payload (JSON string) and returns HandlerResult
		std::any jsHandler = args[1];

		// Create a C++ HandlerFunc wrapper
		// Note: The JS function is stored as std::any; actual V8 invocation
		// would require V8Subsystem access, which is a future enhancement.
		// For now, handlers are registered from C++ side (EntityAPI, CameraAPI, etc.)
		HandlerFunc handler = [jsHandler](std::any const& payload) -> HandlerResult
		{
			UNUSED(payload)
			// Placeholder: JS-registered handlers are a future enhancement
			// C++ handlers are registered directly via GenericCommandExecutor::RegisterHandler
			return HandlerResult::Error("JS-registered handlers not yet implemented");
		};

		bool registered = m_executor->RegisterHandler(type, std::move(handler));
		return ScriptMethodResult::Success(registered);
	}
	catch (std::bad_any_cast const& e)
	{
		return ScriptMethodResult::Error(
			"registerHandler: Type conversion error - " + String(e.what()));
	}
}

//----------------------------------------------------------------------------------------------------
// ExecuteUnregisterHandler
//
// JS: commandQueue.unregisterHandler(type)
//----------------------------------------------------------------------------------------------------
ScriptMethodResult GenericCommandScriptInterface::ExecuteUnregisterHandler(ScriptArgs const& args)
{
	if (args.size() != 1)
	{
		return ScriptMethodResult::Error(
			"unregisterHandler: Expected 1 argument (type), got " +
			std::to_string(args.size()));
	}

	try
	{
		String type = std::any_cast<std::string>(args[0]);
		bool removed = m_executor->UnregisterHandler(type);
		return ScriptMethodResult::Success(removed);
	}
	catch (std::bad_any_cast const& e)
	{
		return ScriptMethodResult::Error(
			"unregisterHandler: Type conversion error - " + String(e.what()));
	}
}

//----------------------------------------------------------------------------------------------------
// ExecuteGetRegisteredTypes
//
// JS: commandQueue.getRegisteredTypes()
// Returns: JSON array string of registered command types
//----------------------------------------------------------------------------------------------------
ScriptMethodResult GenericCommandScriptInterface::ExecuteGetRegisteredTypes(ScriptArgs const& args)
{
	if (!args.empty())
	{
		return ScriptMethodResult::Error(
			"getRegisteredTypes: Expected 0 arguments, got " +
			std::to_string(args.size()));
	}

	std::vector<String> types = m_executor->GetRegisteredTypes();

	nlohmann::json typesArray = nlohmann::json::array();
	for (String const& type : types)
	{
		typesArray.push_back(type);
	}

	return ScriptMethodResult::Success(typesArray.dump());
}

//----------------------------------------------------------------------------------------------------
// GenerateCallbackID
//----------------------------------------------------------------------------------------------------
uint64_t GenericCommandScriptInterface::GenerateCallbackID()
{
	return m_nextCallbackId++;
}
