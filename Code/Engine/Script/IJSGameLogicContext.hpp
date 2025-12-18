//----------------------------------------------------------------------------------------------------
// IJSGameLogicContext.hpp
// Engine Script Module - JavaScript Game Logic Context Interface
//
// Purpose:
//   Abstract interface for game-specific JavaScript execution context.
//   Decouples Engine's JSGameLogicJob from Game implementation details.
//
// Design Rationale:
//   - Dependency Inversion: Engine depends on interface, not concrete Game class
//   - Enables Engine library compilation without Game code
//   - Facilitates testing with mock implementations
//   - Provides future extensibility for different game types
//
// Usage:
//   class Game : public IJSGameLogicContext {
//       void UpdateJSWorkerThread(float deltaTime, ...) override { ... }
//   };
//
//   JSGameLogicJob* job = new JSGameLogicJob(gameContext, commandQueue, entityBuffer);
//
// Thread Safety:
//   - Interface methods called from worker thread
//   - Implementations must ensure thread-safe JavaScript execution
//   - Typically protected by v8::Locker in V8 multi-threaded mode
//
// Author: M4-T8 Engine Refactoring
// Date: 2025-10-26
//----------------------------------------------------------------------------------------------------

#pragma once

//----------------------------------------------------------------------------------------------------
// Forward Declarations
//----------------------------------------------------------------------------------------------------
class RenderCommandQueue;
class CameraStateBuffer;

//----------------------------------------------------------------------------------------------------
// IJSGameLogicContext Interface
//
// Abstract interface for JavaScript game logic execution context.
// Implemented by Game class (or other game-specific classes) to provide
// worker thread access to JavaScript execution environment.
//
// Design Notes:
//   - Pure virtual interface (no implementation in Engine)
//   - Game class implements this interface
//   - JSGameLogicJob uses this interface (dependency injection)
//   - Enables Engine to compile without Game code
//
// Thread Safety Contract:
//   - All methods called from worker thread
//   - Implementations must handle V8 thread safety (v8::Locker)
//   - Implementations must handle state buffer thread safety
//
// Future Extensions (Phase 2+):
//   - UpdateJSWorkerThread(): Execute JavaScript update logic
//   - RenderJSWorkerThread(): Execute JavaScript render logic
//   - HandleJSException(): JavaScript error recovery
//   - GetScriptSubsystem(): Access to V8 subsystem
//----------------------------------------------------------------------------------------------------
class IJSGameLogicContext
{
public:
    //------------------------------------------------------------------------------------------------
    // Construction / Destruction
    //------------------------------------------------------------------------------------------------

    // Virtual destructor (required for interface classes)
    virtual ~IJSGameLogicContext() = default;

    //------------------------------------------------------------------------------------------------
    // JavaScript Execution Interface (Worker Thread)
    //------------------------------------------------------------------------------------------------

    // Execute JavaScript update logic on worker thread
    //
    // Parameters:
    //   - deltaTime: Time since last frame (seconds)
    //   - entityBuffer: Back buffer for entity state updates (worker thread writes)
    //   - commandQueue: Render command queue for C++ communication
    //
    // Thread Safety:
    //   - Called from worker thread
    //   - Must acquire v8::Locker before V8 API calls
    //   - entityBuffer: Write to back buffer only (safe)
    //   - commandQueue: Lock-free SPSC queue (safe)
    //
    // Future Implementation (Phase 2):
    //   Calls into JSEngine.update() through ScriptSubsystem
    //   Updates entity state buffer based on JavaScript logic
    //   Submits render commands to command queue
    virtual void UpdateJSWorkerThread(float               deltaTime,
                                      RenderCommandQueue* commandQueue) = 0;

    // Execute JavaScript render logic on worker thread
    //
    // Parameters:
    //   - deltaTime: Time since last frame (seconds)
    //   - cameraBuffer: Back buffer for camera state updates (worker thread writes)
    //   - commandQueue: Render command queue for C++ communication
    //
    // Thread Safety:
    //   - Called from worker thread
    //   - Must acquire v8::Locker before V8 API calls
    //   - cameraBuffer: Write to back buffer only (safe)
    //   - commandQueue: Lock-free SPSC queue (safe)
    //
    // Future Implementation (Phase 2):
    //   Calls into JSEngine.render() through ScriptSubsystem
    //   Updates camera state buffer based on JavaScript logic
    //   Submits rendering commands to command queue
    virtual void RenderJSWorkerThread(float               deltaTime,
                                      RenderCommandQueue* commandQueue) = 0;

    //------------------------------------------------------------------------------------------------
    // Error Handling Interface (Worker Thread)
    //------------------------------------------------------------------------------------------------

    // Handle JavaScript exception from worker thread
    //
    // Parameters:
    //   - errorMessage: Exception message from JavaScript
    //   - stackTrace: JavaScript stack trace (if available)
    //
    // Thread Safety:
    //   - Called from worker thread
    //   - Implementation should log error, signal recovery
    //   - Should NOT crash worker thread
    //
    // Future Implementation (Phase 3):
    //   Log JavaScript error to console
    //   Attempt hot-reload recovery
    //   Signal main thread of error state
    //   Continue worker thread execution with last known good state
    virtual void HandleJSException(char const* errorMessage,
                                   char const* stackTrace) = 0;
};

//----------------------------------------------------------------------------------------------------
// Design Notes
//
// Why Interface Pattern?
//   - Dependency Inversion Principle: Engine depends on abstraction, not Game
//   - Enables Engine library to compile independently
//   - Facilitates unit testing with mock implementations
//   - Provides clear contract for game-specific JavaScript execution
//
// Why Pure Virtual?
//   - No default implementation in Engine (Game-specific behavior required)
//   - Forces Game class to implement all methods
//   - Clear compile-time error if methods not implemented
//
// Why Separate Update/Render Methods?
//   - Mirrors JavaScript JSEngine.update() / JSEngine.render() pattern
//   - Allows different entity/camera buffer management
//   - Enables future optimization (skip render on slow frames)
//
// Future Extensions (Phase 2+):
//   - GetV8Isolate(): Direct V8 isolate access
//   - GetScriptSubsystem(): Access to script subsystem
//   - ExecuteJavaScriptString(): Dynamic JavaScript execution
//   - SetJavaScriptBreakpoint(): Debugging support
//
// Alternative Designs Considered:
//   - Callback Functions: Rejected (less type-safe, harder to mock)
//   - Direct Game* Pointer: Rejected (tight coupling, no abstraction)
//   - Template Policy: Rejected (Engine would depend on Game template parameter)
//----------------------------------------------------------------------------------------------------
