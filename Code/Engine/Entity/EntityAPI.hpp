// //----------------------------------------------------------------------------------------------------
// // EntityAPI.hpp
// // M4-T8: Engine Refactoring - Entity Management API
// //
// // Purpose:
// //   Provides high-level entity management API for JavaScript integration.
// //   Extracted from HighLevelEntityAPI to separate entity concerns from camera concerns.
// //
// // Design Philosophy:
// //   - Single Responsibility: Entity-specific operations only
// //   - Async callbacks for creation operations (error resilience)
// //   - Thread-safe command submission to RenderCommandQueue
// //   - JavaScript errors must NOT crash C++ rendering
// //
// // API Surface:
// //   Entity Creation/Destruction:
// //     - CreateMesh(type, position, scale, color, callback) - Async, returns entityId via callback
// //     - DestroyEntity(entityId) - Remove entity from rendering
// //
// //   Entity Updates:
// //     - UpdatePosition(entityId, position) - Absolute positioning
// //     - MoveBy(entityId, delta) - Relative movement
// //     - UpdateOrientation(entityId, orientation) - Euler angles
// //     - UpdateColor(entityId, color) - RGBA color
// //
// // Coordinate System:
// //   X-forward, Y-left, Z-up (right-handed)
// //   +X = forward, +Y = left, +Z = up
// //
// // Thread Safety:
// //   - Methods submit RenderCommands to RenderCommandQueue (lock-free)
// //   - Callbacks executed on JavaScript worker thread (V8 isolation required)
// //   - C++ rendering continues even if JavaScript callbacks throw errors
// //
// // Author: M4-T8 Engine Refactoring
// // Date: 2025-10-26
// //----------------------------------------------------------------------------------------------------
// #pragma once
// //----------------------------------------------------------------------------------------------------
// #include "Engine/Entity/EntityID.hpp"
// #include "Engine/Renderer/RenderCommand.hpp"
// #include "Engine/Script/ScriptCommon.hpp"
// //----------------------------------------------------------------------------------------------------
// #include <any>
// #include <unordered_map>
//
// //-Forward-Declaration--------------------------------------------------------------------------------
// class RenderCommandQueue;
// class ScriptSubsystem;
// class CallbackQueue;
//
// //----------------------------------------------------------------------------------------------------
// // CallbackID Type Definition (shared with CameraAPI)
// using CallbackID = uint64_t;
//
// //----------------------------------------------------------------------------------------------------
// // ScriptCallback Type Definition (shared with CameraAPI)
// using ScriptCallback = std::any;
//
// ///----------------------------------------------------------------------------------------------------
// /// EntityAPI
// ///
// /// High-level entity management API for JavaScript integration.
// /// Handles entity creation, updates, and destruction through render command queue.
// ///
// /// Usage Pattern (from JavaScript):
// ///
// /// Entity Creation (Async):
// ///   entity.createMesh('cube', {
// ///       position: {x: 5, y: 0, z: 0},  // X-forward, Y-left, Z-up
// ///       scale: 1.0,
// ///       color: {r: 255, g: 0, b: 0, a: 255}
// ///   }, (entityId) => {
// ///       console.log('Entity created:', entityId);
// ///   });
// ///
// /// Entity Update (Sync):
// ///   entity.updatePosition(entityId, {x: 10, y: 0, z: 0});  // Absolute
// ///   entity.moveBy(entityId, {dx: 1, dy: 0, dz: 0});        // Relative (+X = forward)
// ///   entity.updateOrientation(entityId, {yaw: 45, pitch: 0, roll: 0});
// ///   entity.updateColor(entityId, {r: 0, g: 255, b: 0, a: 255});
// ///
// /// Error Resilience:
// ///   - JavaScript callback errors are caught and logged
// ///   - C++ rendering continues with last valid state
// ///   - Invalid entityIds are ignored with warning logs
// ///----------------------------------------------------------------------------------------------------
// class EntityAPI
// {
// public:
// 	//------------------------------------------------------------------------------------------------
// 	// Construction / Destruction
// 	//------------------------------------------------------------------------------------------------
// 	explicit EntityAPI(RenderCommandQueue* commandQueue,
// 	                   ScriptSubsystem* scriptSubsystem);
// 	~EntityAPI();
//
// 	// Non-copyable (manages callback state)
// 	EntityAPI(EntityAPI const&)            = delete;
// 	EntityAPI& operator=(EntityAPI const&) = delete;
//
// 	//------------------------------------------------------------------------------------------------
// 	// Entity Creation/Destruction
// 	//------------------------------------------------------------------------------------------------
//
// 	// Create a mesh entity (async, returns entityId via callback)
// 	// Parameters:
// 	//   - meshType: "cube", "sphere", "grid", "plane"
// 	//   - position: {x, y, z} world-space position (X-forward, Y-left, Z-up)
// 	//   - scale: Uniform scale (float)
// 	//   - color: {r, g, b, a} RGBA color (0-255)
// 	//   - callback: JavaScript function (entityId) => {...}
// 	// Returns: CallbackID (for internal tracking, not exposed to JavaScript)
// 	CallbackID CreateMesh(std::string const& meshType,
// 	                      Vec3 const& position,
// 	                      float scale,
// 	                      Rgba8 const& color,
// 	                      ScriptCallback const& callback);
//
// 	// Destroy entity (remove from rendering)
// 	void DestroyEntity(EntityID entityId);
//
// 	//------------------------------------------------------------------------------------------------
// 	// Entity Updates
// 	//------------------------------------------------------------------------------------------------
//
// 	// Update entity position (absolute, world-space)
// 	void UpdatePosition(EntityID entityId, Vec3 const& position);
//
// 	// Move entity by delta (relative movement)
// 	// Delta convention: +X = forward, +Y = left, +Z = up
// 	void MoveBy(EntityID entityId, Vec3 const& delta);
//
// 	// Update entity orientation (Euler angles in degrees)
// 	void UpdateOrientation(EntityID entityId, EulerAngles const& orientation);
//
// 	// Update entity color (RGBA)
// 	void UpdateColor(EntityID entityId, Rgba8 const& color);
//
// 	//------------------------------------------------------------------------------------------------
// 	// Callback Execution (called by HighLevelEntityAPI/main thread)
// 	//------------------------------------------------------------------------------------------------
//
// 	// Execute pending callbacks with results
// 	// Called by App::Update() after processing render commands
// 	// Executes callbacks on JavaScript worker thread with V8 locking
// 	void ExecutePendingCallbacks(CallbackQueue* callbackQueue);
//
// 	// Register a callback completion (called by command processor)
// 	void NotifyCallbackReady(CallbackID callbackId, EntityID resultId);
//
// 	// Execute a single callback (with error handling to prevent C++ crash)
// 	// Phase 2.3: Made public to allow CallbackQueue consumer to invoke callbacks
// 	// Called by App::Update() after dequeuing from CallbackQueue
// 	void ExecuteCallback(CallbackID callbackId, EntityID resultId);
//
// 	//------------------------------------------------------------------------------------------------
// 	// ID Generation (for HighLevelEntityAPI coordination)
// 	//------------------------------------------------------------------------------------------------
//
// 	// Generate next entity ID
// 	EntityID GenerateEntityID();
//
// 	// Generate next callback ID
// 	CallbackID GenerateCallbackID();
//
// private:
// 	//------------------------------------------------------------------------------------------------
// 	// Internal State
// 	//------------------------------------------------------------------------------------------------
//
// 	RenderCommandQueue* m_commandQueue;     // Queue for submitting render commands
// 	ScriptSubsystem*    m_scriptSubsystem;  // Script subsystem for callback execution
//
// 	// Entity ID generation
// 	EntityID m_nextEntityId;   // Auto-incremented entity ID counter (starts at 1)
//
// 	// Callback ID generation
// 	CallbackID m_nextCallbackId;  // Auto-incremented callback ID counter
//
// 	// Callback storage (CallbackID → {ScriptFunction, resultId})
// 	struct PendingCallback
// 	{
// 		ScriptCallback callback;
// 		EntityID       resultId;
// 		bool           ready;  // True when C++ has processed command and resultId is available
// 	};
// 	std::unordered_map<CallbackID, PendingCallback> m_pendingCallbacks;
//
// 	//------------------------------------------------------------------------------------------------
// 	// Helper Methods
// 	//------------------------------------------------------------------------------------------------
//
// 	// Submit render command to queue (with error handling)
// 	bool SubmitCommand(RenderCommand const& command);
// };
//
// //----------------------------------------------------------------------------------------------------
// // Design Notes
// //
// // Separation Rationale:
// //   - EntityAPI extracted from HighLevelEntityAPI for Single Responsibility Principle
// //   - Entity operations are logically distinct from camera operations
// //   - Separate JavaScript binding (EntityScriptInterface) for clear API surface
// //
// // Callback Execution Flow:
// //   1. JavaScript calls entity.createMesh(..., callback)
// //   2. C++ stores callback in m_pendingCallbacks with ready=false
// //   3. C++ submits CREATE_MESH command to RenderCommandQueue
// //   4. Main thread processes CREATE_MESH, creates entity, calls NotifyCallbackReady()
// //   5. EntityAPI::ExecutePendingCallbacks() executes callback with entityId
// //   6. Callback removed from m_pendingCallbacks
// //
// // Error Resilience Strategy:
// //   - JavaScript callback errors caught with V8 TryCatch blocks
// //   - C++ continues rendering even if callback throws
// //   - Invalid entityIds logged as warnings, commands ignored
// //   - Queue overflow logged, creation requests dropped (user notified via callback failure)
// //
// // Thread Safety:
// //   - CreateMesh called on JavaScript worker thread
// //   - NotifyCallbackReady called on main thread (command processor)
// //   - ExecutePendingCallbacks called on worker thread (requires mutex for m_pendingCallbacks)
// //   - V8 locking required for callback execution (v8::Locker)
// //
// // Coordinate System Conventions:
// //   - X-forward (+X points forward in world space)
// //   - Y-left (+Y points left in world space)
// //   - Z-up (+Z points up in world space)
// //   - Right-handed coordinate system
// //   - All positions/deltas use this convention
// //----------------------------------------------------------------------------------------------------
