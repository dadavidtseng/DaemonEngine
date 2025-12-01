//----------------------------------------------------------------------------------------------------
// CallbackQueue.hpp
// Phase 1: Command Queue Refactoring - Callback Queue
//
// Purpose:
//   Thread-safe, lock-free Single-Producer-Single-Consumer (SPSC) ring buffer for
//   Main render thread (C++) → JavaScript worker thread callback communication.
//   Now inherits from CommandQueueBase<CallbackData> template.
//
// Design Rationale:
//   - Inherits SPSC implementation from CommandQueueBase (eliminates ~150 lines)
//   - Maintains Enqueue/DequeueAll API for backward compatibility
//   - Enqueue() wraps Submit(), DequeueAll() wraps ConsumeAll()
//
// Thread Safety Model:
//   - Producer (Main Thread): Calls Enqueue() to queue callbacks
//   - Consumer (JS Worker): Calls DequeueAll() to process callbacks
//   - Inherited from CommandQueueBase: Cache-line separated atomic indices
//
// Performance Characteristics:
//   - Enqueue: O(1), lock-free, < 1µs latency
//   - Dequeue: O(n) where n = callbacks per frame (typically 1-10)
//   - Memory: Fixed ~4 KB (100 callbacks × ~40 bytes)
//
// Author: Phase 1 - Command Queue Refactoring
// Date: 2025-11-30
//----------------------------------------------------------------------------------------------------

#pragma once

//----------------------------------------------------------------------------------------------------
#include "Engine/Core/CommandQueueBase.hpp"
#include "Engine/Core/CallbackData.hpp"

#include <functional>

//----------------------------------------------------------------------------------------------------
// CallbackQueue
//
// Lock-free SPSC ring buffer for asynchronous callback delivery.
// Inherits core SPSC implementation from CommandQueueBase<CallbackData>.
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
//   - Inherited from CommandQueueBase: Single producer, single consumer
//   - Lock-free progress guarantee
//   - No blocking operations (conditional wait/notify not required)
//----------------------------------------------------------------------------------------------------
class CallbackQueue : public CommandQueueBase<CallbackData>
{
public:
	//------------------------------------------------------------------------------------------------
	// Constants
	//------------------------------------------------------------------------------------------------
	static constexpr size_t DEFAULT_CAPACITY = 100;   // 100 callbacks ≈ 4 KB

	//------------------------------------------------------------------------------------------------
	// Construction / Destruction
	//------------------------------------------------------------------------------------------------
	explicit CallbackQueue(size_t capacity = DEFAULT_CAPACITY);
	~CallbackQueue();

	// Non-copyable, non-movable (inherited from base)
	CallbackQueue(CallbackQueue const&)            = delete;
	CallbackQueue& operator=(CallbackQueue const&) = delete;
	CallbackQueue(CallbackQueue&&)                 = delete;
	CallbackQueue& operator=(CallbackQueue&&)      = delete;

	//------------------------------------------------------------------------------------------------
	// Producer API (Main Render Thread - C++) - Wrappers for compatibility
	//------------------------------------------------------------------------------------------------

	// Enqueue a callback to the queue (non-blocking)
	// Wraps CommandQueueBase::Submit() for backward API compatibility
	// Returns:
	//   true  - Callback successfully enqueued
	//   false - Queue full (backpressure triggered)
	//
	// Thread Safety: Safe to call from single producer thread only
	// Performance: O(1), lock-free, < 1µs latency
	bool Enqueue(CallbackData const& callback) { return Submit(callback); }

	//------------------------------------------------------------------------------------------------
	// Consumer API (JavaScript Worker Thread) - Wrappers for compatibility
	//------------------------------------------------------------------------------------------------

	// Dequeue all available callbacks using a callback processor
	// Wraps CommandQueueBase::ConsumeAll() for backward API compatibility
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
	void DequeueAll(ProcessorFunc&& processor) { ConsumeAll(std::forward<ProcessorFunc>(processor)); }

	//------------------------------------------------------------------------------------------------
	// Monitoring API - Wrappers for compatibility
	//------------------------------------------------------------------------------------------------

	// Get total callbacks enqueued since creation (wraps GetTotalSubmitted)
	uint64_t GetTotalEnqueued() const { return GetTotalSubmitted(); }

	// Get total callbacks dequeued since creation (wraps GetTotalConsumed)
	uint64_t GetTotalDequeued() const { return GetTotalConsumed(); }

	//------------------------------------------------------------------------------------------------
	// Inherited Public API (from CommandQueueBase)
	//------------------------------------------------------------------------------------------------
	// bool Submit(CallbackData const& callback);               ← Used by Enqueue()
	// template <typename ProcessorFunc> void ConsumeAll(...);  ← Used by DequeueAll()
	// size_t GetApproximateSize() const;                       ← Direct inheritance
	// size_t GetCapacity() const;                              ← Direct inheritance
	// bool IsEmpty() const;                                    ← Direct inheritance
	// bool IsFull() const;                                     ← Direct inheritance
	// uint64_t GetTotalSubmitted() const;                      ← Wrapped by GetTotalEnqueued()
	// uint64_t GetTotalConsumed() const;                       ← Wrapped by GetTotalDequeued()
};

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
