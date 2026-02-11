//----------------------------------------------------------------------------------------------------
// GenericCommandScriptInterface.hpp
// GenericCommand System - V8 JavaScript Bridge (Anti-Corruption Layer)
//
// Purpose:
//   Single universal V8 bridge for all GenericCommand operations.
//   This is the anti-corruption layer where V8 types are converted to std::any:
//     - Inbound (JS→C++): submit() extracts V8 args and converts to std::any payload
//   Outbound callback delivery (C++→JS) is handled by the existing
//   CallbackQueueScriptInterface.dequeueAll(), which already supports GENERIC type.
//
// JavaScript API (exposed methods):
//   - commandQueue.submit(type, payloadJson, agentId, callback?)
//       Submit a GenericCommand. Returns callbackId (or 0 if no callback).
//   - commandQueue.registerHandler(type, handlerFunc)
//       Register a C++ handler for a command type.
//   - commandQueue.unregisterHandler(type)
//       Remove a handler for a command type.
//   - commandQueue.getRegisteredTypes()
//       Get JSON array of registered command type strings.
//
// Thread Safety:
//   - All methods called from JavaScript worker thread
//   - submit() enqueues to GenericCommandQueue (SPSC, lock-free)
//   - registerHandler() uses executor's mutex (infrequent, startup only)
//
// Author: GenericCommand System - Phase 3
// Date: 2026-02-10
//----------------------------------------------------------------------------------------------------

#pragma once

//----------------------------------------------------------------------------------------------------
#include "Engine/Script/IScriptableObject.hpp"

//----------------------------------------------------------------------------------------------------
// Forward Declarations
class GenericCommandQueue;
class GenericCommandExecutor;

//----------------------------------------------------------------------------------------------------
class GenericCommandScriptInterface : public IScriptableObject
{
public:
	//------------------------------------------------------------------------------------------------
	// Construction / Destruction
	//------------------------------------------------------------------------------------------------
	GenericCommandScriptInterface(GenericCommandQueue*    commandQueue,
	                              GenericCommandExecutor* executor);
	~GenericCommandScriptInterface() override = default;

	//------------------------------------------------------------------------------------------------
	// IScriptableObject Interface
	//------------------------------------------------------------------------------------------------
	void                          InitializeMethodRegistry() override;
	ScriptMethodResult            CallMethod(String const& methodName, ScriptArgs const& args) override;
	std::vector<ScriptMethodInfo> GetAvailableMethods() const override;
	StringList                    GetAvailableProperties() const override;
	std::any                      GetProperty(String const& propertyName) const override;
	bool                          SetProperty(String const& propertyName, std::any const& value) override;

private:
	//------------------------------------------------------------------------------------------------
	// Implementation Methods
	//------------------------------------------------------------------------------------------------

	// Submit a GenericCommand to the queue
	// JS: commandQueue.submit(type, payloadJson, agentId, callback?)
	// Returns: callbackId (double) or 0 if no callback
	ScriptMethodResult ExecuteSubmit(ScriptArgs const& args);

	// Register a handler for a command type
	// JS: commandQueue.registerHandler(type, handlerFunc)
	// Returns: true/false
	ScriptMethodResult ExecuteRegisterHandler(ScriptArgs const& args);

	// Unregister a handler for a command type
	// JS: commandQueue.unregisterHandler(type)
	// Returns: true/false
	ScriptMethodResult ExecuteUnregisterHandler(ScriptArgs const& args);

	// Get list of registered command types
	// JS: commandQueue.getRegisteredTypes()
	// Returns: JSON array string
	ScriptMethodResult ExecuteGetRegisteredTypes(ScriptArgs const& args);

	//------------------------------------------------------------------------------------------------
	// Callback Delivery Note:
	//   executePendingCallbacks is NOT implemented here.
	//   GENERIC callbacks are delivered through the existing CallbackQueueScriptInterface.dequeueAll()
	//   which already handles all CallbackType values including GENERIC.
	//   The JS-side CommandQueue.js routes GENERIC callbacks to the appropriate JS callbacks.
	//   After full migration, all callback types become GENERIC and the legacy types are removed.
	//------------------------------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	// Internal Helpers
	//------------------------------------------------------------------------------------------------

	// Generate unique callback ID (atomic counter)
	uint64_t GenerateCallbackID();

	//------------------------------------------------------------------------------------------------
	// Dependencies (all owned by App, not by this interface)
	//------------------------------------------------------------------------------------------------
	GenericCommandQueue*    m_commandQueue;
	GenericCommandExecutor* m_executor;

	// Atomic callback ID counter
	uint64_t m_nextCallbackId = 1;
};
