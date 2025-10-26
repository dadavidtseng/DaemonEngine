//----------------------------------------------------------------------------------------------------
// CameraScriptInterface.hpp
// M4-T8: JavaScript Interface for Camera API
//
// Purpose:
//   Exposes CameraAPI to JavaScript runtime through IScriptableObject interface.
//   Provides user-friendly JavaScript APIs for camera management.
//
// Design Philosophy:
//   - Single Responsibility: Camera operations only (extracted from EntityScriptInterface)
//   - Clean separation from entity operations
//   - Error-resilient (JavaScript errors don't crash C++ rendering)
//   - Async callbacks for creation operations
//
// JavaScript API (exposed methods):
//   Camera Management:
//     - camera.create(posX, posY, posZ, yaw, pitch, roll, type, callback)
//     - camera.update(cameraId, posX, posY, posZ, yaw, pitch, roll)
//     - camera.updatePosition(cameraId, posX, posY, posZ)  // DEPRECATED
//     - camera.updateOrientation(cameraId, yaw, pitch, roll)  // DEPRECATED
//     - camera.moveBy(cameraId, dx, dy, dz)
//     - camera.lookAt(cameraId, targetX, targetY, targetZ)
//     - camera.setActive(cameraId, callback)
//     - camera.updateType(cameraId, type, callback)
//     - camera.destroy(cameraId, callback)
//     - camera.getHandle(cameraId)
//
// Usage Example (from JavaScript):
//   // Create a camera
//   camera.create(
//       -10, 0, 5,    // position (X-forward, Y-left, Z-up)
//       0, 0, 0,      // orientation (yaw, pitch, roll)
//       'world',      // type
//       (cameraId) => {
//           console.log('Camera created:', cameraId);
//           // Set as active camera
//           camera.setActive(cameraId, () => {
//               console.log('Camera activated');
//           });
//       }
//   );
//
//   // Update camera (atomic position + orientation)
//   camera.update(cameraId, -15, 0, 10, 45, 0, 0);
//
// Thread Safety:
//   - All methods submit commands to RenderCommandQueue (lock-free)
//   - Callbacks executed on JavaScript worker thread
//   - V8 locking handled internally by CameraAPI::ExecutePendingCallbacks()
//
// Author: M4-T8 Engine Refactoring
// Date: 2025-10-26
//----------------------------------------------------------------------------------------------------

#pragma once

//----------------------------------------------------------------------------------------------------
#include "Engine/Script/IScriptableObject.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Entity/EntityID.hpp"

#include <memory>
#include <optional>
#include <any>

//----------------------------------------------------------------------------------------------------
// Forward Declarations
class CameraAPI;

// ScriptCallback is defined as std::any in CameraAPI.hpp
using ScriptCallback = std::any;

//----------------------------------------------------------------------------------------------------
// CameraScriptInterface
//
// JavaScript interface for camera management.
// Wraps CameraAPI and exposes methods to V8 JavaScript runtime.
//
// Registration:
//   - Registered in ScriptSubsystem as "camera" global object
//   - Separate from EntityScriptInterface (which manages entities)
//
// Method Naming Convention:
//   - JavaScript methods use camelCase (e.g., create, moveBy, setActive)
//   - C++ methods map to CameraAPI (e.g., CreateCamera, MoveCameraBy, SetActiveCamera)
//
// Error Handling:
//   - Invalid parameters return ScriptMethodResult::Error()
//   - Errors logged to console, don't crash C++ rendering
//   - Callbacks with error status notify JavaScript of failures
//----------------------------------------------------------------------------------------------------
class CameraScriptInterface : public IScriptableObject
{
public:
	//------------------------------------------------------------------------------------------------
	// Construction / Destruction
	//------------------------------------------------------------------------------------------------
	explicit CameraScriptInterface(CameraAPI* cameraAPI);
	~CameraScriptInterface() override = default;

	//------------------------------------------------------------------------------------------------
	// IScriptableObject Interface
	//------------------------------------------------------------------------------------------------
	void                          InitializeMethodRegistry() override;
	ScriptMethodResult            CallMethod(String const& methodName, ScriptArgs const& args) override;
	std::vector<ScriptMethodInfo> GetAvailableMethods() const override;
	std::vector<String>           GetAvailableProperties() const override;
	std::any                      GetProperty(String const& propertyName) const override;
	bool                          SetProperty(String const& propertyName, std::any const& value) override;

private:
	//------------------------------------------------------------------------------------------------
	// Camera Management Methods (exposed to JavaScript)
	//------------------------------------------------------------------------------------------------

	// Create a camera (async with callback)
	// JavaScript signature: create(posX, posY, posZ, yaw, pitch, roll, type, callback)
	// Parameters:
	//   - posX, posY, posZ: double (camera position)
	//   - yaw, pitch, roll: double (camera orientation in degrees)
	//   - type: string ("world" or "screen")
	//   - callback: function (cameraId) => {...}
	ScriptMethodResult ExecuteCreateCamera(ScriptArgs const& args);

	// RECOMMENDED: Update camera position AND orientation atomically (eliminates race conditions)
	// JavaScript signature: update(cameraId, posX, posY, posZ, yaw, pitch, roll)
	// Parameters:
	//   - cameraId: number
	//   - posX, posY, posZ: number (position components)
	//   - yaw, pitch, roll: number (orientation components in degrees)
	ScriptMethodResult ExecuteUpdateCamera(ScriptArgs const& args);

	// DEPRECATED: Move camera to absolute position (use ExecuteUpdateCamera instead)
	// JavaScript signature: updatePosition(cameraId, posX, posY, posZ)
	// Parameters:
	//   - cameraId: number
	//   - posX, posY, posZ: number (position components)
	ScriptMethodResult ExecuteUpdateCameraPosition(ScriptArgs const& args);

	// DEPRECATED: Update camera orientation only (use ExecuteUpdateCamera instead)
	// JavaScript signature: updateOrientation(cameraId, yaw, pitch, roll)
	// Parameters:
	//   - cameraId: number
	//   - yaw, pitch, roll: number (orientation components in degrees)
	ScriptMethodResult ExecuteUpdateCameraOrientation(ScriptArgs const& args);

	// Move camera by delta (relative movement)
	// JavaScript signature: moveBy(cameraId, dx, dy, dz)
	// Parameters:
	//   - cameraId: number
	//   - dx, dy, dz: number (delta components)
	ScriptMethodResult ExecuteMoveCameraBy(ScriptArgs const& args);

	// Point camera at target position
	// JavaScript signature: lookAt(cameraId, targetX, targetY, targetZ)
	// Parameters:
	//   - cameraId: number
	//   - targetX, targetY, targetZ: number (target position components)
	ScriptMethodResult ExecuteLookAtCamera(ScriptArgs const& args);

	// Set active camera for rendering (async with callback)
	// JavaScript signature: setActive(cameraId, callback)
	// Parameters:
	//   - cameraId: number
	//   - callback: function
	ScriptMethodResult ExecuteSetActiveCamera(ScriptArgs const& args);

	// Update camera type (async with callback)
	// JavaScript signature: updateType(cameraId, type, callback)
	// Parameters:
	//   - cameraId: number
	//   - type: string ("world" or "screen")
	//   - callback: function
	ScriptMethodResult ExecuteUpdateCameraType(ScriptArgs const& args);

	// Destroy camera (async with callback)
	// JavaScript signature: destroy(cameraId, callback)
	// Parameters:
	//   - cameraId: number
	//   - callback: function
	ScriptMethodResult ExecuteDestroyCamera(ScriptArgs const& args);

	// Get camera handle by ID (for debug rendering)
	// JavaScript signature: getHandle(cameraId)
	// Parameters:
	//   - cameraId: number
	// Returns: Camera pointer as number (uintptr_t), or 0 if not found
	ScriptMethodResult ExecuteGetCameraHandle(ScriptArgs const& args);

	//------------------------------------------------------------------------------------------------
	// Helper Methods
	//------------------------------------------------------------------------------------------------

	// Extract EntityID (uint64_t) from JavaScript number
	// Returns std::nullopt if extraction fails
	std::optional<EntityID> ExtractEntityID(std::any const& value) const;

	// Extract callback function from std::any
	// Returns std::nullopt if extraction fails
	std::optional<ScriptCallback> ExtractCallback(std::any const& value) const;

private:
	//------------------------------------------------------------------------------------------------
	// Internal State
	//------------------------------------------------------------------------------------------------
	CameraAPI* m_cameraAPI;  // Pointer to camera API (owned by HighLevelEntityAPI)
};

//----------------------------------------------------------------------------------------------------
// Design Notes
//
// M4-T8 API Splitting:
//   - Extracted from EntityScriptInterface for Single Responsibility Principle
//   - Camera operations logically distinct from entity operations
//   - Registered as separate "camera" global in JavaScript
//
// Method Naming Convention:
//   - JavaScript uses camelCase: create, update, setActive
//   - C++ uses PascalCase: ExecuteCreateCamera, ExecuteUpdateCamera, ExecuteSetActiveCamera
//
// Parameter Extraction Strategy:
//   - FLATTENED API: V8 binding cannot handle nested JavaScript objects
//   - All parameters passed as individual primitive values (double, string, function)
//   - Helper methods (ExtractEntityID, ExtractCallback) validate and extract types
//   - Extraction failures return ScriptMethodResult::Error() with descriptive message
//
// Callback Handling:
//   - Callbacks stored in CameraAPI::m_pendingCallbacks
//   - Executed by CameraAPI::ExecutePendingCallbacks() on worker thread
//   - V8 locking handled by CameraAPI (not this interface)
//
// Error Resilience:
//   - All parameter extraction wrapped in try-catch (std::bad_any_cast protection)
//   - Invalid parameters → ScriptMethodResult::Error() → JavaScript receives error
//   - C++ rendering continues regardless of JavaScript errors
//
// Coordinate System:
//   - X-forward, Y-left, Z-up (right-handed)
//   - +X = forward, +Y = left, +Z = up
//   - All positions/deltas use this convention
//----------------------------------------------------------------------------------------------------
