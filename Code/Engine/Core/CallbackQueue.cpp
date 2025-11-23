//----------------------------------------------------------------------------------------------------
// CallbackQueue.cpp
// Phase 2: Async Architecture - Lock-Free SPSC Callback Queue Implementation
//----------------------------------------------------------------------------------------------------

#include "Engine/Core/CallbackQueue.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/LogSubsystem.hpp"
#include "Engine/Core/EngineCommon.hpp"

//----------------------------------------------------------------------------------------------------
// Constructor
//
// Allocates ring buffer with specified capacity.
// Initializes atomic indices and statistics counters.
//----------------------------------------------------------------------------------------------------
CallbackQueue::CallbackQueue(size_t const capacity)
	: m_capacity(capacity)
{
	// Validate capacity (must be > 0)
	if (capacity == 0)
	{
		ERROR_AND_DIE("CallbackQueue: Capacity must be greater than zero");
	}

	// Allocate ring buffer storage
	m_buffer = new CallbackData[m_capacity];

	DAEMON_LOG(LogCore, eLogVerbosity::Log,
	           Stringf("CallbackQueue: Initialized with capacity %llu (%.2f KB)",
	               static_cast<uint64_t>(m_capacity),
	               (m_capacity * sizeof(CallbackData)) / 1024.f));
}

//----------------------------------------------------------------------------------------------------
// Destructor
//
// Deallocates ring buffer storage.
// Logs statistics for debugging/profiling.
//----------------------------------------------------------------------------------------------------
CallbackQueue::~CallbackQueue()
{
	// Log final statistics
	uint64_t totalEnqueued = m_totalEnqueued.load(std::memory_order_relaxed);
	uint64_t totalDequeued = m_totalDequeued.load(std::memory_order_relaxed);

	DAEMON_LOG(LogCore, eLogVerbosity::Log,
	           Stringf("CallbackQueue: Shutdown - Total enqueued: %llu, Total dequeued: %llu, Lost: %llu",
	               totalEnqueued,
	               totalDequeued,
	               totalEnqueued - totalDequeued));

	// Deallocate buffer
	delete[] m_buffer;
	m_buffer = nullptr;
}

//----------------------------------------------------------------------------------------------------
// Enqueue (Producer API)
//
// Enqueues a callback to the ring buffer (non-blocking, lock-free).
//
// Algorithm:
//   1. Load current tail (producer write position)
//   2. Calculate next tail position
//   3. Load current head (consumer read position) with acquire semantics
//   4. Check if queue is full (next tail == head)
//   5. If not full: Write callback, advance tail with release semantics
//
// Memory Ordering:
//   - m_head.load (acquire): Ensures consumer's updates are visible to producer
//   - m_tail.store (release): Ensures callback data is visible to consumer before tail update
//
// Returns:
//   true  - Callback successfully enqueued
//   false - Queue full (backpressure)
//----------------------------------------------------------------------------------------------------
bool CallbackQueue::Enqueue(CallbackData const& callback)
{
	// Load current producer position (relaxed ordering sufficient for SPSC)
	size_t currentTail = m_tail.load(std::memory_order_relaxed);
	size_t nextTail    = NextIndex(currentTail);

	// Load current consumer position (acquire ordering to synchronize with consumer's release)
	size_t currentHead = m_head.load(std::memory_order_acquire);

	// Check if queue is full (next tail would equal head)
	if (nextTail == currentHead)
	{
		// Queue full - backpressure triggered
		return false;
	}

	// Write callback to buffer
	m_buffer[currentTail] = callback;

	// Update producer tail position (release ordering to ensure callback data is visible)
	m_tail.store(nextTail, std::memory_order_release);

	// Increment enqueue counter
	m_totalEnqueued.fetch_add(1, std::memory_order_relaxed);

	return true;
}

//----------------------------------------------------------------------------------------------------
// GetApproximateSize (Monitoring API)
//
// Returns current queue size (difference between tail and head).
// Value may be stale due to concurrent consumer, use for monitoring only.
//----------------------------------------------------------------------------------------------------
size_t CallbackQueue::GetApproximateSize() const
{
	size_t currentHead = m_head.load(std::memory_order_relaxed);
	size_t currentTail = m_tail.load(std::memory_order_relaxed);

	// Calculate size with wraparound handling
	if (currentTail >= currentHead)
	{
		return currentTail - currentHead;
	}
	else
	{
		return m_capacity - (currentHead - currentTail);
	}
}

//----------------------------------------------------------------------------------------------------
// IsEmpty (Monitoring API)
//
// Returns true if queue appears empty.
// Value may change immediately after call due to concurrent producer.
//----------------------------------------------------------------------------------------------------
bool CallbackQueue::IsEmpty() const
{
	size_t currentHead = m_head.load(std::memory_order_relaxed);
	size_t currentTail = m_tail.load(std::memory_order_relaxed);
	return currentHead == currentTail;
}

//----------------------------------------------------------------------------------------------------
// IsFull (Monitoring API)
//
// Returns true if queue appears full.
// Value may change immediately after call due to concurrent consumer.
//----------------------------------------------------------------------------------------------------
bool CallbackQueue::IsFull() const
{
	size_t currentTail = m_tail.load(std::memory_order_relaxed);
	size_t nextTail    = NextIndex(currentTail);
	size_t currentHead = m_head.load(std::memory_order_relaxed);
	return nextTail == currentHead;
}

//----------------------------------------------------------------------------------------------------
// Implementation Notes
//
// Lock-Free Progress Guarantee:
//   - Enqueue() always completes in bounded time (no blocking)
//   - DequeueAll() always completes in bounded time (no blocking)
//   - No mutex, no conditional wait, no priority inversion
//
// Backpressure Strategy (Phase 2):
//   - When queue full, Enqueue() returns false immediately
//   - Producer (C++ main thread) should log warning and drop callback
//   - Future optimization: Dynamic capacity expansion or priority queue
//
// Performance Profiling Hooks:
//   - m_totalEnqueued / m_totalDequeued for throughput monitoring
//   - GetApproximateSize() for queue depth monitoring
//   - Can add high-resolution timer in Enqueue() for latency profiling
//
// Memory Safety:
//   - Ring buffer allocated in constructor, deallocated in destructor
//   - No dangling pointers (buffer lifetime = queue lifetime)
//   - CallbackData is copyable, no ownership issues
//
// Thread Safety Validation:
//   - Run under Thread Sanitizer (TSan) to detect data races
//   - Atomic operations should satisfy happens-before relationships
//   - Cache-line padding should prevent false sharing
//
// Callback Ownership Model:
//   - JavaScript owns callback functions in g_pendingCallbacks Map
//   - C++ only stores callback IDs (uint64_t), not v8::Function objects
//   - This ensures thread safety - no v8::Function across frames
//----------------------------------------------------------------------------------------------------
