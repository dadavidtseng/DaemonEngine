//----------------------------------------------------------------------------------------------------
// CallbackQueueScriptInterface.cpp
// Phase 2.4: JavaScript Interface for CallbackQueue Implementation
//----------------------------------------------------------------------------------------------------

#include "Engine/Core/CallbackQueueScriptInterface.hpp"

#include "Engine/Core/CallbackQueue.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/LogSubsystem.hpp"
#include "Engine/Script/ScriptTypeExtractor.hpp"

#include "ThirdParty/json/json.hpp"

//----------------------------------------------------------------------------------------------------
// Constructor
//
// Initializes CallbackQueueScriptInterface with pointer to CallbackQueue.
//----------------------------------------------------------------------------------------------------
CallbackQueueScriptInterface::CallbackQueueScriptInterface(CallbackQueue* callbackQueue)
	: m_callbackQueue(callbackQueue)
{
	if (!m_callbackQueue)
	{
		ERROR_AND_DIE("CallbackQueueScriptInterface: CallbackQueue pointer cannot be null");
	}

	CallbackQueueScriptInterface::InitializeMethodRegistry();
}

//----------------------------------------------------------------------------------------------------
// GetAvailableMethods (IScriptableObject Interface)
//
// Returns list of JavaScript methods exposed by this interface.
//----------------------------------------------------------------------------------------------------
std::vector<ScriptMethodInfo> CallbackQueueScriptInterface::GetAvailableMethods() const
{
	return {
		ScriptMethodInfo("dequeueAll",
		                 "Dequeue all callbacks and invoke processor function for each",
		                 {"processor:function"},
		                 "number")  // Returns number of callbacks processed
	};
}

//----------------------------------------------------------------------------------------------------
// GetAvailableProperties (IScriptableObject Interface)
//
// No properties exposed for CallbackQueue.
//----------------------------------------------------------------------------------------------------
StringList CallbackQueueScriptInterface::GetAvailableProperties() const
{
	return {};
}

//----------------------------------------------------------------------------------------------------
// CallMethod (IScriptableObject Interface)
//
// Routes JavaScript method calls to implementation methods.
//----------------------------------------------------------------------------------------------------
ScriptMethodResult CallbackQueueScriptInterface::CallMethod(String const&     methodName,
                                                             ScriptArgs const& args)
{
	try
	{
		if (methodName == "dequeueAll")
		{
			return ExecuteDequeueAll(args);
		}

		return ScriptMethodResult::Error("Unknown method: " + methodName);
	}
	catch (std::exception const& e)
	{
		return ScriptMethodResult::Error("Method execution exception: " + String(e.what()));
	}
}

//----------------------------------------------------------------------------------------------------
// GetProperty (IScriptableObject Interface)
//
// No properties to get.
//----------------------------------------------------------------------------------------------------
std::any CallbackQueueScriptInterface::GetProperty(const String& propertyName) const
{
	UNUSED(propertyName)
	return std::any{};
}

//----------------------------------------------------------------------------------------------------
// SetProperty (IScriptableObject Interface)
//
// No properties to set.
//----------------------------------------------------------------------------------------------------
bool CallbackQueueScriptInterface::SetProperty(const String& propertyName, const std::any& value)
{
	UNUSED(propertyName)
	UNUSED(value)
	return false;
}

//----------------------------------------------------------------------------------------------------
// InitializeMethodRegistry (IScriptableObject Interface)
//
// Initializes method registry for CallbackQueue operations.
// No additional initialization needed beyond constructor.
//----------------------------------------------------------------------------------------------------
void CallbackQueueScriptInterface::InitializeMethodRegistry()
{
	// No additional method registry initialization required
}

//----------------------------------------------------------------------------------------------------
// ExecuteDequeueAll (Implementation Method)
//
// Dequeues all callbacks from CallbackQueue and returns them as JSON array for JavaScript.
//
// JavaScript Signature:
//   const callbacks = callbackQueue.dequeueAll()  // Returns array of callback objects
//
// CallbackData Structure (JavaScript object in array):
//   {
//       callbackId: number,
//       resultId: number,
//       errorMessage: string,
//       type: string  // "ENTITY_CREATED", "CAMERA_CREATED", etc.
//   }
//
// Returns:
//   JSON string containing array of callbacks
//
// Thread Safety:
//   - Called from JavaScript worker thread
//   - CallbackQueue is lock-free SPSC (safe for worker thread consumption)
//   - JavaScript processes callbacks after this method returns
//----------------------------------------------------------------------------------------------------
ScriptMethodResult CallbackQueueScriptInterface::ExecuteDequeueAll(ScriptArgs const& args)
{
	// Validate arguments (no arguments needed - dequeue all available callbacks)
	if (args.size() != 0)
	{
		return ScriptMethodResult::Error("dequeueAll() requires no arguments");
	}

	// Collect all callbacks into JSON array
	nlohmann::json callbacksArray = nlohmann::json::array();

	m_callbackQueue->DequeueAll([&](CallbackData const& cb)
	{
		// Convert CallbackData to JSON object
		nlohmann::json callbackJson;
		callbackJson["callbackId"]    = cb.callbackId;
		callbackJson["resultId"]      = cb.resultId;
		callbackJson["errorMessage"]  = cb.errorMessage;

		// Convert CallbackType enum to string
		switch (cb.type)
		{
		case CallbackType::ENTITY_CREATED:
			callbackJson["type"] = "ENTITY_CREATED";
			break;
		case CallbackType::CAMERA_CREATED:
			callbackJson["type"] = "CAMERA_CREATED";
			break;
		case CallbackType::RESOURCE_LOADED:
			callbackJson["type"] = "RESOURCE_LOADED";
			break;
		case CallbackType::GENERIC:
			callbackJson["type"] = "GENERIC";
			break;
		default:
			callbackJson["type"] = "UNKNOWN";
			break;
		}

		// Add to callbacks array
		callbacksArray.push_back(callbackJson);
	});

	// Convert JSON array to string for JavaScript
	String callbacksJson = callbacksArray.dump();

	// Ensure we return valid JSON even for empty array
	if (callbacksJson.empty())
	{
		callbacksJson = "[]";
	}

	// Phase 2.4: Callback dequeue logging disabled (diagnostic only, no longer needed)
	// Callbacks are processed silently for production performance

	// Return JSON string containing all callbacks
	return ScriptMethodResult::Success(callbacksJson);
}
