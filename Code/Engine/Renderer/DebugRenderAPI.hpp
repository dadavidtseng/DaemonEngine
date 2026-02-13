// //----------------------------------------------------------------------------------------------------
// // DebugRenderAPI.hpp
// //----------------------------------------------------------------------------------------------------
// // Phase 4: Async Debug Render System
// // C++ API layer for submitting debug render commands to RenderCommandQueue
// //
// // Pattern matches EntityAPI and CameraAPI:
// // - Provides high-level async interface for debug primitive creation
// // - Submits DEBUG_* commands to RenderCommandQueue (unified with Entity/Camera commands)
// // - No direct state ownership (App owns DebugRenderStateBuffer)
// //
// // Architecture Flow:
// // 1. JavaScript calls DebugRenderSystemScriptInterface.addWorldLine()
// // 2. Script interface calls DebugRenderAPI.AddLine()
// // 3. DebugRenderAPI submits DEBUG_ADD_LINE to RenderCommandQueue
// // 4. App::ProcessRenderCommands() processes command → updates DebugRenderStateBuffer
// // 5. App::RenderDebugPrimitives() reads state buffer → renders via DebugRenderSystem
// //----------------------------------------------------------------------------------------------------
// #pragma once
// #include "Engine/Core/Rgba8.hpp"
// #include "Engine/Math/Vec3.hpp"
// #include "Engine/Math/Vec2.hpp"
// #include "Engine/Math/Mat44.hpp"
// #include "Engine/Renderer/DebugRenderStateBuffer.hpp"  // Type alias, not a class - include required
// #include "Engine/Script/ScriptCommon.hpp"
// #include <any>
// #include <cstdint>
// #include <unordered_map>
//
// //----------------------------------------------------------------------------------------------------
// // Forward Declarations
// //----------------------------------------------------------------------------------------------------
// class RenderCommandQueue;
// class ScriptSubsystem;
// class CallbackQueue;
// enum class eDebugRenderMode : int8_t;
//
// //----------------------------------------------------------------------------------------------------
// // CallbackID Type Definition (shared with EntityAPI, CameraAPI, AudioAPI)
// using CallbackID = uint64_t;
//
// //----------------------------------------------------------------------------------------------------
// // ScriptCallback Type Definition (shared with EntityAPI, CameraAPI, AudioAPI)
// using ScriptCallback = std::any;
//
// //----------------------------------------------------------------------------------------------------
// // DebugRenderAPI
// //
// // High-level async API for debug primitive submission.
// // Matches EntityAPI/CameraAPI pattern:
// // - Constructor receives queue/subsystem pointers (NO OWNERSHIP)
// // - Public methods generate unique IDs and submit commands
// // - Returns primitive IDs for runtime modification
// //----------------------------------------------------------------------------------------------------
// class DebugRenderAPI
// {
// public:
// 	//----------------------------------------------------------------------------------------------------
// 	// Constructor
// 	//
// 	// Parameters:
// 	//   commandQueue    - RenderCommandQueue pointer (NO OWNERSHIP, must outlive this API)
// 	//   scriptSubsystem - ScriptSubsystem pointer (NO OWNERSHIP, must outlive this API)
// 	//   stateBuffer     - DebugRenderStateBuffer pointer (NO OWNERSHIP, used for ID generation)
// 	//   callbackQueue   - CallbackQueue pointer (NO OWNERSHIP, for async callback support)
// 	//----------------------------------------------------------------------------------------------------
// 	explicit DebugRenderAPI(RenderCommandQueue* commandQueue,
// 	                        ScriptSubsystem* scriptSubsystem,
// 	                        DebugRenderStateBuffer* stateBuffer,
// 	                        CallbackQueue* callbackQueue);
//
// 	//----------------------------------------------------------------------------------------------------
// 	// Debug Primitive Creation Methods
// 	//
// 	// All methods:
// 	// - Generate unique primitive IDs
// 	// - Submit DEBUG_* commands to RenderCommandQueue
// 	// - Return primitive ID for runtime modification/removal
// 	//----------------------------------------------------------------------------------------------------
//
// 	// Add 3D line segment
// 	uint32_t AddLine(Vec3 const& start, Vec3 const& end,
// 	                 Rgba8 const& startColor, Rgba8 const& endColor,
// 	                 float radius, float duration);
//
// 	// Add 3D point/billboard
// 	uint32_t AddPoint(Vec3 const& position, Rgba8 const& color,
// 	                  float radius, float duration, bool isBillboard);
//
// 	// Add 3D sphere (wireframe or solid)
// 	uint32_t AddSphere(Vec3 const& center, float radius,
// 	                   Rgba8 const& color, float duration, bool isSolid);
//
// 	// Add 3D axis-aligned bounding box
// 	uint32_t AddAABB(Vec3 const& minBounds, Vec3 const& maxBounds,
// 	                 Rgba8 const& color, float duration);
//
// 	// Add 3D coordinate system visualization (XYZ arrows)
// 	uint32_t AddBasis(Vec3 const& position,
// 	                  Vec3 const& iBasis, Vec3 const& jBasis, Vec3 const& kBasis,
// 	                  float duration, float axisLength);
//
// 	// Add 3D world-space text
// 	uint64_t AddWorldText(std::string const& text, Mat44 const& transform,
// 	                      float fontSize, Vec2 const& alignment,
// 	                      float duration, Rgba8 const& color,
// 	                      eDebugRenderMode mode);
//
// 	// Add 2D screen-space text
// 	uint64_t AddScreenText(std::string const& text, Vec2 const& position,
// 	                       float fontSize, Vec2 const& alignment,
// 	                       float duration, Rgba8 const& color);
//
// 	//----------------------------------------------------------------------------------------------------
// 	// Runtime Modification Methods
// 	//----------------------------------------------------------------------------------------------------
//
// 	// Update color of existing debug primitive
// 	void UpdateColor(uint32_t primitiveId, Rgba8 const& newColor);
//
// 	// Remove specific debug primitive
// 	void Remove(uint32_t primitiveId);
//
// 	// Clear all debug primitives
// 	void ClearAll();
//
// 	//----------------------------------------------------------------------------------------------------
// 	// Callback Execution (called by App::Update() on main thread)
// 	//----------------------------------------------------------------------------------------------------
//
// 	// Execute pending callbacks with results
// 	// Called by App::Update() after processing render commands
// 	// Executes callbacks on JavaScript worker thread with V8 locking
// 	void ExecutePendingCallbacks(CallbackQueue* callbackQueue);
//
// 	// Register a callback completion (called by command processor)
// 	void NotifyCallbackReady(CallbackID callbackId, uint64_t resultId);
//
// private:
// 	//----------------------------------------------------------------------------------------------------
// 	// Member Variables
// 	//----------------------------------------------------------------------------------------------------
// 	RenderCommandQueue*      m_commandQueue;    // Queue for command submission (NO OWNERSHIP)
// 	ScriptSubsystem*         m_scriptSubsystem; // Script subsystem reference (NO OWNERSHIP)
// 	DebugRenderStateBuffer*  m_stateBuffer;     // State buffer for ID generation (NO OWNERSHIP)
// 	CallbackQueue*           m_callbackQueue;   // Callback queue for async results (NO OWNERSHIP)
//
// 	uint32_t m_nextPrimitiveId = 1;  // Unique ID generator (0 reserved for invalid)
//
// 	// Callback ID generation
// 	CallbackID m_nextCallbackId = 1;  // Unique callback ID generator (0 reserved)
//
// 	// Callback storage (CallbackID → {ScriptCallback, resultId, ready})
// 	struct PendingCallback
// 	{
// 		ScriptCallback callback;
// 		uint64_t       resultId;
// 		bool           ready;  // True when C++ has processed command and resultId is available
// 	};
// 	std::unordered_map<CallbackID, PendingCallback> m_pendingCallbacks;
//
// 	//----------------------------------------------------------------------------------------------------
// 	// Helper Methods
// 	//----------------------------------------------------------------------------------------------------
// 	uint32_t GenerateUniquePrimitiveId();  // Atomic increment with collision detection
// 	CallbackID GenerateCallbackID();       // Generate unique callback ID
// };
//
// //----------------------------------------------------------------------------------------------------
// // USAGE EXAMPLE (from DebugRenderSystemScriptInterface):
// //----------------------------------------------------------------------------------------------------
// // // In SetupScriptingBindings():
// // m_debugRenderAPI = new DebugRenderAPI(m_renderCommandQueue, g_scriptSubsystem, m_debugRenderStateBuffer);
// // m_debugRenderScriptInterface = std::make_shared<DebugRenderSystemScriptInterface>(m_debugRenderAPI);
// //
// // // In DebugRenderSystemScriptInterface::ExecuteAddWorldLine():
// // std::any DebugRenderSystemScriptInterface::ExecuteAddWorldLine(std::vector<std::any> const& args) {
// //     Vec3 start = std::any_cast<Vec3>(args[0]);
// //     Vec3 end = std::any_cast<Vec3>(args[1]);
// //     // ... extract other parameters ...
// //
// //     uint32_t primitiveId = m_debugRenderAPI->AddLine(start, end, startColor, endColor, radius, duration);
// //     return static_cast<double>(primitiveId);  // Return to JavaScript
// // }
// //----------------------------------------------------------------------------------------------------
