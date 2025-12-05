//----------------------------------------------------------------------------------------------------
// ResourceLoadJob.hpp
// Phase 3: Resource Command Queue - JobSystem Integration
//
// Purpose:
//   Wraps resource loading commands (sResourceCommand) as Job instances for JobSystem execution.
//   Processes resource loading on I/O worker threads to avoid blocking the main thread.
//   Submits results to CallbackQueue for JavaScript notification.
//
// Design Rationale:
//   - Inherits from Job with JOB_TYPE_IO (designated for file I/O operations)
//   - Stores sResourceCommand, ResourceSubsystem pointer, and CallbackQueue pointer
//   - Execute() processes command on worker thread, loads resource, submits callback
//   - Error handling: Never crashes worker thread, always submits callback (success or failure)
//
// Thread Safety:
//   - Job creation: Main thread only (JavaScript → ResourceCommandQueue → ResourceLoadJob)
//   - Job execution: I/O worker threads only (JobSystem workers)
//   - Job deletion: Main thread only (after completion)
//   - ResourceSubsystem calls: Thread-safe (internal mutex for cache access)
//   - CallbackQueue: Thread-safe (SPSC queue, producer = worker thread)
//
// Critical Constraint:
//   **DO NOT CALL DIRECTX APIs ON WORKER THREADS**
//   - File I/O: Safe on worker threads ✓
//   - CPU processing (parsing, decompression): Safe on worker threads ✓
//   - GPU uploads (CreateTexture, CreateBuffer): MAIN THREAD ONLY ✗
//   - Solution: Load data on worker thread, GPU upload deferred to main thread
//
// Author: Phase 3 - Resource Command Queue Implementation
// Date: 2025-12-01
//----------------------------------------------------------------------------------------------------

#pragma once

//----------------------------------------------------------------------------------------------------
#include "Engine/Core/Job.hpp"
#include "Engine/Resource/ResourceCommand.hpp"

//----------------------------------------------------------------------------------------------------
// Forward declarations
class ResourceSubsystem;
class CallbackQueue;

//----------------------------------------------------------------------------------------------------
// ResourceLoadJob
//
// Job wrapper for asynchronous resource loading commands.
// Processes sResourceCommand on I/O worker threads via JobSystem.
//
// Usage Pattern:
//
// Main Thread (ProcessPendingCommands):
//   resourceCommandQueue->ConsumeAll([](sResourceCommand const& cmd) {
//       auto* job = new ResourceLoadJob(cmd, resourceSubsystem, callbackQueue);
//       jobSystem->Submit(job);
//   });
//
// I/O Worker Thread (JobSystem):
//   job->Execute();  // Loads resource from disk
//                    // Submits CallbackData to CallbackQueue
//                    // JavaScript receives callback on main thread
//
// Thread Safety Guarantees:
//   - Execute() runs on I/O worker thread (JOB_TYPE_IO)
//   - ResourceSubsystem methods are thread-safe (internal locking)
//   - CallbackQueue::Submit() is thread-safe (lock-free SPSC)
//   - No DirectX calls in Execute() (GPU uploads deferred to main thread)
//
// Error Handling:
//   - File not found → CallbackData with errorMessage, resultId = 0
//   - Invalid format → CallbackData with errorMessage, resultId = 0
//   - Exception during load → Caught, logged, CallbackData with error
//   - Worker thread never crashes (defensive programming)
//----------------------------------------------------------------------------------------------------
class ResourceLoadJob : public Job
{
public:
	//------------------------------------------------------------------------------------------------
	// Constructor
	//
	// Creates a resource loading job for the specified command.
	//
	// Parameters:
	//   - command: Resource loading command to process (copied, not referenced)
	//   - resourceSubsystem: Pointer to ResourceSubsystem (must outlive job execution)
	//   - callbackQueue: Pointer to CallbackQueue for result notification (must outlive job)
	//
	// Thread Safety:
	//   - Must be called from main thread only
	//   - Pointers must remain valid until job completes and is deleted
	//------------------------------------------------------------------------------------------------
	ResourceLoadJob(sResourceCommand const& command,
	                ResourceSubsystem* resourceSubsystem,
	                CallbackQueue* callbackQueue);

	//------------------------------------------------------------------------------------------------
	// Destructor
	//
	// Default destructor (no special cleanup required).
	// Job deletion handled by main thread after retrieval from JobSystem.
	//------------------------------------------------------------------------------------------------
	~ResourceLoadJob() override = default;

	//------------------------------------------------------------------------------------------------
	// Execute (Job Interface Implementation)
	//
	// Processes the resource loading command on I/O worker thread.
	// Handles all ResourceCommandType cases via std::visit pattern.
	// Submits results to CallbackQueue for JavaScript notification.
	//
	// Command Processing:
	//   LOAD_TEXTURE:  Load texture data from disk, submit callback with ResourceID
	//   LOAD_MODEL:    Load model geometry from disk, submit callback with ResourceID
	//   LOAD_SHADER:   Load shader source from disk, submit callback with ResourceID
	//   LOAD_AUDIO:    Load audio data via FMOD, submit callback with ResourceID
	//   LOAD_FONT:     Load bitmap font data, submit callback with ResourceID
	//   UNLOAD_RESOURCE: Decrement reference count, submit callback with success status
	//
	// Error Handling:
	//   - File I/O errors → CallbackData with errorMessage
	//   - Invalid format → CallbackData with errorMessage
	//   - Uncaught exceptions → Logged, CallbackData with error
	//   - All errors are non-fatal (worker thread continues execution)
	//
	// Thread Safety:
	//   - Runs on I/O worker thread (JOB_TYPE_IO)
	//   - ResourceSubsystem calls are thread-safe
	//   - CallbackQueue::Submit() is thread-safe
	//   - NO DirectX calls (deferred to main thread)
	//
	// Performance:
	//   - Typical execution time: 10-100ms (disk I/O dependent)
	//   - CPU-intensive operations (decompression, parsing) acceptable on worker thread
	//   - GPU uploads deferred to main thread (DirectX constraint)
	//------------------------------------------------------------------------------------------------
	void Execute() override;

	// Prevent copying and moving (jobs are unique, managed by JobSystem)
	ResourceLoadJob(ResourceLoadJob const&)            = delete;
	ResourceLoadJob& operator=(ResourceLoadJob const&) = delete;
	ResourceLoadJob(ResourceLoadJob&&)                 = delete;
	ResourceLoadJob& operator=(ResourceLoadJob&&)      = delete;

private:
	//------------------------------------------------------------------------------------------------
	// Member Variables
	//------------------------------------------------------------------------------------------------
	sResourceCommand   m_command;           // Resource loading command to process
	ResourceSubsystem* m_resourceSubsystem; // ResourceSubsystem for loading operations
	CallbackQueue*     m_callbackQueue;     // CallbackQueue for result notification

	//------------------------------------------------------------------------------------------------
	// Helper Methods (for cleaner Execute() implementation)
	//------------------------------------------------------------------------------------------------
	void ProcessLoadTexture(sTextureLoadData const& data);
	void ProcessLoadModel(sModelLoadData const& data);
	void ProcessLoadShader(sShaderLoadData const& data);
	void ProcessLoadAudio(sAudioLoadData const& data);
	void ProcessLoadFont(sFontLoadData const& data);
	void ProcessUnloadResource(sResourceUnloadData const& data);

	// Helper method to submit success callback with resourceId
	void SubmitSuccessCallback(uint64_t callbackId, uint64_t resourceId);

	// Helper method to submit error callback with errorMessage
	void SubmitErrorCallback(uint64_t callbackId, String const& errorMessage);
};

//----------------------------------------------------------------------------------------------------
// Design Notes
//
// JobSystem Integration Benefits:
//   - Eliminates custom worker threads in ResourceSubsystem (~150 lines removed)
//   - Unified thread pool management (JobSystem handles all I/O workers)
//   - Automatic work distribution across available I/O workers
//   - Job prioritization via JobSystem (high-priority resources loaded first)
//
// GPU Upload Deferral Strategy:
//   - Worker thread: Load file data, parse/decompress, store in CPU memory
//   - Main thread: ProcessPendingCallbacks() → GPU upload → Bind to rendering pipeline
//   - Reason: DirectX requires all GPU resource creation on main thread
//   - Alternative considered: Deferred rendering context (too complex, minimal benefit)
//
// Error Handling Philosophy:
//   - Non-fatal errors: Submit CallbackData with errorMessage, worker continues
//   - Fatal errors (nullptr, heap corruption): Assert/log, but never crash worker
//   - JavaScript receives error callback → Can retry, show error UI, or fallback
//
// Memory Management:
//   - sResourceCommand: Copied into job (value semantics, safe across threads)
//   - ResourceSubsystem*: Borrowed pointer, must outlive job execution
//   - CallbackQueue*: Borrowed pointer, must outlive job execution
//   - Job lifetime: Created on main thread, executed on worker, deleted on main thread
//
// Performance Profiling:
//   - Add DAEMON_LOG statements in Execute() for load time tracking
//   - Monitor JobSystem queue depth (backpressure detection)
//   - Track callback latency (worker → main thread → JavaScript)
//----------------------------------------------------------------------------------------------------
