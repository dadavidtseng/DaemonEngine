//----------------------------------------------------------------------------------------------------
// CameraScriptInterface.cpp
// M4-T8: JavaScript Interface for Camera API Implementation
//----------------------------------------------------------------------------------------------------

#include "Engine/Renderer/CameraScriptInterface.hpp"
#include "Engine/Renderer/CameraAPI.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/LogSubsystem.hpp"
#include "Engine/Core/EngineCommon.hpp"

#include <unordered_map>

//----------------------------------------------------------------------------------------------------
// Construction / Destruction
//----------------------------------------------------------------------------------------------------

CameraScriptInterface::CameraScriptInterface(CameraAPI* cameraAPI)
	: m_cameraAPI(cameraAPI)
{
	GUARANTEE_OR_DIE(m_cameraAPI != nullptr, "CameraScriptInterface: CameraAPI is nullptr!");

	// CRITICAL: Initialize method registry so CallMethod() can find methods
	CameraScriptInterface::InitializeMethodRegistry();

	DebuggerPrintf("CameraScriptInterface: Initialized with %zu methods (M4-T8)\n", m_methodRegistry.size());
}

//----------------------------------------------------------------------------------------------------
// IScriptableObject Interface
//----------------------------------------------------------------------------------------------------

void CameraScriptInterface::InitializeMethodRegistry()
{
	// Camera management methods
	m_methodRegistry["create"]              = [this](ScriptArgs const& args) { return ExecuteCreateCamera(args); };
	m_methodRegistry["update"]              = [this](ScriptArgs const& args) { return ExecuteUpdateCamera(args); };  // RECOMMENDED: Atomic position+orientation update
	m_methodRegistry["updatePosition"]      = [this](ScriptArgs const& args) { return ExecuteUpdateCameraPosition(args); };  // DEPRECATED: Use update
	m_methodRegistry["updateOrientation"]   = [this](ScriptArgs const& args) { return ExecuteUpdateCameraOrientation(args); };  // DEPRECATED: Use update
	m_methodRegistry["moveBy"]              = [this](ScriptArgs const& args) { return ExecuteMoveCameraBy(args); };
	m_methodRegistry["lookAt"]              = [this](ScriptArgs const& args) { return ExecuteLookAtCamera(args); };
	m_methodRegistry["setActive"]           = [this](ScriptArgs const& args) { return ExecuteSetActiveCamera(args); };
	m_methodRegistry["updateType"]          = [this](ScriptArgs const& args) { return ExecuteUpdateCameraType(args); };
	m_methodRegistry["destroy"]             = [this](ScriptArgs const& args) { return ExecuteDestroyCamera(args); };
	m_methodRegistry["getHandle"]           = [this](ScriptArgs const& args) { return ExecuteGetCameraHandle(args); };
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult CameraScriptInterface::CallMethod(String const& methodName, ScriptArgs const& args)
{
	// Look up method in registry
	auto it = m_methodRegistry.find(methodName);
	if (it != m_methodRegistry.end())
	{
		// Execute method
		return it->second(args);
	}

	// Method not found
	return ScriptMethodResult::Error("CameraScriptInterface: Unknown method '" + methodName + "'");
}

//----------------------------------------------------------------------------------------------------
std::vector<ScriptMethodInfo> CameraScriptInterface::GetAvailableMethods() const
{
	return {
		ScriptMethodInfo("create",
		                 "Create a camera (async with callback)",
		                 {"number posX", "number posY", "number posZ", "number yaw", "number pitch", "number roll", "string type", "function callback"},
		                 "number callbackId"),
		ScriptMethodInfo("update",
		                 "RECOMMENDED: Update camera position AND orientation atomically (eliminates race conditions)",
		                 {"number cameraId", "number posX", "number posY", "number posZ", "number yaw", "number pitch", "number roll"},
		                 "void"),
		ScriptMethodInfo("updatePosition",
		                 "DEPRECATED: Move camera to absolute position (may cause race conditions, use update instead)",
		                 {"number cameraId", "number posX", "number posY", "number posZ"},
		                 "void"),
		ScriptMethodInfo("updateOrientation",
		                 "DEPRECATED: Update camera rotation (may cause race conditions, use update instead)",
		                 {"number cameraId", "number yaw", "number pitch", "number roll"},
		                 "void"),
		ScriptMethodInfo("moveBy",
		                 "Move camera by delta (relative, flattened API)",
		                 {"number cameraId", "number dx", "number dy", "number dz"},
		                 "void"),
		ScriptMethodInfo("lookAt",
		                 "Point camera at target position (flattened API)",
		                 {"number cameraId", "number targetX", "number targetY", "number targetZ"},
		                 "void"),
		ScriptMethodInfo("setActive",
		                 "Set active camera for rendering (async with callback)",
		                 {"number cameraId", "function callback"},
		                 "number callbackId"),
		ScriptMethodInfo("updateType",
		                 "Change camera type (async with callback)",
		                 {"number cameraId", "string type", "function callback"},
		                 "number callbackId"),
		ScriptMethodInfo("destroy",
		                 "Destroy camera (async with callback)",
		                 {"number cameraId", "function callback"},
		                 "number callbackId"),
		ScriptMethodInfo("getHandle",
		                 "Get camera pointer by ID (for debug rendering)",
		                 {"number cameraId"},
		                 "number cameraHandle")
	};
}

//----------------------------------------------------------------------------------------------------
std::vector<String> CameraScriptInterface::GetAvailableProperties() const
{
	// No properties exposed
	return {};
}

//----------------------------------------------------------------------------------------------------
std::any CameraScriptInterface::GetProperty(String const& propertyName) const
{
	(void)propertyName; // Unused
	// No properties
	return std::any{};
}

//----------------------------------------------------------------------------------------------------
bool CameraScriptInterface::SetProperty(String const& propertyName, std::any const& value)
{
	(void)propertyName; // Unused
	(void)value;        // Unused
	// No properties
	return false;
}

//----------------------------------------------------------------------------------------------------
// Camera Management Methods
//----------------------------------------------------------------------------------------------------

ScriptMethodResult CameraScriptInterface::ExecuteCreateCamera(ScriptArgs const& args)
{
	DAEMON_LOG(LogScript, eLogVerbosity::Log,
		Stringf("[CALLBACK FLOW] CameraScriptInterface::ExecuteCreateCamera - ENTRY with %zu args", args.size()));

	// FLATTENED API: V8 binding cannot handle nested objects
	// Signature: create(posX, posY, posZ, yaw, pitch, roll, type, callback)
	// Total: 8 arguments (6 doubles + 1 string + 1 function)
	if (args.size() != 8)
	{
		DAEMON_LOG(LogScript, eLogVerbosity::Error,
			Stringf("[CALLBACK FLOW] CameraScriptInterface::ExecuteCreateCamera - WRONG ARG COUNT: expected 8, got %zu", args.size()));
		return ScriptMethodResult::Error("camera.create: Expected 8 arguments (posX, posY, posZ, yaw, pitch, roll, type, callback), got " +
		                                  std::to_string(args.size()));
	}

	try
	{
		// Extract position components (3 doubles)
		double posX = std::any_cast<double>(args[0]);
		double posY = std::any_cast<double>(args[1]);
		double posZ = std::any_cast<double>(args[2]);
		Vec3 position(static_cast<float>(posX), static_cast<float>(posY), static_cast<float>(posZ));

		// Extract orientation components (3 doubles)
		double yaw = std::any_cast<double>(args[3]);
		double pitch = std::any_cast<double>(args[4]);
		double roll = std::any_cast<double>(args[5]);
		EulerAngles orientation(static_cast<float>(yaw), static_cast<float>(pitch), static_cast<float>(roll));

		// Extract type (string)
		std::string type = std::any_cast<std::string>(args[6]);

		DAEMON_LOG(LogScript, eLogVerbosity::Log,
			Stringf("[CALLBACK FLOW] CameraScriptInterface::ExecuteCreateCamera - Calling CameraAPI::CreateCamera (pos=[%.2f,%.2f,%.2f], type=%s)",
				posX, posY, posZ, type.c_str()));

		// Extract callback (function)
		auto callbackOpt = ExtractCallback(args[7]);
		if (!callbackOpt.has_value())
		{
			DAEMON_LOG(LogScript, eLogVerbosity::Error,
				Stringf("[CALLBACK FLOW] CameraScriptInterface::ExecuteCreateCamera - Invalid callback function"));
			return ScriptMethodResult::Error("camera.create: Invalid callback function");
		}

		// Call CameraAPI
		CallbackID callbackId = m_cameraAPI->CreateCamera(position,
		                                                   orientation,
		                                                   type,
		                                                   callbackOpt.value());

		DAEMON_LOG(LogScript, eLogVerbosity::Log,
			Stringf("[CALLBACK FLOW] CameraScriptInterface::ExecuteCreateCamera - CameraAPI returned callbackId=%llu", callbackId));

		// Return callback ID as double (JavaScript numbers are IEEE-754 doubles)
		// V8 cannot directly marshal uint64_t, so we explicitly cast to double
		double callbackIdDouble = static_cast<double>(callbackId);
		return ScriptMethodResult::Success(callbackIdDouble);
	}
	catch (std::bad_any_cast const& e)
	{
		DAEMON_LOG(LogScript, eLogVerbosity::Error,
			Stringf("[CALLBACK FLOW] CameraScriptInterface::ExecuteCreateCamera - Type conversion error: %s", e.what()));
		return ScriptMethodResult::Error("camera.create: Type conversion error - " + std::string(e.what()));
	}
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult CameraScriptInterface::ExecuteUpdateCamera(ScriptArgs const& args)
{
	// FLATTENED API: V8 cannot handle nested objects, expect individual primitive arguments
	// Signature: update(cameraId, posX, posY, posZ, yaw, pitch, roll)
	// Validate argument count
	if (args.size() != 7)
	{
		return ScriptMethodResult::Error("camera.update: Expected 7 arguments (cameraId, posX, posY, posZ, yaw, pitch, roll), got " +
		                                  std::to_string(args.size()));
	}

	try
	{
		// Extract camera ID
		auto cameraIdOpt = ExtractEntityID(args[0]);
		if (!cameraIdOpt.has_value())
		{
			return ScriptMethodResult::Error("camera.update: Invalid cameraId");
		}

		// Extract position components (flattened - doubles from JavaScript)
		double posX = std::any_cast<double>(args[1]);
		double posY = std::any_cast<double>(args[2]);
		double posZ = std::any_cast<double>(args[3]);

		Vec3 position(static_cast<float>(posX),
		              static_cast<float>(posY),
		              static_cast<float>(posZ));

		// Extract orientation components (flattened - doubles from JavaScript)
		double yaw   = std::any_cast<double>(args[4]);
		double pitch = std::any_cast<double>(args[5]);
		double roll  = std::any_cast<double>(args[6]);

		EulerAngles orientation(static_cast<float>(yaw),
		                        static_cast<float>(pitch),
		                        static_cast<float>(roll));

		// Call CameraAPI with combined update
		m_cameraAPI->UpdateCamera(cameraIdOpt.value(), position, orientation);

		return ScriptMethodResult::Success();
	}
	catch (std::bad_any_cast const& e)
	{
		return ScriptMethodResult::Error("camera.update: Type conversion error - " + std::string(e.what()));
	}
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult CameraScriptInterface::ExecuteUpdateCameraPosition(ScriptArgs const& args)
{
	// FLATTENED API: V8 cannot handle nested objects, expect individual primitive arguments
	// Signature: updatePosition(cameraId, posX, posY, posZ)
	// Validate argument count
	if (args.size() != 4)
	{
		return ScriptMethodResult::Error("camera.updatePosition: Expected 4 arguments (cameraId, posX, posY, posZ), got " +
		                                  std::to_string(args.size()));
	}

	try
	{
		// Extract camera ID
		auto cameraIdOpt = ExtractEntityID(args[0]);
		if (!cameraIdOpt.has_value())
		{
			return ScriptMethodResult::Error("camera.updatePosition: Invalid cameraId");
		}

		// Extract position components (flattened - doubles from JavaScript)
		double posX = std::any_cast<double>(args[1]);
		double posY = std::any_cast<double>(args[2]);
		double posZ = std::any_cast<double>(args[3]);

		Vec3 position(static_cast<float>(posX),
		              static_cast<float>(posY),
		              static_cast<float>(posZ));

		// Call CameraAPI
		m_cameraAPI->UpdateCameraPosition(cameraIdOpt.value(), position);

		return ScriptMethodResult::Success();
	}
	catch (std::bad_any_cast const& e)
	{
		return ScriptMethodResult::Error("camera.updatePosition: Type conversion error - " + std::string(e.what()));
	}
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult CameraScriptInterface::ExecuteUpdateCameraOrientation(ScriptArgs const& args)
{
	// FLATTENED API: V8 cannot handle nested objects, expect individual primitive arguments
	// Signature: updateOrientation(cameraId, yaw, pitch, roll)
	// Validate argument count
	if (args.size() != 4)
	{
		return ScriptMethodResult::Error("camera.updateOrientation: Expected 4 arguments (cameraId, yaw, pitch, roll), got " +
		                                  std::to_string(args.size()));
	}

	try
	{
		// Extract camera ID
		auto cameraIdOpt = ExtractEntityID(args[0]);
		if (!cameraIdOpt.has_value())
		{
			return ScriptMethodResult::Error("camera.updateOrientation: Invalid cameraId");
		}

		// Extract orientation components (flattened - doubles from JavaScript)
		double yaw   = std::any_cast<double>(args[1]);
		double pitch = std::any_cast<double>(args[2]);
		double roll  = std::any_cast<double>(args[3]);

		EulerAngles orientation(static_cast<float>(yaw),
		                        static_cast<float>(pitch),
		                        static_cast<float>(roll));

		// Call CameraAPI
		m_cameraAPI->UpdateCameraOrientation(cameraIdOpt.value(), orientation);

		return ScriptMethodResult::Success();
	}
	catch (std::bad_any_cast const& e)
	{
		return ScriptMethodResult::Error("camera.updateOrientation: Type conversion error - " + std::string(e.what()));
	}
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult CameraScriptInterface::ExecuteMoveCameraBy(ScriptArgs const& args)
{
	// FLATTENED API: V8 cannot handle nested objects, expect individual primitive arguments
	// Signature: moveBy(cameraId, dx, dy, dz)
	// Validate argument count
	if (args.size() != 4)
	{
		return ScriptMethodResult::Error("camera.moveBy: Expected 4 arguments (cameraId, dx, dy, dz), got " +
		                                  std::to_string(args.size()));
	}

	try
	{
		// Extract camera ID
		auto cameraIdOpt = ExtractEntityID(args[0]);
		if (!cameraIdOpt.has_value())
		{
			return ScriptMethodResult::Error("camera.moveBy: Invalid cameraId");
		}

		// Extract delta components (flattened - doubles from JavaScript)
		double dx = std::any_cast<double>(args[1]);
		double dy = std::any_cast<double>(args[2]);
		double dz = std::any_cast<double>(args[3]);

		Vec3 delta(static_cast<float>(dx),
		           static_cast<float>(dy),
		           static_cast<float>(dz));

		// Call CameraAPI
		m_cameraAPI->MoveCameraBy(cameraIdOpt.value(), delta);

		return ScriptMethodResult::Success();
	}
	catch (std::bad_any_cast const& e)
	{
		return ScriptMethodResult::Error("camera.moveBy: Type conversion error - " + std::string(e.what()));
	}
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult CameraScriptInterface::ExecuteLookAtCamera(ScriptArgs const& args)
{
	// FLATTENED API: V8 cannot handle nested objects, expect individual primitive arguments
	// Signature: lookAt(cameraId, targetX, targetY, targetZ)
	// Validate argument count
	if (args.size() != 4)
	{
		return ScriptMethodResult::Error("camera.lookAt: Expected 4 arguments (cameraId, targetX, targetY, targetZ), got " +
		                                  std::to_string(args.size()));
	}

	try
	{
		// Extract camera ID
		auto cameraIdOpt = ExtractEntityID(args[0]);
		if (!cameraIdOpt.has_value())
		{
			return ScriptMethodResult::Error("camera.lookAt: Invalid cameraId");
		}

		// Extract target components (flattened - doubles from JavaScript)
		double targetX = std::any_cast<double>(args[1]);
		double targetY = std::any_cast<double>(args[2]);
		double targetZ = std::any_cast<double>(args[3]);

		Vec3 target(static_cast<float>(targetX),
		            static_cast<float>(targetY),
		            static_cast<float>(targetZ));

		// Call CameraAPI
		m_cameraAPI->LookAtCamera(cameraIdOpt.value(), target);

		return ScriptMethodResult::Success();
	}
	catch (std::bad_any_cast const& e)
	{
		return ScriptMethodResult::Error("camera.lookAt: Type conversion error - " + std::string(e.what()));
	}
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult CameraScriptInterface::ExecuteSetActiveCamera(ScriptArgs const& args)
{
	// Validate argument count
	if (args.size() != 2)
	{
		return ScriptMethodResult::Error("camera.setActive: Expected 2 arguments (cameraId, callback), got " +
		                                  std::to_string(args.size()));
	}

	try
	{
		// Extract camera ID
		auto cameraIdOpt = ExtractEntityID(args[0]);
		if (!cameraIdOpt.has_value())
		{
			return ScriptMethodResult::Error("camera.setActive: Invalid cameraId");
		}

		// Extract callback
		auto callbackOpt = ExtractCallback(args[1]);
		if (!callbackOpt.has_value())
		{
			return ScriptMethodResult::Error("camera.setActive: Invalid callback function");
		}

		// Call CameraAPI
		CallbackID callbackId = m_cameraAPI->SetActiveCamera(cameraIdOpt.value(), callbackOpt.value());

		// Return callback ID as double
		double callbackIdDouble = static_cast<double>(callbackId);
		return ScriptMethodResult::Success(callbackIdDouble);
	}
	catch (std::bad_any_cast const& e)
	{
		return ScriptMethodResult::Error("camera.setActive: Type conversion error - " + std::string(e.what()));
	}
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult CameraScriptInterface::ExecuteUpdateCameraType(ScriptArgs const& args)
{
	// Validate argument count
	if (args.size() != 3)
	{
		return ScriptMethodResult::Error("camera.updateType: Expected 3 arguments (cameraId, type, callback), got " +
		                                  std::to_string(args.size()));
	}

	try
	{
		// Extract camera ID
		auto cameraIdOpt = ExtractEntityID(args[0]);
		if (!cameraIdOpt.has_value())
		{
			return ScriptMethodResult::Error("camera.updateType: Invalid cameraId");
		}

		// Extract type string
		std::string type = std::any_cast<std::string>(args[1]);

		// Extract callback
		auto callbackOpt = ExtractCallback(args[2]);
		if (!callbackOpt.has_value())
		{
			return ScriptMethodResult::Error("camera.updateType: Invalid callback function");
		}

		// Call CameraAPI
		CallbackID callbackId = m_cameraAPI->UpdateCameraType(cameraIdOpt.value(), type, callbackOpt.value());

		// Return callback ID as double
		double callbackIdDouble = static_cast<double>(callbackId);
		return ScriptMethodResult::Success(callbackIdDouble);
	}
	catch (std::bad_any_cast const& e)
	{
		return ScriptMethodResult::Error("camera.updateType: Type conversion error - " + std::string(e.what()));
	}
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult CameraScriptInterface::ExecuteDestroyCamera(ScriptArgs const& args)
{
	// Validate argument count
	if (args.size() != 2)
	{
		return ScriptMethodResult::Error("camera.destroy: Expected 2 arguments (cameraId, callback), got " +
		                                  std::to_string(args.size()));
	}

	try
	{
		// Extract camera ID
		auto cameraIdOpt = ExtractEntityID(args[0]);
		if (!cameraIdOpt.has_value())
		{
			return ScriptMethodResult::Error("camera.destroy: Invalid cameraId");
		}

		// Extract callback
		auto callbackOpt = ExtractCallback(args[1]);
		if (!callbackOpt.has_value())
		{
			return ScriptMethodResult::Error("camera.destroy: Invalid callback function");
		}

		// Call CameraAPI
		CallbackID callbackId = m_cameraAPI->DestroyCamera(cameraIdOpt.value(), callbackOpt.value());

		// Return callback ID as double
		double callbackIdDouble = static_cast<double>(callbackId);
		return ScriptMethodResult::Success(callbackIdDouble);
	}
	catch (std::bad_any_cast const& e)
	{
		return ScriptMethodResult::Error("camera.destroy: Type conversion error - " + std::string(e.what()));
	}
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult CameraScriptInterface::ExecuteGetCameraHandle(ScriptArgs const& args)
{
	// Validate argument count
	if (args.size() != 1)
	{
		return ScriptMethodResult::Error("camera.getHandle: Expected 1 argument (cameraId), got " +
		                                  std::to_string(args.size()));
	}

	try
	{
		// Extract camera ID
		auto cameraIdOpt = ExtractEntityID(args[0]);
		if (!cameraIdOpt.has_value())
		{
			return ScriptMethodResult::Error("camera.getHandle: Invalid cameraId");
		}

		// Call CameraAPI to get camera handle
		uintptr_t cameraHandle = m_cameraAPI->GetCameraHandle(cameraIdOpt.value());

		// Return camera handle as double (JavaScript number type)
		double cameraHandleDouble = static_cast<double>(cameraHandle);
		return ScriptMethodResult::Success(cameraHandleDouble);
	}
	catch (std::bad_any_cast const& e)
	{
		return ScriptMethodResult::Error("camera.getHandle: Type conversion error - " + std::string(e.what()));
	}
}

//----------------------------------------------------------------------------------------------------
// Helper Methods
//----------------------------------------------------------------------------------------------------

std::optional<EntityID> CameraScriptInterface::ExtractEntityID(std::any const& value) const
{
	try
	{
		// JavaScript numbers are doubles, convert to uint64_t
		double idDouble = std::any_cast<double>(value);
		return static_cast<EntityID>(idDouble);
	}
	catch (...)
	{
		return std::nullopt;
	}
}

//----------------------------------------------------------------------------------------------------
std::optional<ScriptCallback> CameraScriptInterface::ExtractCallback(std::any const& value) const
{
	// Callback is already a std::any, just return it as-is
	// V8Subsystem will handle conversion to v8::Function when executing
	return value;
}

//----------------------------------------------------------------------------------------------------
// Design Notes
//
// M4-T8 API Splitting Implementation:
//   - Extracted all camera methods from EntityScriptInterface
//   - Maintains identical signature patterns for consistency
//   - Uses CameraAPI instead of HighLevelEntityAPI
//   - Registered as "camera" global in JavaScript (separate from "entity")
//
// Error Handling Strategy:
//   - All methods wrapped in try-catch to handle std::bad_any_cast
//   - Helper methods return std::nullopt on extraction failure
//   - Main methods return ScriptMethodResult::Error() with descriptive message
//   - No C++ crashes on invalid JavaScript input
//
// Type Conversion:
//   - JavaScript numbers → double → float/uint64_t
//   - JavaScript strings → std::string
//   - JavaScript functions → std::any (opaque, handled by CameraAPI)
//
// Performance Considerations:
//   - std::unordered_map lookups for method registry (O(1) average)
//   - Minimal copying (pass by const reference where possible)
//   - No allocations in hot path (command submission)
//----------------------------------------------------------------------------------------------------
