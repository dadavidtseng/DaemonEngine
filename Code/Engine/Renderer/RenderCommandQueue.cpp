//----------------------------------------------------------------------------------------------------
// RenderCommandQueue.cpp
// Phase 1: Async Architecture - Lock-Free SPSC Command Queue Implementation
//----------------------------------------------------------------------------------------------------

#include "Engine/Renderer/RenderCommandQueue.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/LogSubsystem.hpp"
#include "Engine/Core/EngineCommon.hpp"

//----------------------------------------------------------------------------------------------------
// Constructor
//
// Allocates ring buffer with specified capacity.
// Initializes atomic indices and statistics counters.
//----------------------------------------------------------------------------------------------------
RenderCommandQueue::RenderCommandQueue(size_t capacity)
    : m_buffer(nullptr)
    , m_capacity(capacity)
    , m_head(0)
    , m_tail(0)
    , m_totalSubmitted(0)
    , m_totalConsumed(0)
{
	// Validate capacity (must be > 0)
	if (capacity == 0)
	{
		ERROR_AND_DIE("RenderCommandQueue: Capacity must be greater than zero");
	}

	// Allocate ring buffer storage
	m_buffer = new RenderCommand[m_capacity];

	DAEMON_LOG(LogRenderer, eLogVerbosity::Log,
	           Stringf("RenderCommandQueue: Initialized with capacity %llu (%.2f KB)",
	                   static_cast<uint64_t>(m_capacity),
	                   (m_capacity * sizeof(RenderCommand)) / 1024.0f));
}

//----------------------------------------------------------------------------------------------------
// Destructor
//
// Deallocates ring buffer storage.
// Logs statistics for debugging/profiling.
//----------------------------------------------------------------------------------------------------
RenderCommandQueue::~RenderCommandQueue()
{
	// Log final statistics
	uint64_t totalSubmitted = m_totalSubmitted.load(std::memory_order_relaxed);
	uint64_t totalConsumed  = m_totalConsumed.load(std::memory_order_relaxed);

	DAEMON_LOG(LogRenderer, eLogVerbosity::Log,
	           Stringf("RenderCommandQueue: Shutdown - Total submitted: %llu, Total consumed: %llu, Lost: %llu",
	                   totalSubmitted,
	                   totalConsumed,
	                   totalSubmitted - totalConsumed));

	// Deallocate buffer
	delete[] m_buffer;
	m_buffer = nullptr;
}

//----------------------------------------------------------------------------------------------------
// Submit (Producer API)
//
// Enqueues a command to the ring buffer (non-blocking, lock-free).
//
// Algorithm:
//   1. Load current tail (producer write position)
//   2. Calculate next tail position
//   3. Load current head (consumer read position) with acquire semantics
//   4. Check if queue is full (next tail == head)
//   5. If not full: Write command, advance tail with release semantics
//
// Memory Ordering:
//   - m_head.load (acquire): Ensures consumer's updates are visible to producer
//   - m_tail.store (release): Ensures command data is visible to consumer before tail update
//
// Returns:
//   true  - Command successfully enqueued
//   false - Queue full (backpressure)
//----------------------------------------------------------------------------------------------------
bool RenderCommandQueue::Submit(RenderCommand const& command)
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

	// Write command to buffer
	m_buffer[currentTail] = command;

	// Update producer tail position (release ordering to ensure command data is visible)
	m_tail.store(nextTail, std::memory_order_release);

	// Increment submission counter
	m_totalSubmitted.fetch_add(1, std::memory_order_relaxed);

	return true;
}

//----------------------------------------------------------------------------------------------------
// GetApproximateSize (Monitoring API)
//
// Returns current queue size (difference between tail and head).
// Value may be stale due to concurrent consumer, use for monitoring only.
//----------------------------------------------------------------------------------------------------
size_t RenderCommandQueue::GetApproximateSize() const
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
bool RenderCommandQueue::IsEmpty() const
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
bool RenderCommandQueue::IsFull() const
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
//   - Submit() always completes in bounded time (no blocking)
//   - ConsumeAll() always completes in bounded time (no blocking)
//   - No mutex, no conditional wait, no priority inversion
//
// Backpressure Strategy (Phase 1):
//   - When queue full, Submit() returns false immediately
//   - Producer (JavaScript) should log warning and drop command
//   - Future optimization: Dynamic capacity expansion or priority queue
//
// Performance Profiling Hooks:
//   - m_totalSubmitted / m_totalConsumed for throughput monitoring
//   - GetApproximateSize() for queue depth monitoring
//   - Can add high-resolution timer in Submit() for latency profiling
//
// Memory Safety:
//   - Ring buffer allocated in constructor, deallocated in destructor
//   - No dangling pointers (buffer lifetime = queue lifetime)
//   - RenderCommand is copyable, no ownership issues
//
// Thread Safety Validation:
//   - Run under Thread Sanitizer (TSan) to detect data races
//   - Atomic operations should satisfy happens-before relationships
//   - Cache-line padding should prevent false sharing
//----------------------------------------------------------------------------------------------------
