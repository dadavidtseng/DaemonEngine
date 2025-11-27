//----------------------------------------------------------------------------------------------------
// CallbackQueueScriptInterface.hpp
// Phase 2.4: JavaScript Interface for CallbackQueue (Callback Dequeuing)
//
// Purpose:
//   Exposes CallbackQueue to JavaScript runtime for callback dequeuing on worker thread.
//   Provides JavaScript API to process callbacks enqueued by C++ main thread.
//
// Design Philosophy:
//   - Minimal interface: Only expose dequeue functionality
//   - Worker thread execution: Callbacks dequeued and executed on JavaScript worker thread
//   - Error resilient: Callback execution errors don't crash update loop
//
// JavaScript API (exposed methods):
//   - callbackQueue.dequeueAll() - Dequeue all callbacks and return as JSON array
//
// Usage Example (from JavaScript - JSEngine.js):
//   const callbacksJson = callbackQueue.dequeueAll();  // Returns JSON string
//   const callbacks = JSON.parse(callbacksJson);      // Parse to array
//   for (const cb of callbacks) {
//       executeCallback(cb);
//   }
//
// Thread Safety:
//   - CallbackQueue is lock-free SPSC queue (safe for worker thread consumption)
//   - Callbacks executed on JavaScript worker thread (same thread as JSEngine.update())
//   - No V8 locking needed (already protected by JSGameLogicJob's v8::Locker)
//
// Author: Phase 2.4 - JavaScript Callback Processing
// Date: 2025-11-26
//----------------------------------------------------------------------------------------------------

#pragma once

//----------------------------------------------------------------------------------------------------
#include "Engine/Script/IScriptableObject.hpp"

//----------------------------------------------------------------------------------------------------
// Forward Declarations
class CallbackQueue;

//----------------------------------------------------------------------------------------------------
// CallbackQueueScriptInterface
//
// JavaScript interface for callback queue consumption.
// Wraps CallbackQueue and exposes dequeue method to V8 JavaScript runtime.
//
// Registration:
//   - Registered in ScriptSubsystem as "callbackQueue" global object
//   - Accessible from JavaScript worker thread (JSEngine.update())
//
// Method Naming Convention:
//   - JavaScript methods use camelCase (e.g., dequeueAll)
//   - C++ methods map to CallbackQueue (e.g., DequeueAll)
//
// Error Handling:
//   - Invalid parameters return ScriptMethodResult::Error()
//   - Callback execution errors caught in JavaScript (try-catch in executeCallback)
//   - Errors logged to console, don't crash update loop
//----------------------------------------------------------------------------------------------------
class CallbackQueueScriptInterface : public IScriptableObject
{
public:
	//------------------------------------------------------------------------------------------------
	// Construction / Destruction
	//------------------------------------------------------------------------------------------------
	explicit CallbackQueueScriptInterface(CallbackQueue* callbackQueue);
	~CallbackQueueScriptInterface() override = default;

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
	ScriptMethodResult ExecuteDequeueAll(ScriptArgs const& args);

	//------------------------------------------------------------------------------------------------
	// Dependencies
	//------------------------------------------------------------------------------------------------
	CallbackQueue* m_callbackQueue;  // Pointer to CallbackQueue (owned by App)
};
