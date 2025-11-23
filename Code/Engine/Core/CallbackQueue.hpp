//----------------------------------------------------------------------------------------------------
// CallbackQueue.hpp
// Phase 2: Async Architecture - Lock-Free SPSC Callback Queue
//
// Purpose:
//   Thread-safe, lock-free Single-Producer-Single-Consumer (SPSC) ring buffer for
//   Main render thread (C++) → JavaScript worker thread callback communication.
//
// Design Rationale:
//   - SPSC over MPMC: Simpler, faster (single writer = Main thread)
//   - Ring buffer over linked list: Cache-friendly, bounded memory
//   - Lock-free over mutex: Predictable latency, no priority inversion
//   - Bounded capacity: Backpressure prevents memory runaway
//
// Thread Safety Model:
//   - Producer (Main Thread): Writes to m_tail, reads m_head (atomic)
//   - Consumer (JS Worker): Writes to m_head, reads m_tail (atomic)
//   - Cache-line separation: Prevents false sharing between head/tail
//
// Performance Characteristics:
//   - Enqueue: O(1), lock-free, < 1µs latency
//   - Dequeue: O(n) where n = callbacks per frame (typically 1-10)
//   - Memory: Fixed ~4 KB (100 callbacks × ~40 bytes)
//
// Author: Phase 2 - Async Architecture Implementation
// Date: 2025-11-18
//----------------------------------------------------------------------------------------------------

#pragma once

//----------------------------------------------------------------------------------------------------
// Suppress C4324: Structure padding warning for cache-line alignment
// This warning is intentional - we WANT the compiler to add padding for optimal performance
#pragma warning(push)
#pragma warning(disable: 4324)

//----------------------------------------------------------------------------------------------------
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>

//----------------------------------------------------------------------------------------------------
// CallbackType
//
// Enum for different types of callbacks that can be enqueued.
//----------------------------------------------------------------------------------------------------
enum class CallbackType : uint8_t
{
	ENTITY_CREATED,
	CAMERA_CREATED,
	RESOURCE_LOADED,
	GENERIC
};

//----------------------------------------------------------------------------------------------------
// CallbackData
//
// Data structure for C++→JavaScript callback messages.
// Size: ~40 bytes (8+8+24+4 with padding)
//----------------------------------------------------------------------------------------------------
struct CallbackData
{
	uint64_t     callbackId;      // Unique callback identifier (JavaScript-generated)
	uint64_t     resultId;        // EntityID or CameraID returned from C++
	std::string  errorMessage;    // Empty = success, non-empty = error description
	CallbackType type;            // Type of callback for type-specific handling
};

//----------------------------------------------------------------------------------------------------
// CallbackQueue
//
// Lock-free SPSC ring buffer for asynchronous callback delivery.
//
// Usage Pattern:
//
// Producer (Main Render Thread - C++):
//   CallbackData data{callbackId, entityId, "", CallbackType::ENTITY_CREATED};
//   bool enqueued = queue->Enqueue(data);
//   if (!enqueued) {
//       // Queue full - backpressure triggered
//       // Log warning and continue (callback dropped)
//   }
//
// Consumer (JavaScript Worker Thread):
//   queue->DequeueAll([](CallbackData const& cb) {
//       // Look up callback function in JavaScript Map
//       // Execute callback with resultId
//   });
//
// Capacity Management:
//   - Default: 100 callbacks (configurable via constructor)
//   - Full queue → Enqueue() returns false (backpressure)
//   - Empty queue → DequeueAll() returns immediately
//
// Thread Safety Guarantees:
//   - Single producer (main thread), single consumer (JS worker)
//   - Lock-free progress guarantee
//   - No blocking operations (conditional wait/notify not required)
//----------------------------------------------------------------------------------------------------
class CallbackQueue
{
public:
	//------------------------------------------------------------------------------------------------
	// Constants
	//------------------------------------------------------------------------------------------------
	static constexpr size_t DEFAULT_CAPACITY = 100;   // 100 callbacks ≈ 4 KB
	static constexpr size_t CACHE_LINE_SIZE  = 64;    // Modern CPU cache line size

	//------------------------------------------------------------------------------------------------
	// Construction / Destruction
	//------------------------------------------------------------------------------------------------
	explicit CallbackQueue(size_t capacity = DEFAULT_CAPACITY);
	~CallbackQueue();

	// Non-copyable, non-movable (contains atomic members)
	CallbackQueue(CallbackQueue const&)            = delete;
	CallbackQueue& operator=(CallbackQueue const&) = delete;
	CallbackQueue(CallbackQueue&&)                 = delete;
	CallbackQueue& operator=(CallbackQueue&&)      = delete;

	//------------------------------------------------------------------------------------------------
	// Producer API (Main Render Thread - C++)
	//------------------------------------------------------------------------------------------------

	// Enqueue a callback to the queue (non-blocking)
	// Returns:
	//   true  - Callback successfully enqueued
	//   false - Queue full (backpressure triggered)
	//
	// Thread Safety: Safe to call from single producer thread only
	// Performance: O(1), lock-free, < 1µs latency
	bool Enqueue(CallbackData const& callback);

	// Get current queue size (approximate, for monitoring only)
	// Warning: Value may be stale due to concurrent consumer
	size_t GetApproximateSize() const;

	//------------------------------------------------------------------------------------------------
	// Consumer API (JavaScript Worker Thread)
	//------------------------------------------------------------------------------------------------

	// Dequeue all available callbacks using a callback processor
	// The processor is called for each callback in FIFO order
	//
	// Template Processor Signature:
	//   void Processor(CallbackData const& cb)
	//
	// Example:
	//   queue->DequeueAll([](CallbackData const& cb) {
	//       if (cb.errorMessage.empty()) {
	//           ExecuteCallback(cb.callbackId, cb.resultId);
	//       } else {
	//           LogError("Callback %llu failed: %s", cb.callbackId, cb.errorMessage.c_str());
	//       }
	//   });
	//
	// Thread Safety: Safe to call from single consumer thread only
	// Performance: O(n) where n = number of callbacks in queue
	template <typename ProcessorFunc>
	void DequeueAll(ProcessorFunc&& processor);

	// Get queue capacity (fixed at construction)
	size_t GetCapacity() const { return m_capacity; }

	//------------------------------------------------------------------------------------------------
	// Monitoring / Debugging
	//------------------------------------------------------------------------------------------------

	// Check if queue is empty (approximate, may change immediately after call)
	bool IsEmpty() const;

	// Check if queue is full (approximate, may change immediately after call)
	bool IsFull() const;

	// Get total callbacks enqueued since creation (atomic counter)
	uint64_t GetTotalEnqueued() const { return m_totalEnqueued.load(std::memory_order_relaxed); }

	// Get total callbacks dequeued since creation (atomic counter)
	uint64_t GetTotalDequeued() const { return m_totalDequeued.load(std::memory_order_relaxed); }

private:
	//------------------------------------------------------------------------------------------------
	// Ring Buffer Implementation
	//------------------------------------------------------------------------------------------------

	// Ring buffer storage (dynamically allocated array)
	CallbackData* m_buffer = nullptr;
	size_t        m_capacity;  // Power of 2 preferred for modulo optimization

	//------------------------------------------------------------------------------------------------
	// Atomic Indices (Cache-Line Separated)
	//------------------------------------------------------------------------------------------------

	// Producer writes to m_tail, reads m_head
	// Aligned to cache line to prevent false sharing
	alignas(CACHE_LINE_SIZE) std::atomic<size_t> m_head = 0;  // Consumer write, producer read

	// Consumer writes to m_head, reads m_tail
	// Aligned to cache line to prevent false sharing
	alignas(CACHE_LINE_SIZE) std::atomic<size_t> m_tail = 0;  // Producer write, consumer read

	//------------------------------------------------------------------------------------------------
	// Statistics (Atomic Counters)
	//------------------------------------------------------------------------------------------------
	std::atomic<uint64_t> m_totalEnqueued = 0;  // Total callbacks enqueued (overflow expected)
	std::atomic<uint64_t> m_totalDequeued = 0;  // Total callbacks dequeued (overflow expected)

	//------------------------------------------------------------------------------------------------
	// Helper Methods
	//------------------------------------------------------------------------------------------------

	// Get next index in ring buffer (wraps around at capacity)
	size_t NextIndex(size_t index) const { return (index + 1) % m_capacity; }
};

//----------------------------------------------------------------------------------------------------
// Template Implementation (must be in header for template instantiation)
//----------------------------------------------------------------------------------------------------

template <typename ProcessorFunc>
void CallbackQueue::DequeueAll(ProcessorFunc&& processor)
{
	// Load current consumer position (relaxed ordering sufficient for SPSC)
	size_t currentHead = m_head.load(std::memory_order_relaxed);

	// Load current producer position (acquire ordering to synchronize with producer's release)
	size_t currentTail = m_tail.load(std::memory_order_acquire);

	// Process all callbacks from head to tail
	while (currentHead != currentTail)
	{
		// Read callback from buffer
		CallbackData const& callback = m_buffer[currentHead];

		// Invoke processor callback
		processor(callback);

		// Advance head index
		currentHead = NextIndex(currentHead);

		// Increment dequeue counter
		m_totalDequeued.fetch_add(1, std::memory_order_relaxed);
	}

	// Update consumer head position (release ordering to synchronize with producer's acquire)
	m_head.store(currentHead, std::memory_order_release);
}

//----------------------------------------------------------------------------------------------------
// Design Notes
//
// Memory Ordering Rationale:
//   - m_tail.load (acquire): Ensures callbacks written by producer are visible to consumer
//   - m_head.store (release): Ensures consumer's updates are visible to producer
//   - Counters use relaxed: Statistics only, no synchronization required
//
// Backpressure Handling:
//   - When queue full, Enqueue() returns false immediately (no blocking)
//   - Producer must handle backpressure (drop, wait, or buffer elsewhere)
//   - Phase 2: Drop callbacks if full (logged as warning)
//   - Future: Dynamic capacity expansion or priority queue
//
// Cache-Line Padding:
//   - m_head and m_tail separated by full cache line (64 bytes)
//   - Prevents false sharing (CPU cache coherency thrashing)
//   - Critical for lock-free performance on multi-core systems
//
// Capacity Considerations:
//   - 100 callbacks ≈ 4 KB memory overhead (acceptable)
//   - Typical frame: 1-10 callbacks (< 10% capacity usage)
//   - Burst tolerance: 50 callbacks (50% capacity)
//   - Full queue indicates C++ producing faster than JavaScript consuming
//
// Performance Validation (Phase 2 Acceptance Criteria):
//   - Enqueue latency: < 1µs (measured via high-resolution timer)
//   - No dropped callbacks under typical load (< 10 callbacks/frame)
//   - JavaScript worker thread processes callbacks without blocking C++
//----------------------------------------------------------------------------------------------------

// Restore warning settings
#pragma warning(pop)
