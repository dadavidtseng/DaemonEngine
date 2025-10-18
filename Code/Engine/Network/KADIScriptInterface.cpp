//----------------------------------------------------------------------------------------------------
// KADIScriptInterface.cpp
// V8 JavaScript bindings implementation for KADI broker integration
//----------------------------------------------------------------------------------------------------
#ifndef NOMINMAX
#define NOMINMAX
#endif

// Disable V8 header warnings (external library)
#pragma warning(push)
#pragma warning(disable: 4100)  // unreferenced formal parameter
#pragma warning(disable: 4127)  // conditional expression is constant
#pragma warning(disable: 4324)  // structure was padded due to alignment specifier

#include "Engine/Network/KADIScriptInterface.hpp"
#include "Engine/Network/KADIAuthenticationUtility.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Script/ScriptTypeExtractor.hpp"

#pragma warning(pop)

//----------------------------------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------------------------------
KADIScriptInterface::KADIScriptInterface(KADIWebSocketSubsystem* kadiSubsystem)
	: m_kadiSubsystem(kadiSubsystem)
	, m_v8Isolate(nullptr)
{
	if (!m_kadiSubsystem)
	{
		ERROR_AND_DIE("KADIScriptInterface: KADIWebSocketSubsystem pointer cannot be null");
	}

	InitializeMethodRegistry();
}

//----------------------------------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------------------------------
KADIScriptInterface::~KADIScriptInterface()
{
	// Defensive cleanup - only if ClearCallbacks() wasn't called
	// Note: ClearCallbacks() should be called explicitly before V8 isolate destruction
	ClearCallbacks();
}

//----------------------------------------------------------------------------------------------------
// IScriptableObject Interface Implementation
//----------------------------------------------------------------------------------------------------

std::vector<ScriptMethodInfo> KADIScriptInterface::GetAvailableMethods() const
{
	return {
		ScriptMethodInfo("connect",
		                 "Connect to KADI broker with authentication keys",
		                 {"string", "string", "string"},  // brokerUrl, publicKey, privateKey
		                 "void"),

		ScriptMethodInfo("disconnect",
		                 "Disconnect from KADI broker",
		                 {},
		                 "void"),

		ScriptMethodInfo("getConnectionState",
		                 "Get current connection state as string",
		                 {},
		                 "string"),

		ScriptMethodInfo("registerTools",
		                 "Register tool capabilities with broker",
		                 {"string"},  // JSON string of tools array
		                 "void"),

		ScriptMethodInfo("sendToolResult",
		                 "Send tool execution result to broker",
		                 {"number", "string"},  // requestId, JSON result string
		                 "void"),

		ScriptMethodInfo("sendToolError",
		                 "Send tool execution error to broker",
		                 {"number", "string"},  // requestId, errorMessage
		                 "void"),

		ScriptMethodInfo("subscribeToEvents",
		                 "Subscribe to event channels",
		                 {"string"},  // JSON string of channels array
		                 "void"),

		ScriptMethodInfo("publishEvent",
		                 "Publish event to channel",
		                 {"string", "string"},  // channel, JSON data string
		                 "void"),

		ScriptMethodInfo("onToolInvoke",
		                 "Register JavaScript callback for tool invocations (callback receives: requestId, toolName, argumentsJSON)",
		                 {"function"},
		                 "void"),

		ScriptMethodInfo("onEventDelivery",
		                 "Register JavaScript callback for event deliveries (callback receives: channel, dataJSON)",
		                 {"function"},
		                 "void"),

		ScriptMethodInfo("onConnectionStateChange",
		                 "Register JavaScript callback for connection state changes (callback receives: oldState, newState)",
		                 {"function"},
		                 "void"),

		ScriptMethodInfo("generateKeyPair",
		                 "Generate new Ed25519 key pair (returns: {publicKey: string, privateKey: string})",
		                 {},
		                 "object")
	};
}

StringList KADIScriptInterface::GetAvailableProperties() const
{
	return {};  // Phase 1: No properties, only methods
}

ScriptMethodResult KADIScriptInterface::CallMethod(String const& methodName, ScriptArgs const& args)
{
	try
	{
		if (methodName == "connect") return ExecuteConnect(args);
		if (methodName == "disconnect") return ExecuteDisconnect(args);
		if (methodName == "getConnectionState") return ExecuteGetConnectionState(args);
		if (methodName == "registerTools") return ExecuteRegisterTools(args);
		if (methodName == "sendToolResult") return ExecuteSendToolResult(args);
		if (methodName == "sendToolError") return ExecuteSendToolError(args);
		if (methodName == "subscribeToEvents") return ExecuteSubscribeToEvents(args);
		if (methodName == "publishEvent") return ExecutePublishEvent(args);
		if (methodName == "onToolInvoke") return ExecuteOnToolInvoke(args);
		if (methodName == "onEventDelivery") return ExecuteOnEventDelivery(args);
		if (methodName == "onConnectionStateChange") return ExecuteOnConnectionStateChange(args);
		if (methodName == "generateKeyPair") return ExecuteGenerateKeyPair(args);

		return ScriptMethodResult::Error("Unknown method: " + methodName);
	}
	catch (std::exception const& e)
	{
		return ScriptMethodResult::Error("Method execution exception: " + String(e.what()));
	}
}

std::any KADIScriptInterface::GetProperty(String const& propertyName) const
{
	UNUSED(propertyName);
	return std::any{};
}

bool KADIScriptInterface::SetProperty(String const& propertyName, std::any const& value)
{
	UNUSED(propertyName);
	UNUSED(value);
	return false;
}

void KADIScriptInterface::InitializeMethodRegistry()
{
	// Phase 1: No method registry initialization needed
}

//----------------------------------------------------------------------------------------------------
// Method Implementations
//----------------------------------------------------------------------------------------------------

ScriptMethodResult KADIScriptInterface::ExecuteConnect(ScriptArgs const& args)
{
	auto result = ScriptTypeExtractor::ValidateArgCount(args, 3, "connect");
	if (!result.success) return result;

	try
	{
		String brokerUrl = ScriptTypeExtractor::ExtractString(args[0]);
		String publicKey = ScriptTypeExtractor::ExtractString(args[1]);
		String privateKey = ScriptTypeExtractor::ExtractString(args[2]);

		m_kadiSubsystem->Connect(brokerUrl, publicKey, privateKey);

		return ScriptMethodResult::Success();
	}
	catch (std::exception const& e)
	{
		return ScriptMethodResult::Error("KADI connect failed: " + String(e.what()));
	}
}

ScriptMethodResult KADIScriptInterface::ExecuteDisconnect(ScriptArgs const& args)
{
	auto result = ScriptTypeExtractor::ValidateArgCount(args, 0, "disconnect");
	if (!result.success) return result;

	try
	{
		m_kadiSubsystem->Disconnect();
		return ScriptMethodResult::Success();
	}
	catch (std::exception const& e)
	{
		return ScriptMethodResult::Error("KADI disconnect failed: " + String(e.what()));
	}
}

ScriptMethodResult KADIScriptInterface::ExecuteGetConnectionState(ScriptArgs const& args)
{
	auto result = ScriptTypeExtractor::ValidateArgCount(args, 0, "getConnectionState");
	if (!result.success) return result;

	try
	{
		eKADIConnectionState state = m_kadiSubsystem->GetConnectionState();
		String stateString = ConnectionStateToString(state);
		return ScriptMethodResult::Success(stateString);
	}
	catch (std::exception const& e)
	{
		return ScriptMethodResult::Error("Get connection state failed: " + String(e.what()));
	}
}

ScriptMethodResult KADIScriptInterface::ExecuteRegisterTools(ScriptArgs const& args)
{
	auto result = ScriptTypeExtractor::ValidateArgCount(args, 1, "registerTools");
	if (!result.success) return result;

	try
	{
		String toolsJSON = ScriptTypeExtractor::ExtractString(args[0]);

		// Parse JSON string to nlohmann::json
		nlohmann::json tools = nlohmann::json::parse(std::string(toolsJSON));

		m_kadiSubsystem->RegisterTools(tools);

		return ScriptMethodResult::Success();
	}
	catch (nlohmann::json::parse_error const& e)
	{
		return ScriptMethodResult::Error("KADI registerTools JSON parse error: " + String(e.what()));
	}
	catch (std::exception const& e)
	{
		return ScriptMethodResult::Error("KADI registerTools failed: " + String(e.what()));
	}
}

ScriptMethodResult KADIScriptInterface::ExecuteSendToolResult(ScriptArgs const& args)
{
	auto result = ScriptTypeExtractor::ValidateArgCount(args, 2, "sendToolResult");
	if (!result.success) return result;

	try
	{
		int requestId = ScriptTypeExtractor::ExtractInt(args[0]);
		String resultJSON = ScriptTypeExtractor::ExtractString(args[1]);

		// Parse JSON string to nlohmann::json
		nlohmann::json resultData = nlohmann::json::parse(std::string(resultJSON));

		m_kadiSubsystem->SendToolResult(requestId, resultData);

		return ScriptMethodResult::Success();
	}
	catch (nlohmann::json::parse_error const& e)
	{
		return ScriptMethodResult::Error("KADI sendToolResult JSON parse error: " + String(e.what()));
	}
	catch (std::exception const& e)
	{
		return ScriptMethodResult::Error("KADI sendToolResult failed: " + String(e.what()));
	}
}

ScriptMethodResult KADIScriptInterface::ExecuteSendToolError(ScriptArgs const& args)
{
	auto result = ScriptTypeExtractor::ValidateArgCount(args, 2, "sendToolError");
	if (!result.success) return result;

	try
	{
		int requestId = ScriptTypeExtractor::ExtractInt(args[0]);
		String errorMessage = ScriptTypeExtractor::ExtractString(args[1]);

		m_kadiSubsystem->SendToolError(requestId, errorMessage);

		return ScriptMethodResult::Success();
	}
	catch (std::exception const& e)
	{
		return ScriptMethodResult::Error("KADI sendToolError failed: " + String(e.what()));
	}
}

ScriptMethodResult KADIScriptInterface::ExecuteSubscribeToEvents(ScriptArgs const& args)
{
	auto result = ScriptTypeExtractor::ValidateArgCount(args, 1, "subscribeToEvents");
	if (!result.success) return result;

	try
	{
		String channelsJSON = ScriptTypeExtractor::ExtractString(args[0]);

		// Parse JSON string to nlohmann::json array
		nlohmann::json channelsArray = nlohmann::json::parse(std::string(channelsJSON));

		if (!channelsArray.is_array())
		{
			return ScriptMethodResult::Error("KADI subscribeToEvents requires JSON array of channels");
		}

		std::vector<std::string> channels;
		for (auto const& channel : channelsArray)
		{
			if (channel.is_string())
			{
				channels.push_back(channel.get<std::string>());
			}
		}

		m_kadiSubsystem->SubscribeToEvents(channels);

		return ScriptMethodResult::Success();
	}
	catch (nlohmann::json::parse_error const& e)
	{
		return ScriptMethodResult::Error("KADI subscribeToEvents JSON parse error: " + String(e.what()));
	}
	catch (std::exception const& e)
	{
		return ScriptMethodResult::Error("KADI subscribeToEvents failed: " + String(e.what()));
	}
}

ScriptMethodResult KADIScriptInterface::ExecutePublishEvent(ScriptArgs const& args)
{
	auto result = ScriptTypeExtractor::ValidateArgCount(args, 2, "publishEvent");
	if (!result.success) return result;

	try
	{
		String channel = ScriptTypeExtractor::ExtractString(args[0]);
		String dataJSON = ScriptTypeExtractor::ExtractString(args[1]);

		// Parse JSON string to nlohmann::json
		nlohmann::json data = nlohmann::json::parse(std::string(dataJSON));

		m_kadiSubsystem->PublishEvent(channel, data);

		return ScriptMethodResult::Success();
	}
	catch (nlohmann::json::parse_error const& e)
	{
		return ScriptMethodResult::Error("KADI publishEvent JSON parse error: " + String(e.what()));
	}
	catch (std::exception const& e)
	{
		return ScriptMethodResult::Error("KADI publishEvent failed: " + String(e.what()));
	}
}

ScriptMethodResult KADIScriptInterface::ExecuteOnToolInvoke(ScriptArgs const& args)
{
	auto result = ScriptTypeExtractor::ValidateArgCount(args, 1, "onToolInvoke");
	if (!result.success) return result;

	try
	{
		if (!m_v8Isolate)
		{
			return ScriptMethodResult::Error("KADI onToolInvoke: V8 isolate not initialized");
		}

		// Extract v8::Local<v8::Function> from std::any
		// The ScriptSubsystem passes v8::Function wrapped in std::any
		if (args[0].type() != typeid(v8::Local<v8::Function>))
		{
			return ScriptMethodResult::Error("KADI onToolInvoke: Argument must be a function");
		}

		v8::Local<v8::Function> callback = std::any_cast<v8::Local<v8::Function>>(args[0]);

		// Store as persistent handle
		m_jsToolInvokeCallback = std::make_unique<v8::Persistent<v8::Function>>(m_v8Isolate, callback);

		// Store current context for later invocation
		v8::Local<v8::Context> context = m_v8Isolate->GetCurrentContext();
		if (!m_v8Context)
		{
			m_v8Context = std::make_unique<v8::Persistent<v8::Context>>(m_v8Isolate, context);
		}

		// Register C++ callback with KADIWebSocketSubsystem
		m_kadiSubsystem->SetToolInvokeCallback(
			[this](int requestId, std::string const& toolName, nlohmann::json const& arguments)
			{
				InvokeToolInvokeCallback(requestId, toolName, arguments);
			}
		);

		DebuggerPrintf("KADIScriptInterface: onToolInvoke callback registered successfully\n");

		return ScriptMethodResult::Success();
	}
	catch (std::exception const& e)
	{
		return ScriptMethodResult::Error("KADI onToolInvoke failed: " + String(e.what()));
	}
}

ScriptMethodResult KADIScriptInterface::ExecuteOnEventDelivery(ScriptArgs const& args)
{
	auto result = ScriptTypeExtractor::ValidateArgCount(args, 1, "onEventDelivery");
	if (!result.success) return result;

	try
	{
		if (!m_v8Isolate)
		{
			return ScriptMethodResult::Error("KADI onEventDelivery: V8 isolate not initialized");
		}

		// Extract v8::Local<v8::Function> from std::any
		if (args[0].type() != typeid(v8::Local<v8::Function>))
		{
			return ScriptMethodResult::Error("KADI onEventDelivery: Argument must be a function");
		}

		v8::Local<v8::Function> callback = std::any_cast<v8::Local<v8::Function>>(args[0]);

		// Store as persistent handle
		m_jsEventDeliveryCallback = std::make_unique<v8::Persistent<v8::Function>>(m_v8Isolate, callback);

		// Store current context for later invocation
		v8::Local<v8::Context> context = m_v8Isolate->GetCurrentContext();
		if (!m_v8Context)
		{
			m_v8Context = std::make_unique<v8::Persistent<v8::Context>>(m_v8Isolate, context);
		}

		// Register C++ callback with KADIWebSocketSubsystem
		m_kadiSubsystem->SetEventDeliveryCallback(
			[this](std::string const& channel, nlohmann::json const& data)
			{
				InvokeEventDeliveryCallback(channel, data);
			}
		);

		DebuggerPrintf("KADIScriptInterface: onEventDelivery callback registered successfully\n");

		return ScriptMethodResult::Success();
	}
	catch (std::exception const& e)
	{
		return ScriptMethodResult::Error("KADI onEventDelivery failed: " + String(e.what()));
	}
}

ScriptMethodResult KADIScriptInterface::ExecuteOnConnectionStateChange(ScriptArgs const& args)
{
	auto result = ScriptTypeExtractor::ValidateArgCount(args, 1, "onConnectionStateChange");
	if (!result.success) return result;

	try
	{
		if (!m_v8Isolate)
		{
			return ScriptMethodResult::Error("KADI onConnectionStateChange: V8 isolate not initialized");
		}

		// Extract v8::Local<v8::Function> from std::any
		if (args[0].type() != typeid(v8::Local<v8::Function>))
		{
			return ScriptMethodResult::Error("KADI onConnectionStateChange: Argument must be a function");
		}

		v8::Local<v8::Function> callback = std::any_cast<v8::Local<v8::Function>>(args[0]);

		// Store as persistent handle
		m_jsConnectionStateCallback = std::make_unique<v8::Persistent<v8::Function>>(m_v8Isolate, callback);

		// Store current context for later invocation
		v8::Local<v8::Context> context = m_v8Isolate->GetCurrentContext();
		if (!m_v8Context)
		{
			m_v8Context = std::make_unique<v8::Persistent<v8::Context>>(m_v8Isolate, context);
		}

		// Register C++ callback with KADIWebSocketSubsystem
		m_kadiSubsystem->SetConnectionStateCallback(
			[this](eKADIConnectionState oldState, eKADIConnectionState newState)
			{
				std::string oldStateStr = ConnectionStateToString(oldState);
				std::string newStateStr = ConnectionStateToString(newState);
				InvokeConnectionStateCallback(oldStateStr, newStateStr);
			}
		);

		DebuggerPrintf("KADIScriptInterface: onConnectionStateChange callback registered successfully\n");

		return ScriptMethodResult::Success();
	}
	catch (std::exception const& e)
	{
		return ScriptMethodResult::Error("KADI onConnectionStateChange failed: " + String(e.what()));
	}
}

ScriptMethodResult KADIScriptInterface::ExecuteGenerateKeyPair(ScriptArgs const& args)
{
	auto validationResult = ScriptTypeExtractor::ValidateArgCount(args, 0, "generateKeyPair");
	if (!validationResult.success) return validationResult;

	try
	{
		sEd25519KeyPair keyPair;
		if (!KADIAuthenticationUtility::GenerateKeyPair(keyPair))
		{
			return ScriptMethodResult::Error("KADI generateKeyPair: Key generation failed");
		}

		// Convert to base64 strings
		String publicKeyBase64 = keyPair.GetPublicKeyBase64();
		String privateKeyBase64 = keyPair.GetPrivateKeyBase64();

		// Return as JSON string
		nlohmann::json result = {
			{"publicKey", std::string(publicKeyBase64)},
			{"privateKey", std::string(privateKeyBase64)}
		};

		String resultJSON = result.dump();

		return ScriptMethodResult::Success(resultJSON);
	}
	catch (std::exception const& e)
	{
		return ScriptMethodResult::Error("KADI generateKeyPair failed: " + String(e.what()));
	}
}

//----------------------------------------------------------------------------------------------------
// V8 Integration (Phase 2)
//----------------------------------------------------------------------------------------------------

void KADIScriptInterface::SetV8Isolate(v8::Isolate* isolate)
{
	m_v8Isolate = isolate;
}

void KADIScriptInterface::ClearCallbacks()
{
	// Clean up V8::Persistent handles safely
	// This must be called BEFORE V8 isolate destruction
	if (m_v8Isolate)
	{
		if (m_jsToolInvokeCallback)
		{
			m_jsToolInvokeCallback->Reset();
			m_jsToolInvokeCallback.reset();
		}
		if (m_jsEventDeliveryCallback)
		{
			m_jsEventDeliveryCallback->Reset();
			m_jsEventDeliveryCallback.reset();
		}
		if (m_jsConnectionStateCallback)
		{
			m_jsConnectionStateCallback->Reset();
			m_jsConnectionStateCallback.reset();
		}
		if (m_v8Context)
		{
			m_v8Context->Reset();
			m_v8Context.reset();
		}

		// Clear isolate reference to prevent further callback invocations
		m_v8Isolate = nullptr;
	}
}

void KADIScriptInterface::InvokeToolInvokeCallback(int requestId, std::string const& toolName, nlohmann::json const& arguments)
{
	if (!m_v8Isolate || !m_jsToolInvokeCallback || !m_v8Context)
	{
		DebuggerPrintf("KADIScriptInterface: Cannot invoke tool callback - isolate, context, or callback not set\n");
		return;
	}

	// CRITICAL: Acquire V8 lock before ANY V8 API calls
	// Required for multi-threaded V8 access (Phase 1: Async Architecture)
	v8::Locker locker(m_v8Isolate);
	v8::Isolate::Scope isolateScope(m_v8Isolate);
	v8::HandleScope handleScope(m_v8Isolate);
	v8::Local<v8::Context> context = m_v8Context->Get(m_v8Isolate);
	v8::Context::Scope contextScope(context);

	// Get persistent callback as local
	v8::Local<v8::Function> callback = m_jsToolInvokeCallback->Get(m_v8Isolate);

	// Prepare arguments: requestId, toolName, argumentsJSON
	v8::Local<v8::Value> jsArgs[3];
	jsArgs[0] = v8::Number::New(m_v8Isolate, static_cast<double>(requestId));
	jsArgs[1] = v8::String::NewFromUtf8(m_v8Isolate, toolName.c_str()).ToLocalChecked();
	jsArgs[2] = v8::String::NewFromUtf8(m_v8Isolate, arguments.dump().c_str()).ToLocalChecked();

	// Call JavaScript callback
	v8::TryCatch tryCatch(m_v8Isolate);
	v8::Local<v8::Value> result;
	if (!callback->Call(context, context->Global(), 3, jsArgs).ToLocal(&result))
	{
		if (tryCatch.HasCaught())
		{
			v8::String::Utf8Value error(m_v8Isolate, tryCatch.Exception());
			DebuggerPrintf("KADIScriptInterface: Tool invoke callback error: %s\n", *error);
		}
	}
}

void KADIScriptInterface::InvokeEventDeliveryCallback(std::string const& channel, nlohmann::json const& data)
{
	if (!m_v8Isolate || !m_jsEventDeliveryCallback || !m_v8Context)
	{
		DebuggerPrintf("KADIScriptInterface: Cannot invoke event callback - isolate, context, or callback not set\n");
		return;
	}

	// CRITICAL: Acquire V8 lock before ANY V8 API calls
	// Required for multi-threaded V8 access (Phase 1: Async Architecture)
	v8::Locker locker(m_v8Isolate);
	v8::Isolate::Scope isolateScope(m_v8Isolate);
	v8::HandleScope handleScope(m_v8Isolate);
	v8::Local<v8::Context> context = m_v8Context->Get(m_v8Isolate);
	v8::Context::Scope contextScope(context);

	// Get persistent callback as local
	v8::Local<v8::Function> callback = m_jsEventDeliveryCallback->Get(m_v8Isolate);

	// Prepare arguments: channel, dataJSON
	v8::Local<v8::Value> jsArgs[2];
	jsArgs[0] = v8::String::NewFromUtf8(m_v8Isolate, channel.c_str()).ToLocalChecked();
	jsArgs[1] = v8::String::NewFromUtf8(m_v8Isolate, data.dump().c_str()).ToLocalChecked();

	// Call JavaScript callback
	v8::TryCatch tryCatch(m_v8Isolate);
	v8::Local<v8::Value> result;
	if (!callback->Call(context, context->Global(), 2, jsArgs).ToLocal(&result))
	{
		if (tryCatch.HasCaught())
		{
			v8::String::Utf8Value error(m_v8Isolate, tryCatch.Exception());
			DebuggerPrintf("KADIScriptInterface: Event delivery callback error: %s\n", *error);
		}
	}
}

void KADIScriptInterface::InvokeConnectionStateCallback(std::string const& oldState, std::string const& newState)
{
	if (!m_v8Isolate || !m_jsConnectionStateCallback || !m_v8Context)
	{
		DebuggerPrintf("KADIScriptInterface: Cannot invoke connection state callback - isolate, context, or callback not set\n");
		return;
	}

	// CRITICAL: Acquire V8 lock before ANY V8 API calls
	// Required for multi-threaded V8 access (Phase 1: Async Architecture)
	v8::Locker locker(m_v8Isolate);
	v8::Isolate::Scope isolateScope(m_v8Isolate);
	v8::HandleScope handleScope(m_v8Isolate);
	v8::Local<v8::Context> context = m_v8Context->Get(m_v8Isolate);
	v8::Context::Scope contextScope(context);

	// Get persistent callback as local
	v8::Local<v8::Function> callback = m_jsConnectionStateCallback->Get(m_v8Isolate);

	// Prepare arguments: oldState, newState
	v8::Local<v8::Value> jsArgs[2];
	jsArgs[0] = v8::String::NewFromUtf8(m_v8Isolate, oldState.c_str()).ToLocalChecked();
	jsArgs[1] = v8::String::NewFromUtf8(m_v8Isolate, newState.c_str()).ToLocalChecked();

	// Call JavaScript callback
	v8::TryCatch tryCatch(m_v8Isolate);
	v8::Local<v8::Value> result;
	if (!callback->Call(context, context->Global(), 2, jsArgs).ToLocal(&result))
	{
		if (tryCatch.HasCaught())
		{
			v8::String::Utf8Value error(m_v8Isolate, tryCatch.Exception());
			DebuggerPrintf("KADIScriptInterface: Connection state callback error: %s\n", *error);
		}
	}
}

//----------------------------------------------------------------------------------------------------
// Utility Functions
//----------------------------------------------------------------------------------------------------

std::string KADIScriptInterface::ConnectionStateToString(eKADIConnectionState state)
{
	switch (state)
	{
	case eKADIConnectionState::DISCONNECTED:      return "disconnected";
	case eKADIConnectionState::CONNECTING:        return "connecting";
	case eKADIConnectionState::CONNECTED:         return "connected";
	case eKADIConnectionState::AUTHENTICATING:    return "authenticating";
	case eKADIConnectionState::AUTHENTICATED:     return "authenticated";
	case eKADIConnectionState::REGISTERING_TOOLS: return "registering_tools";
	case eKADIConnectionState::READY:             return "ready";
	default:                                      return "unknown";
	}
}
