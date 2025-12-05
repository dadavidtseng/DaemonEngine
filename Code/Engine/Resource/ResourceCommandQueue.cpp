//----------------------------------------------------------------------------------------------------
// ResourceCommandQueue.cpp
// Phase 3: Resource Command Queue - Resource Command Queue Implementation
//----------------------------------------------------------------------------------------------------

#include "Engine/Resource/ResourceCommandQueue.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/LogSubsystem.hpp"
#include "Engine/Core/EngineCommon.hpp"

//----------------------------------------------------------------------------------------------------
// Constructor
//
// Initializes CommandQueueBase with specified capacity.
// Logs queue initialization for monitoring.
//----------------------------------------------------------------------------------------------------
ResourceCommandQueue::ResourceCommandQueue(size_t const capacity)
	: CommandQueueBase<sResourceCommand>(capacity)  // Initialize base template
{
	// Validate capacity (must be > 0)
	if (capacity == 0)
	{
		ERROR_AND_DIE("ResourceCommandQueue: Capacity must be greater than zero");
	}

	DAEMON_LOG(LogResource, eLogVerbosity::Log,
	           Stringf("ResourceCommandQueue: Initialized with capacity %llu (%.2f KB)",
	               static_cast<uint64_t>(capacity),
	               (capacity * sizeof(sResourceCommand)) / 1024.f));
}

//----------------------------------------------------------------------------------------------------
// Destructor
//
// Logs final statistics for debugging/profiling.
// Base class (CommandQueueBase) handles buffer deallocation.
//----------------------------------------------------------------------------------------------------
ResourceCommandQueue::~ResourceCommandQueue()
{
	// Log final statistics
	uint64_t totalSubmitted = GetTotalSubmitted();
	uint64_t totalConsumed  = GetTotalConsumed();

	DAEMON_LOG(LogResource, eLogVerbosity::Log,
	           Stringf("ResourceCommandQueue: Shutdown - Total submitted: %llu, Total consumed: %llu, Lost: %llu",
	               totalSubmitted,
	               totalConsumed,
	               totalSubmitted - totalConsumed));
}

//----------------------------------------------------------------------------------------------------
// OnQueueFull (Virtual Hook Override)
//
// Called by CommandQueueBase::Submit() when queue is full.
// Logs warning for monitoring/debugging.
//----------------------------------------------------------------------------------------------------
void ResourceCommandQueue::OnQueueFull()
{
	DAEMON_LOG(LogResource, eLogVerbosity::Warning,
	           Stringf("ResourceCommandQueue: Queue full! Capacity: %llu, Submitted: %llu, Consumed: %llu",
	               static_cast<uint64_t>(GetCapacity()),
	               GetTotalSubmitted(),
	               GetTotalConsumed()));
}

//----------------------------------------------------------------------------------------------------
// Implementation Notes
//
// Template Inheritance Benefits:
//   - Eliminates ~200 lines of duplicate SPSC implementation (Submit, ConsumeAll, monitoring)
//   - All lock-free ring buffer logic inherited from CommandQueueBase<sResourceCommand>
//   - Zero performance overhead (template instantiation at compile time)
//   - Maintains exact same public API (zero breaking changes)
//
// Lock-Free Progress Guarantee (Inherited from CommandQueueBase):
//   - Submit() always completes in bounded time (no blocking)
//   - ConsumeAll() always completes in bounded time (no blocking)
//   - No mutex, no conditional wait, no priority inversion
//
// Backpressure Strategy:
//   - When queue full, Submit() returns false immediately
//   - OnQueueFull() hook logs warning for monitoring
//   - Producer (JavaScript) should handle backpressure (drop or retry)
//
// Performance Profiling:
//   - GetTotalSubmitted() / GetTotalConsumed() for throughput monitoring
//   - GetApproximateSize() for queue depth monitoring
//   - OnSubmit() / OnConsume() hooks available for custom profiling
//
// Memory Safety (Inherited from CommandQueueBase):
//   - Ring buffer allocated in base constructor, deallocated in base destructor
//   - No dangling pointers (buffer lifetime = queue lifetime)
//   - sResourceCommand is copyable, no ownership issues
//
// Thread Safety Validation:
//   - Run under Thread Sanitizer (TSan) to detect data races
//   - Atomic operations satisfy happens-before relationships
//   - Cache-line padding prevents false sharing
//
// Resource-Specific Considerations:
//   - Lower capacity (200 vs 1000 for RenderCommandQueue) due to lower command frequency
//   - Larger command size (~280 bytes vs ~72 bytes) balanced by lower capacity
//   - Total memory footprint: 56 KB vs 72 KB for RenderCommandQueue
//   - Typical load: 1-10 commands/frame vs 10-50/frame for rendering
//
// Priority-Based Loading Strategy:
//   - Consumer can sort commands by priority field before dispatching
//   - Critical resources (priority 100) loaded before optional resources (priority -100)
//   - Enables progressive loading: essential assets first, extras later
//
// Async vs Sync Loading Decision:
//   - async=true commands dispatched to JobSystem (I/O worker threads)
//   - async=false commands loaded immediately on main thread (blocking)
//   - JavaScript controls strategy: async for preload, sync for critical startup
//----------------------------------------------------------------------------------------------------
