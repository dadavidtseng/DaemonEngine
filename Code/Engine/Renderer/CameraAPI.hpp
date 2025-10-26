//----------------------------------------------------------------------------------------------------
// CameraAPI.hpp
// M4-T8: Engine Refactoring - Camera Management API
//
// Purpose:
//   Provides high-level camera management API for JavaScript integration.
//   Extracted from HighLevelEntityAPI to separate camera concerns from entity concerns.
//
// Design Philosophy:
//   - Single Responsibility: Camera-specific operations only
//   - Async callbacks for creation operations (error resilience)
//   - Thread-safe command submission to RenderCommandQueue
//   - JavaScript errors must NOT crash C++ rendering
//
// API Surface:
//   Camera Creation/Destruction:
//     - CreateCamera(position, orientation, type, callback) - Async, returns cameraId via callback
//     - DestroyCamera(cameraId, callback) - Async with callback
//
//   Camera Updates:
//     - UpdateCamera(cameraId, position, orientation) - RECOMMENDED: Atomic update
//     - UpdateCameraPosition(cameraId, position) - DEPRECATED: Partial update (race condition risk)
//     - UpdateCameraOrientation(cameraId, orientation) - DEPRECATED: Partial update (race condition risk)
//     - MoveCameraBy(cameraId, delta) - Relative movement
//     - LookAtCamera(cameraId, target) - Point camera at target
//
//   Camera State:
//     - SetActiveCamera(cameraId, callback) - Set rendering camera
//     - UpdateCameraType(cameraId, type, callback) - Change camera mode
//     - GetCameraHandle(cameraId) - Get camera pointer for debug rendering
//
// Coordinate System:
//   X-forward, Y-left, Z-up (right-handed)
//   +X = forward, +Y = left, +Z = up
//
// Thread Safety:
//   - Methods submit RenderCommands to RenderCommandQueue (lock-free)
//   - Callbacks executed on JavaScript worker thread (V8 isolation required)
//   - C++ rendering continues even if JavaScript callbacks throw errors
//
// Author: M4-T8 Engine Refactoring
// Date: 2025-10-26
//----------------------------------------------------------------------------------------------------

#pragma once

//----------------------------------------------------------------------------------------------------
#include "Engine/Renderer/RenderCommand.hpp"
#include "Engine/Script/ScriptCommon.hpp"
#include "Engine/Entity/EntityID.hpp"

#include <any>
#include <functional>
#include <unordered_map>

//----------------------------------------------------------------------------------------------------
// Forward Declarations
class RenderCommandQueue;
class ScriptSubsystem;
class CameraStateBuffer;

//----------------------------------------------------------------------------------------------------
// CallbackID Type Definition (shared with EntityAPI)
using CallbackID = uint64_t;

//----------------------------------------------------------------------------------------------------
// ScriptCallback Type Definition (shared with EntityAPI)
using ScriptCallback = std::any;

//----------------------------------------------------------------------------------------------------
// CameraAPI
//
// High-level camera management API for JavaScript integration.
// Handles camera creation, updates, and state management through render command queue.
//
// Usage Pattern (from JavaScript):
//
// Camera Creation (Async):
//   camera.create({
//       position: {x: -10, y: 0, z: 5},   // 10 units back, 5 units up
//       orientation: {yaw: 0, pitch: 0, roll: 0},
//       type: 'world'  // or 'screen'
//   }, (cameraId) => {
//       console.log('Camera created:', cameraId);
//   });
//
// Camera Update (Sync):
//   camera.update(cameraId, {x: -5, y: 0, z: 3}, {yaw: 45, pitch: 0, roll: 0});
//   camera.moveBy(cameraId, {dx: 1, dy: 0, dz: 0});  // Relative (+X = forward)
//   camera.lookAt(cameraId, {x: 0, y: 0, z: 0});     // Point at target
//
// Error Resilience:
//   - JavaScript callback errors are caught and logged
//   - C++ rendering continues with last valid state
//   - Invalid cameraIds are ignored with warning logs
//----------------------------------------------------------------------------------------------------
class CameraAPI
{
public:
	//------------------------------------------------------------------------------------------------
	// Construction / Destruction
	//------------------------------------------------------------------------------------------------
	explicit CameraAPI(RenderCommandQueue* commandQueue,
	                   ScriptSubsystem* scriptSubsystem,
	                   CameraStateBuffer* cameraBuffer);
	~CameraAPI();

	// Non-copyable (manages callback state)
	CameraAPI(CameraAPI const&)            = delete;
	CameraAPI& operator=(CameraAPI const&) = delete;

	//------------------------------------------------------------------------------------------------
	// Camera Creation/Destruction
	//------------------------------------------------------------------------------------------------

	// Create camera with specified properties (async, returns cameraId via callback)
	// Parameters:
	//   - position: {x, y, z} world-space position (X-forward, Y-left, Z-up)
	//   - orientation: {yaw, pitch, roll} in degrees
	//   - type: "world" (3D perspective) or "screen" (2D orthographic)
	//   - callback: JavaScript function (cameraId) => {...}
	// Returns: CallbackID (for internal tracking)
	// FOV, aspect ratio, near/far planes auto-configured based on type
	CallbackID CreateCamera(Vec3 const& position,
	                        EulerAngles const& orientation,
	                        std::string const& type,
	                        ScriptCallback const& callback);

	// Destroy camera (async with callback)
	CallbackID DestroyCamera(EntityID cameraId, ScriptCallback const& callback);

	//------------------------------------------------------------------------------------------------
	// Camera Updates
	//------------------------------------------------------------------------------------------------

	// RECOMMENDED: Update camera position AND orientation atomically (eliminates race conditions)
	// This is the preferred method - sends both position and orientation in a single command
	// No callback - command queued and processed asynchronously
	void UpdateCamera(EntityID cameraId, Vec3 const& position, EulerAngles const& orientation);

	// DEPRECATED: Update camera position only (may cause race conditions with orientation updates)
	// Use UpdateCamera() instead for atomic updates
	// No callback - command queued and processed asynchronously
	void UpdateCameraPosition(EntityID cameraId, Vec3 const& position);

	// DEPRECATED: Update camera orientation only (may cause race conditions with position updates)
	// Use UpdateCamera() instead for atomic updates
	// No callback - command queued and processed asynchronously
	void UpdateCameraOrientation(EntityID cameraId, EulerAngles const& orientation);

	// Move camera by delta (relative movement, fire-and-forget)
	// Delta convention: +X = forward, +Y = left, +Z = up
	// No callback - command queued and processed asynchronously
	void MoveCameraBy(EntityID cameraId, Vec3 const& delta);

	// Point camera at target position (calculates orientation, fire-and-forget)
	// Target: world-space position to look at
	// No callback - command queued and processed asynchronously
	void LookAtCamera(EntityID cameraId, Vec3 const& target);

	//------------------------------------------------------------------------------------------------
	// Camera State Management
	//------------------------------------------------------------------------------------------------

	// Set active camera (async with callback - rendering switches immediately)
	// Callback notifies JavaScript when camera switch is confirmed
	CallbackID SetActiveCamera(EntityID cameraId, ScriptCallback const& callback);

	// Update camera type (async with callback - requires reconfiguring FOV/etc)
	// Type: "world" (3D perspective) or "screen" (2D orthographic)
	// Callback notifies when reconfiguration is complete
	CallbackID UpdateCameraType(EntityID cameraId, std::string const& type, ScriptCallback const& callback);

	// Get camera pointer by ID (for debug rendering)
	// Returns: Camera pointer as uintptr_t (to pass to JavaScript as number)
	// Returns 0 if camera not found
	// Note: Pointer valid until next SwapBuffers() call
	uintptr_t GetCameraHandle(EntityID cameraId) const;

	//------------------------------------------------------------------------------------------------
	// Callback Execution (called by HighLevelEntityAPI/main thread)
	//------------------------------------------------------------------------------------------------

	// Execute pending callbacks with results
	// Called by App::Update() after processing render commands
	// Executes callbacks on JavaScript worker thread with V8 locking
	void ExecutePendingCallbacks();

	// Register a callback completion (called by command processor)
	void NotifyCallbackReady(CallbackID callbackId, EntityID resultId);

	//------------------------------------------------------------------------------------------------
	// ID Generation (for HighLevelEntityAPI coordination)
	//------------------------------------------------------------------------------------------------

	// Generate next camera ID
	EntityID GenerateCameraID();

	// Generate next callback ID
	CallbackID GenerateCallbackID();

private:
	//------------------------------------------------------------------------------------------------
	// Internal State
	//------------------------------------------------------------------------------------------------

	RenderCommandQueue* m_commandQueue;     // Queue for submitting render commands
	ScriptSubsystem*    m_scriptSubsystem;  // Script subsystem for callback execution
	CameraStateBuffer*  m_cameraBuffer;     // Camera state buffer for camera lookups

	// Camera ID generation
	EntityID m_nextCameraId;   // Auto-incremented camera ID counter (starts at 1000)

	// Callback ID generation
	CallbackID m_nextCallbackId;  // Auto-incremented callback ID counter

	// Callback storage (CallbackID → {ScriptFunction, resultId})
	struct PendingCallback
	{
		ScriptCallback callback;
		EntityID       resultId;
		bool           ready;  // True when C++ has processed command and resultId is available
	};
	std::unordered_map<CallbackID, PendingCallback> m_pendingCallbacks;

	//------------------------------------------------------------------------------------------------
	// Helper Methods
	//------------------------------------------------------------------------------------------------

	// Submit render command to queue (with error handling)
	bool SubmitCommand(RenderCommand const& command);

	// Execute a single callback (with error handling to prevent C++ crash)
	void ExecuteCallback(CallbackID callbackId, EntityID resultId);
};

//----------------------------------------------------------------------------------------------------
// Design Notes
//
// Separation Rationale:
//   - CameraAPI extracted from HighLevelEntityAPI for Single Responsibility Principle
//   - Camera operations are logically distinct from entity operations
//   - Separate JavaScript binding (CameraScriptInterface) for clear API surface
//
// Callback Execution Flow:
//   1. JavaScript calls camera.create(..., callback)
//   2. C++ stores callback in m_pendingCallbacks with ready=false
//   3. C++ submits CREATE_CAMERA command to RenderCommandQueue
//   4. Main thread processes CREATE_CAMERA, creates camera, calls NotifyCallbackReady()
//   5. CameraAPI::ExecutePendingCallbacks() executes callback with cameraId
//   6. Callback removed from m_pendingCallbacks
//
// Error Resilience Strategy:
//   - JavaScript callback errors caught with V8 TryCatch blocks
//   - C++ continues rendering even if callback throws
//   - Invalid cameraIds logged as warnings, commands ignored
//   - Queue overflow logged, creation requests dropped (user notified via callback failure)
//
// Thread Safety:
//   - CreateCamera/DestroyCamera called on JavaScript worker thread
//   - NotifyCallbackReady called on main thread (command processor)
//   - ExecutePendingCallbacks called on worker thread (requires mutex for m_pendingCallbacks)
//   - V8 locking required for callback execution (v8::Locker)
//
// Coordinate System Conventions:
//   - X-forward (+X points forward in world space)
//   - Y-left (+Y points left in world space)
//   - Z-up (+Z points up in world space)
//   - Right-handed coordinate system
//   - All positions/deltas use this convention
//----------------------------------------------------------------------------------------------------
