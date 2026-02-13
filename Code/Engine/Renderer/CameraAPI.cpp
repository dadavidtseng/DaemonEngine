// //----------------------------------------------------------------------------------------------------
// // CameraAPI.cpp
// // M4-T8: Engine Refactoring - Camera Management API Implementation
// //----------------------------------------------------------------------------------------------------
//
// #include "Engine/Renderer/CameraAPI.hpp"
// #include "Engine/Core/CallbackQueue.hpp"
// #include "Engine/Renderer/CameraStateBuffer.hpp"
// #include "Engine/Renderer/RenderCommandQueue.hpp"
// #include "Engine/Script/ScriptSubsystem.hpp"
// #include "Engine/Core/ErrorWarningAssert.hpp"
// #include "Engine/Core/StringUtils.hpp"
// #include "Engine/Core/EngineCommon.hpp"
// #include "Engine/Core/LogSubsystem.hpp"
//
// // Undefine Windows min/max macros before including V8 headers
// #ifdef min
// #undef min
// #endif
// #ifdef max
// #undef max
// #endif
//
// // Suppress V8 header warnings
// #pragma warning(push)
// #pragma warning(disable: 4100)  // unreferenced formal parameter
// #pragma warning(disable: 4127)  // conditional expression is constant
// #pragma warning(disable: 4324)  // structure was padded due to alignment specifier
//
// #include "v8.h"
//
// #pragma warning(pop)
//
// //----------------------------------------------------------------------------------------------------
// // Construction / Destruction
// //----------------------------------------------------------------------------------------------------
//
// CameraAPI::CameraAPI(RenderCommandQueue* commandQueue,
//                      ScriptSubsystem* scriptSubsystem,
//                      CameraStateBuffer* cameraBuffer)
// 	: m_commandQueue(commandQueue)
// 	, m_scriptSubsystem(scriptSubsystem)
// 	, m_cameraBuffer(cameraBuffer)
// 	, m_nextCameraId(1000)   // Start camera IDs at 1000 (separate namespace from entities)
// 	, m_nextCallbackId(1)
// {
// 	GUARANTEE_OR_DIE(m_commandQueue != nullptr, "CameraAPI: RenderCommandQueue is nullptr!");
// 	GUARANTEE_OR_DIE(m_scriptSubsystem != nullptr, "CameraAPI: ScriptSubsystem is nullptr!");
// 	GUARANTEE_OR_DIE(m_cameraBuffer != nullptr, "CameraAPI: CameraStateBuffer is nullptr!");
//
// 	DebuggerPrintf("CameraAPI: Initialized (M4-T8)\n");
// }
//
// //----------------------------------------------------------------------------------------------------
// CameraAPI::~CameraAPI()
// {
// 	// Log any pending callbacks that were never executed
// 	if (!m_pendingCallbacks.empty())
// 	{
// 		DebuggerPrintf("CameraAPI: Warning - %zu pending callbacks not executed at shutdown\n",
// 		               m_pendingCallbacks.size());
// 	}
// }
//
// //----------------------------------------------------------------------------------------------------
// // Camera Creation/Destruction
// //----------------------------------------------------------------------------------------------------
//
// CallbackID CameraAPI::CreateCamera(Vec3 const& position,
//                                    EulerAngles const& orientation,
//                                    std::string const& type,
//                                    ScriptCallback const& callback)
// {
// 	// Generate unique camera ID
// 	EntityID cameraId = GenerateCameraID();
//
// 	// Generate unique callback ID
// 	CallbackID callbackId = GenerateCallbackID();
//
// 	// Store callback
// 	PendingCallback pendingCallback;
// 	pendingCallback.callback = callback;
// 	pendingCallback.resultId = cameraId;
// 	pendingCallback.ready = false;  // Will be set to true by NotifyCallbackReady()
// 	m_pendingCallbacks[callbackId] = pendingCallback;
//
// 	DAEMON_LOG(LogScript, eLogVerbosity::Log,
// 		Stringf("[CALLBACK FLOW] CreateCamera - Stored callback %llu for camera %llu (ready=false)",
// 			callbackId, cameraId));
//
// 	// Create camera creation command
// 	CameraCreationData cameraData;
// 	cameraData.position = position;
// 	cameraData.orientation = orientation;
// 	cameraData.type = type;
// 	cameraData.callbackId = callbackId;  // Phase 2.3: Pass callbackId for async notification
//
// 	RenderCommand command(RenderCommandType::CREATE_CAMERA, cameraId, cameraData);
//
// 	// Submit command to queue
// 	bool submitted = SubmitCommand(command);
// 	if (!submitted)
// 	{
// 		// Queue full - log error and mark callback as ready with invalid ID
// 		DebuggerPrintf("CameraAPI::CreateCamera - Queue full! Dropping camera creation for camera %llu\n",
// 		               cameraId);
// 		m_pendingCallbacks[callbackId].ready = true;
// 		m_pendingCallbacks[callbackId].resultId = 0;  // 0 = creation failed
// 	}
// 	// Phase 2.3 FIX: Do NOT mark callback as ready immediately!
// 	// Callback will be marked ready by command processor calling NotifyCallbackReady()
//
// 	return callbackId;
// }
//
// //----------------------------------------------------------------------------------------------------
// CallbackID CameraAPI::DestroyCamera(EntityID cameraId, ScriptCallback const& callback)
// {
// 	// Generate unique callback ID
// 	CallbackID callbackId = GenerateCallbackID();
//
// 	// Store callback
// 	PendingCallback pendingCallback;
// 	pendingCallback.callback = callback;
// 	pendingCallback.resultId = cameraId;
// 	pendingCallback.ready = false;
// 	m_pendingCallbacks[callbackId] = pendingCallback;
//
// 	// Create destroy camera command (uses std::monostate - no payload needed)
// 	RenderCommand command(RenderCommandType::DESTROY_CAMERA, cameraId, std::monostate{});
//
// 	// Submit command to queue
// 	bool submitted = SubmitCommand(command);
// 	if (!submitted)
// 	{
// 		// Queue full - log error and mark callback as ready with failure
// 		DebuggerPrintf("CameraAPI::DestroyCamera - Queue full! Dropping camera destruction for camera %llu\n",
// 		               cameraId);
// 		m_pendingCallbacks[callbackId].ready = true;
// 		m_pendingCallbacks[callbackId].resultId = 0;  // 0 = operation failed
// 	}
// 	else
// 	{
// 		// Success - mark callback as ready immediately (simplified Phase 2 approach)
// 		m_pendingCallbacks[callbackId].ready = true;
// 	}
//
// 	return callbackId;
// }
//
// //----------------------------------------------------------------------------------------------------
// // Camera Updates
// //----------------------------------------------------------------------------------------------------
//
// void CameraAPI::UpdateCamera(EntityID cameraId, Vec3 const& position, EulerAngles const& orientation)
// {
// 	// DIAGNOSTIC: Log combined camera updates
// 	static int s_updateCount = 0;
// 	s_updateCount++;
// 	if (s_updateCount % 60 == 0)
// 	{
// 		DAEMON_LOG(LogScript, eLogVerbosity::Display,
// 		           StringFormat("[DIAGNOSTIC] CameraAPI::UpdateCamera: cameraId={}, position=({:.2f}, {:.2f}, {:.2f}), orientation=(yaw={:.2f}, pitch={:.2f}, roll={:.2f})",
// 		                        cameraId, position.x, position.y, position.z,
// 		                        orientation.m_yawDegrees, orientation.m_pitchDegrees, orientation.m_rollDegrees));
// 	}
//
// 	// Create update data with BOTH position and orientation
// 	CameraUpdateData updateData;
// 	updateData.position = position;
// 	updateData.orientation = orientation;
//
// 	RenderCommand command(RenderCommandType::UPDATE_CAMERA, cameraId, updateData);
//
// 	bool submitted = SubmitCommand(command);
// 	if (!submitted)
// 	{
// 		DebuggerPrintf("CameraAPI::UpdateCamera - Queue full! Dropping camera update for camera %llu\n",
// 		               cameraId);
// 	}
// }
//
// //----------------------------------------------------------------------------------------------------
// void CameraAPI::UpdateCameraPosition(EntityID cameraId, Vec3 const& position)
// {
// 	// DIAGNOSTIC: Log position updates
// 	static int s_updateCount = 0;
// 	s_updateCount++;
// 	if (s_updateCount % 60 == 0)
// 	{
// 		DAEMON_LOG(LogScript, eLogVerbosity::Display,
// 		           StringFormat("[DIAGNOSTIC] CameraAPI::UpdateCameraPosition: cameraId={}, position=({:.2f}, {:.2f}, {:.2f})",
// 		                        cameraId, position.x, position.y, position.z));
// 	}
//
// 	// Read current orientation from back buffer and send complete state
// 	CameraUpdateData updateData;
// 	updateData.position = position;
// 	updateData.orientation = EulerAngles::ZERO;  // Default if camera not found
//
// 	// Try to read current orientation from back buffer
// 	auto* backBuffer = m_cameraBuffer->GetBackBuffer();
// 	auto it = backBuffer->find(cameraId);
// 	if (it != backBuffer->end())
// 	{
// 		// Preserve existing orientation
// 		updateData.orientation = it->second.orientation;
// 	}
//
// 	RenderCommand command(RenderCommandType::UPDATE_CAMERA, cameraId, updateData);
//
// 	bool submitted = SubmitCommand(command);
// 	if (!submitted)
// 	{
// 		DebuggerPrintf("CameraAPI::UpdateCameraPosition - Queue full! Dropping camera move for camera %llu\n",
// 		               cameraId);
// 	}
// }
//
// //----------------------------------------------------------------------------------------------------
// void CameraAPI::UpdateCameraOrientation(EntityID cameraId, EulerAngles const& orientation)
// {
// 	// DIAGNOSTIC: Log orientation updates
// 	static int s_updateCount = 0;
// 	s_updateCount++;
// 	if (s_updateCount % 60 == 0)
// 	{
// 		DAEMON_LOG(LogScript, eLogVerbosity::Display,
// 		           StringFormat("[DIAGNOSTIC] CameraAPI::UpdateCameraOrientation: cameraId={}, orientation=(yaw={:.2f}, pitch={:.2f}, roll={:.2f})",
// 		                        cameraId, orientation.m_yawDegrees, orientation.m_pitchDegrees, orientation.m_rollDegrees));
// 	}
//
// 	// Read current position from back buffer and send complete state
// 	CameraUpdateData updateData;
// 	updateData.position = Vec3::ZERO;  // Default if camera not found
// 	updateData.orientation = orientation;
//
// 	// Try to read current position from back buffer
// 	auto* backBuffer = m_cameraBuffer->GetBackBuffer();
// 	auto it = backBuffer->find(cameraId);
// 	if (it != backBuffer->end())
// 	{
// 		// Preserve existing position
// 		updateData.position = it->second.position;
// 	}
//
// 	RenderCommand command(RenderCommandType::UPDATE_CAMERA, cameraId, updateData);
//
// 	bool submitted = SubmitCommand(command);
// 	if (!submitted)
// 	{
// 		DebuggerPrintf("CameraAPI::UpdateCameraOrientation - Queue full! Dropping camera orientation update for camera %llu\n",
// 		               cameraId);
// 	}
// }
//
// //----------------------------------------------------------------------------------------------------
// void CameraAPI::MoveCameraBy(EntityID cameraId, Vec3 const& delta)
// {
// 	// PHASE 2 SIMPLIFICATION: Delta movement not fully implemented
// 	DebuggerPrintf("CameraAPI::MoveCameraBy - Not fully implemented in Phase 2! Use UpdateCameraPosition instead.\n");
//
// 	// Treat delta as absolute position temporarily
// 	CameraUpdateData updateData;
// 	updateData.position = delta;
// 	updateData.orientation = EulerAngles::ZERO;
//
// 	RenderCommand command(RenderCommandType::UPDATE_CAMERA, cameraId, updateData);
// 	SubmitCommand(command);
// }
//
// //----------------------------------------------------------------------------------------------------
// void CameraAPI::LookAtCamera(EntityID cameraId, Vec3 const& target)
// {
// 	// PHASE 2: LookAt requires calculating orientation from current position to target
// 	// This is deferred to Phase 2b
// 	DebuggerPrintf("CameraAPI::LookAtCamera - Not implemented in Phase 2!\n");
// 	DebuggerPrintf("  Camera %llu should look at (%.2f, %.2f, %.2f)\n",
// 	               cameraId, target.x, target.y, target.z);
//
// 	// TODO: Calculate orientation from camera position to target
// 	// TODO: Submit UPDATE_CAMERA command with calculated orientation
// }
//
// //----------------------------------------------------------------------------------------------------
// // Camera State Management
// //----------------------------------------------------------------------------------------------------
//
// CallbackID CameraAPI::SetActiveCamera(EntityID cameraId, ScriptCallback const& callback)
// {
// 	// Generate unique callback ID
// 	CallbackID callbackId = GenerateCallbackID();
//
// 	// Store callback
// 	PendingCallback pendingCallback;
// 	pendingCallback.callback = callback;
// 	pendingCallback.resultId = cameraId;
// 	pendingCallback.ready = false;
// 	m_pendingCallbacks[callbackId] = pendingCallback;
//
// 	// Create set active camera command (uses std::monostate - no payload needed)
// 	RenderCommand command(RenderCommandType::SET_ACTIVE_CAMERA, cameraId, std::monostate{});
//
// 	// Submit command to queue
// 	bool submitted = SubmitCommand(command);
// 	if (!submitted)
// 	{
// 		// Queue full - log error and mark callback as ready with failure
// 		DebuggerPrintf("CameraAPI::SetActiveCamera - Queue full! Dropping set active camera for camera %llu\n",
// 		               cameraId);
// 		m_pendingCallbacks[callbackId].ready = true;
// 		m_pendingCallbacks[callbackId].resultId = 0;  // 0 = operation failed
// 	}
// 	else
// 	{
// 		// Success - mark callback as ready immediately (simplified Phase 2 approach)
// 		m_pendingCallbacks[callbackId].ready = true;
// 	}
//
// 	return callbackId;
// }
//
// //----------------------------------------------------------------------------------------------------
// CallbackID CameraAPI::UpdateCameraType(EntityID cameraId, std::string const& type, ScriptCallback const& callback)
// {
// 	// Generate unique callback ID
// 	CallbackID callbackId = GenerateCallbackID();
//
// 	// Store callback
// 	PendingCallback pendingCallback;
// 	pendingCallback.callback = callback;
// 	pendingCallback.resultId = cameraId;
// 	pendingCallback.ready = false;
// 	m_pendingCallbacks[callbackId] = pendingCallback;
//
// 	// Create camera type update command
// 	CameraTypeUpdateData typeUpdateData;
// 	typeUpdateData.type = type;
//
// 	RenderCommand command(RenderCommandType::UPDATE_CAMERA_TYPE, cameraId, typeUpdateData);
//
// 	// Submit command to queue
// 	bool submitted = SubmitCommand(command);
// 	if (!submitted)
// 	{
// 		// Queue full - log error and mark callback as ready with failure
// 		DebuggerPrintf("CameraAPI::UpdateCameraType - Queue full! Dropping camera type update for camera %llu\n",
// 		               cameraId);
// 		m_pendingCallbacks[callbackId].ready = true;
// 		m_pendingCallbacks[callbackId].resultId = 0;  // 0 = operation failed
// 	}
// 	else
// 	{
// 		// Success - mark callback as ready immediately (simplified Phase 2 approach)
// 		m_pendingCallbacks[callbackId].ready = true;
// 	}
//
// 	return callbackId;
// }
//
// //----------------------------------------------------------------------------------------------------
// uintptr_t CameraAPI::GetCameraHandle(EntityID cameraId) const
// {
// 	// Look up camera by ID from CameraStateBuffer
// 	// Returns camera pointer as uintptr_t for JavaScript (cast back to pointer in C++)
// 	// Returns 0 if camera not found
//
// 	if (!m_cameraBuffer)
// 	{
// 		DebuggerPrintf("CameraAPI::GetCameraHandle - CameraStateBuffer is null!\n");
// 		return 0;
// 	}
//
// 	// Get camera from front buffer (rendering thread-safe)
// 	Camera const* camera = m_cameraBuffer->GetCameraById(cameraId);
// 	if (!camera)
// 	{
// 		DebuggerPrintf("CameraAPI::GetCameraHandle - Camera %llu not found\n", cameraId);
// 		return 0;
// 	}
//
// 	// Return camera pointer as uintptr_t
// 	// Note: Pointer valid until next SwapBuffers() call (typically one frame)
// 	return reinterpret_cast<uintptr_t>(camera);
// }
//
// //----------------------------------------------------------------------------------------------------
// // Callback Execution
// //----------------------------------------------------------------------------------------------------
//
// void CameraAPI::ExecutePendingCallbacks(CallbackQueue* callbackQueue)
// {
// 	// Phase 2.2: Enqueue callbacks to CallbackQueue instead of executing directly
// 	// Note: This is called on C++ main thread, enqueues for JavaScript worker thread
//
// 	GUARANTEE_OR_DIE(callbackQueue != nullptr, "CameraAPI::ExecutePendingCallbacks - CallbackQueue is nullptr!");
//
// 	// // Log when we have pending callbacks (diagnostic)
// 	// if (!m_pendingCallbacks.empty())
// 	// {
// 	// 	size_t readyCount = 0;
// 	// 	for (auto const& pair : m_pendingCallbacks)
// 	// 	{
// 	// 		if (pair.second.ready) readyCount++;
// 	// 	}
// 	//
// 	// 	if (readyCount > 0)
// 	// 	{
// 	// 		DAEMON_LOG(LogScript, eLogVerbosity::Log,
// 	// 			Stringf("CameraAPI::ExecutePendingCallbacks - Enqueuing %zu ready callbacks (out of %zu total)",
// 	// 				readyCount, m_pendingCallbacks.size()));
// 	// 	}
// 	// }
//
// 	for (auto it = m_pendingCallbacks.begin(); it != m_pendingCallbacks.end(); ++it)
// 	{
// 		CallbackID callbackId = it->first;
// 		PendingCallback& pending = it->second;
//
// 		if (pending.ready)
// 		{
// 			// Create CallbackData from pending callback
// 			CallbackData data;
// 			data.callbackId = callbackId;
// 			data.resultId = pending.resultId;
// 			data.errorMessage = "";  // Empty = success
// 			data.type = CallbackType::CAMERA_CREATED;
//
// 			// Enqueue callback to CallbackQueue (async, lock-free)
// 			bool enqueued = callbackQueue->Enqueue(data);
//
// 			if (!enqueued)
// 			{
// 				// Backpressure: Queue full - log warning
// 				DAEMON_LOG(LogScript, eLogVerbosity::Warning,
// 					Stringf("CameraAPI::ExecutePendingCallbacks - CallbackQueue full! Dropped callback %llu for camera %llu",
// 						callbackId, pending.resultId));
// 			}
//
// 			// Phase 2.3 FIX: Do NOT erase here! Callback will be erased after execution in ExecuteCallback()
// 			// Callback ownership remains in m_pendingCallbacks until ExecuteCallback() is called
// 		}
// 	}
// }
//
//
// //----------------------------------------------------------------------------------------------------
// void CameraAPI::NotifyCallbackReady(CallbackID callbackId, EntityID resultId)
// {
// 	DAEMON_LOG(LogScript, eLogVerbosity::Log,
// 		Stringf("[CALLBACK FLOW] NotifyCallbackReady - Looking for callback %llu with resultId %llu",
// 			callbackId, resultId));
//
// 	// Find callback in pending map
// 	auto it = m_pendingCallbacks.find(callbackId);
// 	if (it != m_pendingCallbacks.end())
// 	{
// 		// Mark as ready and update result ID
// 		it->second.ready = true;
// 		it->second.resultId = resultId;
//
// 		DAEMON_LOG(LogScript, eLogVerbosity::Log,
// 			Stringf("[CALLBACK FLOW] NotifyCallbackReady - Callback %llu marked ready=true, resultId=%llu",
// 				callbackId, resultId));
// 	}
// 	else
// 	{
// 		DAEMON_LOG(LogScript, eLogVerbosity::Error,
// 			Stringf("[CALLBACK FLOW] NotifyCallbackReady - Callback %llu NOT FOUND in pending map!",
// 				callbackId));
// 		DebuggerPrintf("CameraAPI::NotifyCallbackReady - Callback %llu not found!\n", callbackId);
// 	}
// }
//
// //----------------------------------------------------------------------------------------------------
// // ID Generation
// //----------------------------------------------------------------------------------------------------
//
// EntityID CameraAPI::GenerateCameraID()
// {
// 	return m_nextCameraId++;
// }
//
// //----------------------------------------------------------------------------------------------------
// CallbackID CameraAPI::GenerateCallbackID()
// {
// 	return m_nextCallbackId++;
// }
//
// //----------------------------------------------------------------------------------------------------
// // Helper Methods
// //----------------------------------------------------------------------------------------------------
//
// bool CameraAPI::SubmitCommand(RenderCommand const& command)
// {
// 	// Submit command to queue
// 	bool success = m_commandQueue->Submit(command);
//
// 	if (!success)
// 	{
// 		// Queue full - backpressure triggered
// 		DebuggerPrintf("CameraAPI: RenderCommandQueue FULL! Command dropped.\n");
// 	}
//
// 	return success;
// }
//
// //----------------------------------------------------------------------------------------------------
// void CameraAPI::ExecuteCallback(CallbackID callbackId, EntityID resultId)
// {
// 	// Find callback
// 	auto it = m_pendingCallbacks.find(callbackId);
// 	if (it == m_pendingCallbacks.end())
// 	{
// 		DAEMON_LOG(LogScript, eLogVerbosity::Warning,
// 			Stringf("CameraAPI::ExecuteCallback - Callback %llu not found!", callbackId));
// 		return;
// 	}
//
// 	ScriptCallback const& callback = it->second.callback;
//
// 	// Phase 2b: Execute JavaScript callback with V8 locking
// 	DAEMON_LOG(LogScript, eLogVerbosity::Log,
// 		Stringf("CameraAPI::ExecuteCallback - Executing callback %llu with resultId %llu",
// 			callbackId, resultId));
//
// 	// Get V8 isolate from ScriptSubsystem
// 	v8::Isolate* isolate = m_scriptSubsystem->GetIsolate();
// 	if (!isolate)
// 	{
// 		DAEMON_LOG(LogScript, eLogVerbosity::Error,
// 			"CameraAPI::ExecuteCallback - V8 isolate is null!");
// 		return;
// 	}
//
// 	// Execute callback with V8 locking (CRITICAL for thread safety)
// 	{
// 		v8::Locker locker(isolate);
// 		v8::Isolate::Scope isolate_scope(isolate);
// 		v8::HandleScope handle_scope(isolate);
// 		v8::TryCatch try_catch(isolate);
//
// 		try
// 		{
// 			// Extract v8::Function from std::any
// 			v8::Global<v8::Function>* globalFuncPtr = nullptr;
//
// 			try
// 			{
// 				globalFuncPtr = std::any_cast<v8::Global<v8::Function>*>(callback);
// 			}
// 			catch (std::bad_any_cast const&)
// 			{
// 				DAEMON_LOG(LogScript, eLogVerbosity::Error,
// 					"CameraAPI::ExecuteCallback - Failed to extract callback as v8::Global<v8::Function>*");
// 				return;
// 			}
//
// 			// Validate pointer is not null
// 			if (!globalFuncPtr)
// 			{
// 				DAEMON_LOG(LogScript, eLogVerbosity::Error,
// 					"CameraAPI::ExecuteCallback - Callback pointer is null");
// 				return;
// 			}
//
// 			// Convert Global to Local for execution
// 			v8::Local<v8::Function> callbackFunc = globalFuncPtr->Get(isolate);
//
// 			// Validate function is not empty
// 			if (callbackFunc.IsEmpty())
// 			{
// 				DAEMON_LOG(LogScript, eLogVerbosity::Error,
// 					"CameraAPI::ExecuteCallback - Callback function is empty");
// 				return;
// 			}
//
// 			// Get V8 context from ScriptSubsystem
// 			void* contextPtr = m_scriptSubsystem->GetV8Context();
// 			if (!contextPtr)
// 			{
// 				DAEMON_LOG(LogScript, eLogVerbosity::Error,
// 					"CameraAPI::ExecuteCallback - Failed to get V8 context from ScriptSubsystem");
// 				return;
// 			}
//
// 			// Cast void* to v8::Local<v8::Context>*
// 			v8::Local<v8::Context>* contextLocalPtr = static_cast<v8::Local<v8::Context>*>(contextPtr);
// 			v8::Local<v8::Context> context = *contextLocalPtr;
//
// 			// Validate context is not empty
// 			if (context.IsEmpty())
// 			{
// 				DAEMON_LOG(LogScript, eLogVerbosity::Error,
// 					"CameraAPI::ExecuteCallback - V8 context is empty after retrieval!");
// 				return;
// 			}
//
// 			// Enter the V8 context scope
// 			v8::Context::Scope context_scope(context);
//
// 			// Create JavaScript number from cameraId
// 			v8::Local<v8::Number> resultIdValue = v8::Number::New(isolate, static_cast<double>(resultId));
//
// 			// Prepare arguments array
// 			v8::Local<v8::Value> argv[1] = { resultIdValue };
//
// 			// Call JavaScript function: callback(cameraId)
// 			v8::MaybeLocal<v8::Value> result = callbackFunc->Call(context, v8::Undefined(isolate), 1, argv);
//
// 			// Check for JavaScript exceptions
// 			if (try_catch.HasCaught())
// 			{
// 				v8::Local<v8::Message> message = try_catch.Message();
// 				v8::String::Utf8Value error(isolate, try_catch.Exception());
// 				DAEMON_LOG(LogScript, eLogVerbosity::Error,
// 					Stringf("CameraAPI::ExecuteCallback - JavaScript callback error: %s", *error));
//
// 				// C++ rendering continues even if JavaScript throws
// 				return;
// 			}
//
// 			// Check if Call() returned empty
// 			if (result.IsEmpty())
// 			{
// 				DAEMON_LOG(LogScript, eLogVerbosity::Warning,
// 					"CameraAPI::ExecuteCallback - Callback returned empty result");
// 			}
//
// 			DAEMON_LOG(LogScript, eLogVerbosity::Log,
// 				Stringf("CameraAPI::ExecuteCallback - Callback %llu executed successfully", callbackId));
// 		}
// 		catch (std::bad_any_cast const& e)
// 		{
// 			DAEMON_LOG(LogScript, eLogVerbosity::Error,
// 				Stringf("CameraAPI::ExecuteCallback - Failed to cast callback to v8::Global<v8::Function>: %s",
// 					e.what()));
// 		}
// 		catch (std::exception const& e)
// 		{
// 			DAEMON_LOG(LogScript, eLogVerbosity::Error,
// 				Stringf("CameraAPI::ExecuteCallback - Unexpected exception: %s", e.what()));
// 		}
// 	}
// 	// V8 lock automatically released here
//
// 	// Phase 2.3 FIX: Remove callback from pending map after successful execution
// 	// This prevents memory leaks and allows callbacks to be reused
// 	m_pendingCallbacks.erase(callbackId);
//
// 	DAEMON_LOG(LogScript, eLogVerbosity::Log,
// 		Stringf("[CALLBACK FLOW] ExecuteCallback - Callback %llu removed from pending map", callbackId));
// }
//
// //----------------------------------------------------------------------------------------------------
// // Design Notes
// //
// // M4-T8 Refactoring Changes:
// //   - Extracted from HighLevelEntityAPI to separate camera concerns
// //   - Maintains identical functionality for backward compatibility
// //   - Uses same ID generation strategy (camera IDs start at 1000)
// //   - Uses same callback execution pattern with V8 locking
// //   - Thread-safe command submission to RenderCommandQueue
// //
// // Error Resilience:
// //   - All methods check queue submission success
// //   - Failed submissions logged but don't crash
// //   - Invalid callbacks logged but don't crash
// //   - JavaScript callback errors caught and logged
// //----------------------------------------------------------------------------------------------------
