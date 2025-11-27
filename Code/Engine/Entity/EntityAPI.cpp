//----------------------------------------------------------------------------------------------------
// EntityAPI.cpp
// M4-T8: Engine Refactoring - Entity Management API Implementation
//----------------------------------------------------------------------------------------------------

#include "Engine/Entity/EntityAPI.hpp"
#include "Engine/Core/CallbackQueue.hpp"
#include "Engine/Renderer/RenderCommandQueue.hpp"
#include "Engine/Script/ScriptSubsystem.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/LogSubsystem.hpp"

// Undefine Windows min/max macros before including V8 headers
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

// Suppress V8 header warnings
#pragma warning(push)
#pragma warning(disable: 4100)
#pragma warning(disable: 4127)
#pragma warning(disable: 4324)

#include "v8.h"

#pragma warning(pop)

//----------------------------------------------------------------------------------------------------
// Construction / Destruction
//----------------------------------------------------------------------------------------------------

EntityAPI::EntityAPI(RenderCommandQueue* commandQueue,
                     ScriptSubsystem*    scriptSubsystem)
    : m_commandQueue(commandQueue)
      , m_scriptSubsystem(scriptSubsystem)
      , m_nextEntityId(1)      // Start entity IDs at 1 (0 reserved for invalid)
      , m_nextCallbackId(1)
{
    GUARANTEE_OR_DIE(m_commandQueue != nullptr, "EntityAPI: RenderCommandQueue is nullptr!");
    GUARANTEE_OR_DIE(m_scriptSubsystem != nullptr, "EntityAPI: ScriptSubsystem is nullptr!");

    DebuggerPrintf("EntityAPI: Initialized (M4-T8)\n");
}

//----------------------------------------------------------------------------------------------------
EntityAPI::~EntityAPI()
{
    // Log any pending callbacks that were never executed
    if (!m_pendingCallbacks.empty())
    {
        DebuggerPrintf("EntityAPI: Warning - %zu pending callbacks not executed at shutdown\n",
                       m_pendingCallbacks.size());
    }
}

//----------------------------------------------------------------------------------------------------
// Entity Creation/Destruction
//----------------------------------------------------------------------------------------------------

CallbackID EntityAPI::CreateMesh(std::string const&    meshType,
                                 Vec3 const&           position,
                                 float                 scale,
                                 Rgba8 const&          color,
                                 ScriptCallback const& callback)
{
    // Generate unique entity ID
    EntityID entityId = GenerateEntityID();

    // Generate unique callback ID
    CallbackID callbackId = GenerateCallbackID();

    DebuggerPrintf("[TRACE] EntityAPI::CreateMesh - meshType=%s, entityId=%llu, callbackId=%llu, pos=(%.1f,%.1f,%.1f), scale=%.1f\n",
                   meshType.c_str(), entityId, callbackId, position.x, position.y, position.z, scale);

    // Store callback for later execution
    PendingCallback pendingCallback;
    pendingCallback.callback       = callback;
    pendingCallback.resultId       = entityId;
    pendingCallback.ready          = false;
    m_pendingCallbacks[callbackId] = pendingCallback;

    // Create mesh creation command
    MeshCreationData meshData;
    meshData.meshType = meshType;
    meshData.position = position;
    meshData.radius   = scale;
    meshData.color    = color;

    RenderCommand command(RenderCommandType::CREATE_MESH, entityId, meshData);

    // Submit command to queue
    bool submitted = SubmitCommand(command);
    if (!submitted)
    {
        DebuggerPrintf("EntityAPI::CreateMesh - Queue full! Dropping mesh creation for entity %llu\n", entityId);
        m_pendingCallbacks[callbackId].ready    = true;
        m_pendingCallbacks[callbackId].resultId = 0;  // 0 = creation failed
    }
    else
    {
        DebuggerPrintf("[TRACE] EntityAPI::CreateMesh - Command submitted successfully to queue\n");
        m_pendingCallbacks[callbackId].ready = true;
    }

    return callbackId;
}

//----------------------------------------------------------------------------------------------------
void EntityAPI::DestroyEntity(EntityID entityId)
{
    // Create destroy command
    RenderCommand command(RenderCommandType::DESTROY_ENTITY, entityId, std::monostate{});

    bool submitted = SubmitCommand(command);
    if (!submitted)
    {
        DebuggerPrintf("EntityAPI::DestroyEntity - Queue full! Dropping destroy for entity %llu\n", entityId);
    }
}

//----------------------------------------------------------------------------------------------------
// Entity Updates
//----------------------------------------------------------------------------------------------------

void EntityAPI::UpdatePosition(EntityID entityId, Vec3 const& position)
{
    // Create entity update command with position
    EntityUpdateData updateData;
    updateData.position = position;

    RenderCommand command(RenderCommandType::UPDATE_ENTITY, entityId, updateData);

    bool submitted = SubmitCommand(command);
    if (!submitted)
    {
        DebuggerPrintf("EntityAPI::UpdatePosition - Queue full! Dropping position update for entity %llu\n", entityId);
    }
}

//----------------------------------------------------------------------------------------------------
void EntityAPI::MoveBy(EntityID entityId, Vec3 const& delta)
{
    // PHASE 2 SIMPLIFICATION: Convert delta to absolute position update
    DebuggerPrintf("EntityAPI::MoveBy - Not fully implemented in Phase 2! Use UpdatePosition instead.\n");

    // Create update command with delta (will be interpreted as absolute position for now)
    EntityUpdateData updateData;
    updateData.position = delta;

    RenderCommand command(RenderCommandType::UPDATE_ENTITY, entityId, updateData);
    SubmitCommand(command);
}

//----------------------------------------------------------------------------------------------------
void EntityAPI::UpdateOrientation(EntityID entityId, EulerAngles const& orientation)
{
    // Create entity update command with orientation
    EntityUpdateData updateData;
    updateData.orientation = orientation;

    RenderCommand command(RenderCommandType::UPDATE_ENTITY, entityId, updateData);

    bool submitted = SubmitCommand(command);
    if (!submitted)
    {
        DebuggerPrintf("EntityAPI::UpdateOrientation - Queue full! Dropping orientation update for entity %llu\n", entityId);
    }
}

//----------------------------------------------------------------------------------------------------
void EntityAPI::UpdateColor(EntityID entityId, Rgba8 const& color)
{
    // Create entity update command with color
    EntityUpdateData updateData;
    updateData.color = color;

    RenderCommand command(RenderCommandType::UPDATE_ENTITY, entityId, updateData);

    bool submitted = SubmitCommand(command);
    if (!submitted)
    {
        DebuggerPrintf("EntityAPI::UpdateColor - Queue full! Dropping color update for entity %llu\n", entityId);
    }
}

//----------------------------------------------------------------------------------------------------
// Callback Execution
//----------------------------------------------------------------------------------------------------

void EntityAPI::ExecutePendingCallbacks(CallbackQueue* callbackQueue)
{
	// Phase 2.2: Enqueue callbacks to CallbackQueue instead of executing directly
	// Note: This is called on C++ main thread, enqueues for JavaScript worker thread

	GUARANTEE_OR_DIE(callbackQueue != nullptr, "EntityAPI::ExecutePendingCallbacks - CallbackQueue is nullptr!");

	// // Log when we have pending callbacks (diagnostic)
	// if (!m_pendingCallbacks.empty())
	// {
	// 	size_t readyCount = 0;
	// 	for (auto const& pair : m_pendingCallbacks)
	// 	{
	// 		if (pair.second.ready) readyCount++;
	// 	}
	//
	// 	if (readyCount > 0)
	// 	{
	// 		DAEMON_LOG(LogScript, eLogVerbosity::Log,
	// 		           Stringf("EntityAPI::ExecutePendingCallbacks - Enqueuing %zu ready callbacks (out of %zu total)",
	// 		               readyCount, m_pendingCallbacks.size()));
	// 	}
	// }

	for (auto it = m_pendingCallbacks.begin(); it != m_pendingCallbacks.end(); ++it)
	{
		CallbackID       callbackId = it->first;
		PendingCallback& pending    = it->second;

		if (pending.ready)
		{
			// Create CallbackData from pending callback
			CallbackData data;
			data.callbackId = callbackId;
			data.resultId = pending.resultId;
			data.errorMessage = "";  // Empty = success
			data.type = CallbackType::ENTITY_CREATED;

			// Enqueue callback to CallbackQueue (async, lock-free)
			bool enqueued = callbackQueue->Enqueue(data);

			if (!enqueued)
			{
				// Backpressure: Queue full - log warning
				DAEMON_LOG(LogScript, eLogVerbosity::Warning,
				           Stringf("EntityAPI::ExecutePendingCallbacks - CallbackQueue full! Dropped callback %llu for entity %llu",
				               callbackId, pending.resultId));
			}

			// Phase 2.3 FIX: Do NOT erase here! Callback will be erased after execution in ExecuteCallback()
			// Callback ownership remains in m_pendingCallbacks until ExecuteCallback() is called
		}
	}
}


//----------------------------------------------------------------------------------------------------
void EntityAPI::NotifyCallbackReady(CallbackID callbackId, EntityID resultId)
{
    // Find callback in pending map
    auto it = m_pendingCallbacks.find(callbackId);
    if (it != m_pendingCallbacks.end())
    {
        // Mark as ready and update result ID
        it->second.ready    = true;
        it->second.resultId = resultId;
    }
    else
    {
        DebuggerPrintf("EntityAPI::NotifyCallbackReady - Callback %llu not found!\n", callbackId);
    }
}

//----------------------------------------------------------------------------------------------------
// ID Generation
//----------------------------------------------------------------------------------------------------

EntityID EntityAPI::GenerateEntityID()
{
    return m_nextEntityId++;
}

//----------------------------------------------------------------------------------------------------
CallbackID EntityAPI::GenerateCallbackID()
{
    return m_nextCallbackId++;
}

//----------------------------------------------------------------------------------------------------
// Helper Methods
//----------------------------------------------------------------------------------------------------

bool EntityAPI::SubmitCommand(RenderCommand const& command)
{
    // Submit command to queue
    bool success = m_commandQueue->Submit(command);

    if (!success)
    {
        // Queue full - backpressure triggered
        DebuggerPrintf("EntityAPI: RenderCommandQueue FULL! Command dropped.\n");
    }

    return success;
}

//----------------------------------------------------------------------------------------------------
void EntityAPI::ExecuteCallback(CallbackID callbackId,
                                EntityID   resultId)
{
    // Find callback
    auto it = m_pendingCallbacks.find(callbackId);
    if (it == m_pendingCallbacks.end())
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Warning,
                   Stringf("EntityAPI::ExecuteCallback - Callback %llu not found!", callbackId));
        return;
    }

    ScriptCallback const& callback = it->second.callback;

    // Phase 2b: Execute JavaScript callback with V8 locking
    DAEMON_LOG(LogScript, eLogVerbosity::Log,
               Stringf("EntityAPI::ExecuteCallback - Executing callback %llu with resultId %llu",
                   callbackId, resultId));

    // Get V8 isolate from ScriptSubsystem
    v8::Isolate* isolate = m_scriptSubsystem->GetIsolate();
    if (!isolate)
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Error,
                   "EntityAPI::ExecuteCallback - V8 isolate is null!");
        return;
    }

    // Execute callback with V8 locking (CRITICAL for thread safety)
    {
        v8::Locker         locker(isolate);
        v8::Isolate::Scope isolate_scope(isolate);
        v8::HandleScope    handle_scope(isolate);
        v8::TryCatch       try_catch(isolate);

        try
        {
            // Extract v8::Function from std::any
            v8::Global<v8::Function>* globalFuncPtr = nullptr;

            try
            {
                globalFuncPtr = std::any_cast<v8::Global<v8::Function>*>(callback);
            }
            catch (std::bad_any_cast const&)
            {
                DAEMON_LOG(LogScript, eLogVerbosity::Error,
                           "EntityAPI::ExecuteCallback - Failed to extract callback as v8::Global<v8::Function>*");
                return;
            }

            if (!globalFuncPtr)
            {
                DAEMON_LOG(LogScript, eLogVerbosity::Error,
                           "EntityAPI::ExecuteCallback - Callback pointer is null");
                return;
            }

            v8::Local<v8::Function> callbackFunc = globalFuncPtr->Get(isolate);

            if (callbackFunc.IsEmpty())
            {
                DAEMON_LOG(LogScript, eLogVerbosity::Error,
                           "EntityAPI::ExecuteCallback - Callback function is empty");
                return;
            }

            // Get V8 context from ScriptSubsystem
            void* contextPtr = m_scriptSubsystem->GetV8Context();
            if (!contextPtr)
            {
                DAEMON_LOG(LogScript, eLogVerbosity::Error,
                           "EntityAPI::ExecuteCallback - Failed to get V8 context from ScriptSubsystem");
                return;
            }

            v8::Local<v8::Context>* contextLocalPtr = static_cast<v8::Local<v8::Context>*>(contextPtr);
            v8::Local<v8::Context>  context         = *contextLocalPtr;

            if (context.IsEmpty())
            {
                DAEMON_LOG(LogScript, eLogVerbosity::Error,
                           "EntityAPI::ExecuteCallback - V8 context is empty after retrieval!");
                return;
            }

            v8::Context::Scope context_scope(context);

            // Create JavaScript number from entityId
            v8::Local<v8::Number> resultIdValue = v8::Number::New(isolate, static_cast<double>(resultId));

            // Prepare arguments array
            v8::Local<v8::Value> argv[1] = {resultIdValue};

            // Call JavaScript function: callback(entityId)
            v8::MaybeLocal<v8::Value> result = callbackFunc->Call(context, v8::Undefined(isolate), 1, argv);

            // Check for JavaScript exceptions
            if (try_catch.HasCaught())
            {
                v8::Local<v8::Message> message = try_catch.Message();
                v8::String::Utf8Value  error(isolate, try_catch.Exception());
                DAEMON_LOG(LogScript, eLogVerbosity::Error,
                           Stringf("EntityAPI::ExecuteCallback - JavaScript callback error: %s", *error));
                return;
            }

            if (result.IsEmpty())
            {
                DAEMON_LOG(LogScript, eLogVerbosity::Warning,
                           "EntityAPI::ExecuteCallback - Callback returned empty result");
            }

            DAEMON_LOG(LogScript, eLogVerbosity::Log,
                       Stringf("EntityAPI::ExecuteCallback - Callback %llu executed successfully", callbackId));
        }
        catch (std::bad_any_cast const& e)
        {
            DAEMON_LOG(LogScript, eLogVerbosity::Error,
                       Stringf("EntityAPI::ExecuteCallback - Failed to cast callback to v8::Global<v8::Function>: %s",
                           e.what()));
        }
        catch (std::exception const& e)
        {
            DAEMON_LOG(LogScript, eLogVerbosity::Error,
                       Stringf("EntityAPI::ExecuteCallback - Unexpected exception: %s", e.what()));
        }
    }
    // V8 lock automatically released here

    // Phase 2.3 FIX: Remove callback from pending map after successful execution
    // This prevents memory leaks and allows callbacks to be reused
    m_pendingCallbacks.erase(callbackId);

    DAEMON_LOG(LogScript, eLogVerbosity::Log,
               Stringf("[CALLBACK FLOW] ExecuteCallback - Callback %llu removed from pending map", callbackId));
}

//----------------------------------------------------------------------------------------------------
// Design Notes
//
// M4-T8 Refactoring Changes:
//   - Extracted from HighLevelEntityAPI to separate entity concerns
//   - Maintains identical functionality for backward compatibility
//   - Uses same ID generation strategy (entity IDs start at 1)
//   - Uses same callback execution pattern with V8 locking
//   - Thread-safe command submission to RenderCommandQueue
//
// Error Resilience:
//   - All methods check queue submission success
//   - Failed submissions logged but don't crash
//   - Invalid callbacks logged but don't crash
//   - JavaScript callback errors caught and logged
//----------------------------------------------------------------------------------------------------
