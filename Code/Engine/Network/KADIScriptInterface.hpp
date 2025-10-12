//----------------------------------------------------------------------------------------------------
// KADIScriptInterface.hpp
// V8 JavaScript bindings for KADI broker integration
//----------------------------------------------------------------------------------------------------

#pragma once
#include "Engine/Script/IScriptableObject.hpp"
#include "Engine/Network/KADIWebSocketSubsystem.hpp"

//----------------------------------------------------------------------------------------------------
// Disable V8 header warnings (external library)
#pragma warning(push)
#pragma warning(disable: 4100)  // unreferenced formal parameter
#pragma warning(disable: 4127)  // conditional expression is constant
#pragma warning(disable: 4324)  // structure was padded due to alignment specifier
#include <v8.h>

#pragma warning(pop)
#include <memory>

//----------------------------------------------------------------------------------------------------
// Forward Declarations
//----------------------------------------------------------------------------------------------------
class KADIWebSocketSubsystem;

//----------------------------------------------------------------------------------------------------
// KADIScriptInterface
// Exposes KADI functionality to JavaScript through V8 scripting system
//----------------------------------------------------------------------------------------------------
class KADIScriptInterface : public IScriptableObject
{
public:
	explicit KADIScriptInterface(KADIWebSocketSubsystem* kadiSubsystem);
	~KADIScriptInterface();

	// IScriptableObject Interface
	std::vector<ScriptMethodInfo> GetAvailableMethods() const override;
	StringList                    GetAvailableProperties() const override;

	ScriptMethodResult CallMethod(String const& methodName, ScriptArgs const& args) override;
	std::any           GetProperty(String const& propertyName) const override;
	bool               SetProperty(String const& propertyName, std::any const& value) override;

	// JavaScript Callback Registration
	void SetToolInvokeCallback(std::function<void(int, std::string, nlohmann::json)> callback);
	void SetEventDeliveryCallback(std::function<void(std::string, nlohmann::json)> callback);
	void SetConnectionStateCallback(std::function<void(std::string, std::string)> callback);

	// V8 Isolate Access (needed for callback invocation)
	void SetV8Isolate(v8::Isolate* isolate);

	// V8 Cleanup (must be called before V8 isolate destruction)
	void ClearCallbacks();

	// Callback Invocation from C++ (called by KADIWebSocketSubsystem)
	void InvokeToolInvokeCallback(int requestId, std::string const& toolName, nlohmann::json const& arguments);
	void InvokeEventDeliveryCallback(std::string const& channel, nlohmann::json const& data);
	void InvokeConnectionStateCallback(std::string const& oldState, std::string const& newState);

private:
	KADIWebSocketSubsystem* m_kadiSubsystem;
	v8::Isolate*            m_v8Isolate;

	void InitializeMethodRegistry() override;

	// Method Implementations
	ScriptMethodResult ExecuteConnect(ScriptArgs const& args);
	ScriptMethodResult ExecuteDisconnect(ScriptArgs const& args);
	ScriptMethodResult ExecuteGetConnectionState(ScriptArgs const& args);
	ScriptMethodResult ExecuteRegisterTools(ScriptArgs const& args);
	ScriptMethodResult ExecuteSendToolResult(ScriptArgs const& args);
	ScriptMethodResult ExecuteSendToolError(ScriptArgs const& args);
	ScriptMethodResult ExecuteSubscribeToEvents(ScriptArgs const& args);
	ScriptMethodResult ExecutePublishEvent(ScriptArgs const& args);
	ScriptMethodResult ExecuteOnToolInvoke(ScriptArgs const& args);
	ScriptMethodResult ExecuteOnEventDelivery(ScriptArgs const& args);
	ScriptMethodResult ExecuteOnConnectionStateChange(ScriptArgs const& args);
	ScriptMethodResult ExecuteGenerateKeyPair(ScriptArgs const& args);

	// JavaScript Callbacks (V8::Persistent function storage - Phase 2)
	std::unique_ptr<v8::Persistent<v8::Function>> m_jsToolInvokeCallback;
	std::unique_ptr<v8::Persistent<v8::Function>> m_jsEventDeliveryCallback;
	std::unique_ptr<v8::Persistent<v8::Function>> m_jsConnectionStateCallback;

	// V8 Context storage for callback invocation
	std::unique_ptr<v8::Persistent<v8::Context>> m_v8Context;

	// Connection State String Conversion
	static std::string ConnectionStateToString(eKADIConnectionState state);
};
