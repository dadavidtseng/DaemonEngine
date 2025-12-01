//----------------------------------------------------------------------------------------------------
// CallbackQueue.cpp
// Phase 1: Command Queue Refactoring - Callback Queue Implementation
//----------------------------------------------------------------------------------------------------

#include "Engine/Core/CallbackQueue.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/LogSubsystem.hpp"
#include "Engine/Core/EngineCommon.hpp"

//----------------------------------------------------------------------------------------------------
// Constructor
//
// Initializes CommandQueueBase with specified capacity.
// Logs queue initialization for monitoring.
//----------------------------------------------------------------------------------------------------
CallbackQueue::CallbackQueue(size_t const capacity)
	: CommandQueueBase<CallbackData>(capacity)  // Initialize base template
{
	// Validate capacity (must be > 0)
	if (capacity == 0)
	{
		ERROR_AND_DIE("CallbackQueue: Capacity must be greater than zero");
	}

	DAEMON_LOG(LogCore, eLogVerbosity::Log,
	           Stringf("CallbackQueue: Initialized with capacity %llu (%.2f KB)",
	               static_cast<uint64_t>(capacity),
	               (capacity * sizeof(CallbackData)) / 1024.f));
}

//----------------------------------------------------------------------------------------------------
// Destructor
//
// Logs final statistics for debugging/profiling.
// Base class (CommandQueueBase) handles buffer deallocation.
//----------------------------------------------------------------------------------------------------
CallbackQueue::~CallbackQueue()
{
	// Log final statistics
	uint64_t totalEnqueued = GetTotalEnqueued();
	uint64_t totalDequeued = GetTotalDequeued();

	DAEMON_LOG(LogCore, eLogVerbosity::Log,
	           Stringf("CallbackQueue: Shutdown - Total enqueued: %llu, Total dequeued: %llu, Lost: %llu",
	               totalEnqueued,
	               totalDequeued,
	               totalEnqueued - totalDequeued));
}

//----------------------------------------------------------------------------------------------------
// Implementation Notes
//
// Template Inheritance Benefits:
//   - Eliminates ~150 lines of duplicate SPSC implementation (Enqueue, DequeueAll, monitoring)
//   - All lock-free ring buffer logic inherited from CommandQueueBase<CallbackData>
//   - Zero performance overhead (template instantiation at compile time)
//   - Maintains exact same public API (zero breaking changes via wrappers)
//
// API Wrapper Pattern:
//   - Enqueue() wraps Submit() for backward compatibility
//   - DequeueAll() wraps ConsumeAll() for backward compatibility
//   - GetTotalEnqueued() wraps GetTotalSubmitted()
//   - GetTotalDequeued() wraps GetTotalConsumed()
//   - All other monitoring APIs (IsEmpty, IsFull, GetApproximateSize) inherited directly
//
// Lock-Free Progress Guarantee (Inherited from CommandQueueBase):
//   - Enqueue() always completes in bounded time (no blocking)
//   - DequeueAll() always completes in bounded time (no blocking)
//   - No mutex, no conditional wait, no priority inversion
//
// Backpressure Strategy:
//   - When queue full, Enqueue() returns false immediately
//   - Producer (C++ main thread) should log warning and drop callback
//   - Future optimization: Dynamic capacity expansion or priority queue
//
// Performance Profiling:
//   - GetTotalEnqueued() / GetTotalDequeued() for throughput monitoring
//   - GetApproximateSize() for queue depth monitoring
//   - Can add virtual hooks (OnSubmit, OnConsume) for custom profiling
//
// Memory Safety (Inherited from CommandQueueBase):
//   - Ring buffer allocated in base constructor, deallocated in base destructor
//   - No dangling pointers (buffer lifetime = queue lifetime)
//   - CallbackData is copyable, no ownership issues
//
// Thread Safety Validation:
//   - Run under Thread Sanitizer (TSan) to detect data races
//   - Atomic operations satisfy happens-before relationships
//   - Cache-line padding prevents false sharing
//
// Callback Ownership Model:
//   - JavaScript owns callback functions in g_pendingCallbacks Map
//   - C++ only stores callback IDs (uint64_t), not v8::Function objects
//   - This ensures thread safety - no v8::Function across frames
//----------------------------------------------------------------------------------------------------
