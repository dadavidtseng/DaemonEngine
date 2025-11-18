//----------------------------------------------------------------------------------------------------
// RenderCommandQueue.hpp
// Phase 1: Async Architecture - Lock-Free SPSC Command Queue
//
// Purpose:
//   Thread-safe, lock-free Single-Producer-Single-Consumer (SPSC) ring buffer for
//   JavaScript worker thread → Main render thread communication.
//
// Design Rationale:
//   - SPSC over MPMC: Simpler, faster (single writer = JavaScript worker)
//   - Ring buffer over linked list: Cache-friendly, bounded memory
//   - Lock-free over mutex: Predictable latency, no priority inversion
//   - Bounded capacity: Backpressure prevents memory runaway
//
// Thread Safety Model:
//   - Producer (JS Worker): Writes to m_tail, reads m_head (atomic)
//   - Consumer (Main Thread): Writes to m_head, reads m_tail (atomic)
//   - Cache-line separation: Prevents false sharing between head/tail
//
// Performance Characteristics:
//   - Submission: O(1), lock-free, < 0.5ms latency
//   - Consumption: O(n) where n = commands per frame (typically 10-50)
//   - Memory: Fixed 72 KB (1000 commands × ~72 bytes)
//
// Author: Phase 1 - Async Architecture Implementation
// Date: 2025-10-17
//----------------------------------------------------------------------------------------------------

#pragma once

//----------------------------------------------------------------------------------------------------
// Suppress C4324: Structure padding warning for cache-line alignment
// This warning is intentional - we WANT the compiler to add padding for optimal performance
#pragma warning(push)
#pragma warning(disable: 4324)

//----------------------------------------------------------------------------------------------------
#include "Engine/Renderer/RenderCommand.hpp"

#include <atomic>
#include <cstddef>
#include <functional>

//----------------------------------------------------------------------------------------------------
// RenderCommandQueue
//
// Lock-free SPSC ring buffer for asynchronous render commands.
//
// Usage Pattern:
//
// Producer (JavaScript Worker Thread):
//   bool submitted = queue->Submit(command);
//   if (!submitted) {
//       // Queue full - backpressure triggered
//       // Either drop command or wait/retry
//   }
//
// Consumer (Main Render Thread):
//   queue->ConsumeAll([](RenderCommand const& cmd) {
//       ProcessCommand(cmd);  // Process each command
//   });
//
// Capacity Management:
//   - Default: 1000 commands (configurable via constructor)
//   - Full queue → Submit() returns false (backpressure)
//   - Empty queue → ConsumeAll() returns immediately
//
// Thread Safety Guarantees:
//   - Single producer, single consumer
//   - Lock-free progress guarantee
//   - No blocking operations (conditional wait/notify not required)
//----------------------------------------------------------------------------------------------------
class RenderCommandQueue
{
public:
	//------------------------------------------------------------------------------------------------
	// Constants
	//------------------------------------------------------------------------------------------------
	static constexpr size_t DEFAULT_CAPACITY    = 1000;  // 1000 commands ≈ 72 KB
	static constexpr size_t CACHE_LINE_SIZE     = 64;    // Modern CPU cache line size

	//------------------------------------------------------------------------------------------------
	// Construction / Destruction
	//------------------------------------------------------------------------------------------------
	explicit RenderCommandQueue(size_t capacity = DEFAULT_CAPACITY);
	~RenderCommandQueue();

	// Non-copyable, non-movable (contains atomic members)
	RenderCommandQueue(RenderCommandQueue const&)            = delete;
	RenderCommandQueue& operator=(RenderCommandQueue const&) = delete;
	RenderCommandQueue(RenderCommandQueue&&)                 = delete;
	RenderCommandQueue& operator=(RenderCommandQueue&&)      = delete;

	//------------------------------------------------------------------------------------------------
	// Producer API (JavaScript Worker Thread)
	//------------------------------------------------------------------------------------------------

	// Submit a command to the queue (non-blocking)
	// Returns:
	//   true  - Command successfully enqueued
	//   false - Queue full (backpressure triggered)
	//
	// Thread Safety: Safe to call from single producer thread only
	// Performance: O(1), lock-free, < 0.5ms latency
	bool Submit(RenderCommand const& command);

	// Get current queue size (approximate, for monitoring only)
	// Warning: Value may be stale due to concurrent consumer
	size_t GetApproximateSize() const;

	//------------------------------------------------------------------------------------------------
	// Consumer API (Main Render Thread)
	//------------------------------------------------------------------------------------------------

	// Consume all available commands using a callback processor
	// The processor is called for each command in FIFO order
	//
	// Template Processor Signature:
	//   void Processor(RenderCommand const& cmd)
	//
	// Example:
	//   queue->ConsumeAll([](RenderCommand const& cmd) {
	//       switch (cmd.type) {
	//           case RenderCommandType::CREATE_MESH:
	//               // Handle mesh creation
	//               break;
	//           // ...
	//       }
	//   });
	//
	// Thread Safety: Safe to call from single consumer thread only
	// Performance: O(n) where n = number of commands in queue
	template <typename ProcessorFunc>
	void ConsumeAll(ProcessorFunc&& processor);

	// Get queue capacity (fixed at construction)
	size_t GetCapacity() const { return m_capacity; }

	//------------------------------------------------------------------------------------------------
	// Monitoring / Debugging
	//------------------------------------------------------------------------------------------------

	// Check if queue is empty (approximate, may change immediately after call)
	bool IsEmpty() const;

	// Check if queue is full (approximate, may change immediately after call)
	bool IsFull() const;

	// Get total commands submitted since creation (atomic counter)
	uint64_t GetTotalSubmitted() const { return m_totalSubmitted.load(std::memory_order_relaxed); }

	// Get total commands consumed since creation (atomic counter)
	uint64_t GetTotalConsumed() const { return m_totalConsumed.load(std::memory_order_relaxed); }

private:
	//------------------------------------------------------------------------------------------------
	// Ring Buffer Implementation
	//------------------------------------------------------------------------------------------------

	// Ring buffer storage (dynamically allocated array)
	RenderCommand* m_buffer = nullptr;
	size_t         m_capacity;  // Power of 2 preferred for modulo optimization

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
	std::atomic<uint64_t> m_totalSubmitted = 0;  // Total commands submitted (overflow expected)
	std::atomic<uint64_t> m_totalConsumed  = 0;   // Total commands consumed (overflow expected)

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
void RenderCommandQueue::ConsumeAll(ProcessorFunc&& processor)
{
	// Load current consumer position (relaxed ordering sufficient for SPSC)
	size_t currentHead = m_head.load(std::memory_order_relaxed);

	// Load current producer position (acquire ordering to synchronize with producer's release)
	size_t currentTail = m_tail.load(std::memory_order_acquire);

	// Process all commands from head to tail
	while (currentHead != currentTail)
	{
		// Read command from buffer
		RenderCommand const& command = m_buffer[currentHead];

		// Invoke processor callback
		processor(command);

		// Advance head index
		currentHead = NextIndex(currentHead);

		// Increment consumption counter
		m_totalConsumed.fetch_add(1, std::memory_order_relaxed);
	}

	// Update consumer head position (release ordering to synchronize with producer's acquire)
	m_head.store(currentHead, std::memory_order_release);
}

//----------------------------------------------------------------------------------------------------
// Design Notes
//
// Memory Ordering Rationale:
//   - m_tail.load (acquire): Ensures commands written by producer are visible to consumer
//   - m_head.store (release): Ensures consumer's updates are visible to producer
//   - Counters use relaxed: Statistics only, no synchronization required
//
// Backpressure Handling:
//   - When queue full, Submit() returns false immediately (no blocking)
//   - Producer must handle backpressure (drop, wait, or buffer elsewhere)
//   - Phase 1: Drop commands if full (logged as warning)
//   - Future: Dynamic capacity expansion or priority queue
//
// Cache-Line Padding:
//   - m_head and m_tail separated by full cache line (64 bytes)
//   - Prevents false sharing (CPU cache coherency thrashing)
//   - Critical for lock-free performance on multi-core systems
//
// Capacity Considerations:
//   - 1000 commands ≈ 72 KB memory overhead (acceptable)
//   - Typical frame: 10-50 commands (5% capacity usage)
//   - Burst tolerance: 500 commands (50% capacity)
//   - Full queue indicates JavaScript producing faster than C++ consuming
//
// Performance Validation (Phase 1 Acceptance Criteria):
//   - Submit latency: < 0.5ms (measured via high-resolution timer)
//   - No dropped commands under typical load (< 100 commands/frame)
//   - Stable 60 FPS rendering regardless of JavaScript execution time
//----------------------------------------------------------------------------------------------------

// Restore warning settings
#pragma warning(pop)
