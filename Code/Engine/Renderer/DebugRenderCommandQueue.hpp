// //----------------------------------------------------------------------------------------------------
// // DebugRenderCommandQueue.hpp
// // Phase 4: Debug Rendering Command Queue System
// //
// // Purpose:
// //   Provides a lock-free SPSC (Single-Producer-Single-Consumer) command queue specifically for
// //   debug rendering primitives. Inherits from CommandQueueBase<DebugRenderCommand> to handle
// //   efficient batch submission of debug visualization commands from game loop to render thread.
// //
// // Design Philosophy:
// //   - Lock-free SPSC queue: Optimal performance for game loop → render thread communication
// //   - Fixed capacity (500 commands): Prevents unbounded memory growth
// //   - Type-safe command handling: Each command is immutable after submission
// //   - Efficient memory layout: Cache-line aligned for optimal performance
// //   - Zero-cost abstraction: No virtual dispatch in hot path
// //
// // Inheritance Model:
// //   DebugRenderCommandQueue : public CommandQueueBase<DebugRenderCommand>
// //   - Inherits Submit() for producer (game loop)
// //   - Inherits ConsumeAll() for consumer (render thread)
// //   - Inherits lock-free SPSC ring buffer implementation
// //
// // Usage Example:
// //   // Game loop thread (producer)
// //   DebugRenderCommandQueue debugQueue(500);
// //
// //   sDebugRenderCommand cmd;
// //   cmd.type = eDebugRenderCommandType::ADD_WORLD_LINE;
// //   cmd.color = Rgba8(255, 0, 0, 255);
// //   cmd.positions[0] = Vec3(0, 0, 0);
// //   cmd.positions[1] = Vec3(1, 1, 1);
// //   cmd.duration = 5.0f;
// //   debugQueue.Submit(cmd);  // Non-blocking, lock-free submission
// //
// //   // Render thread (consumer)
// //   debugQueue.ConsumeAll([](DebugRenderCommand const& cmd) {
// //       RenderDebugPrimitive(cmd);
// //   });
// //
// // Thread Safety:
// //   - Single producer (game loop): Calls Submit()
// //   - Single consumer (render thread): Calls ConsumeAll()
// //   - Lock-free: No mutexes, no atomics contention (producer/consumer on different cache lines)
// //   - Atomic counters: Only updated by respective thread (producer writes m_tail, consumer writes m_head)
// //
// // Performance Characteristics:
// //   - Submit: O(1) lock-free, < 1µs latency, non-blocking
// //   - ConsumeAll: O(n) where n = number of pending commands (typically 10-100)
// //   - Memory: Fixed 500 × sizeof(DebugRenderCommand) ≈ 80 KB
// //   - Cache friendliness: Ring buffer layout + cache-line aligned head/tail
// //
// // Backpressure Handling:
// //   - If queue is full, Submit() returns false (game loop can discard or retry)
// //   - Prevents memory exhaustion and ensures bounded latency
// //   - Recommended: Check return value and handle queue full gracefully
// //
// // Author: Phase 4 - Debug Rendering System
// // Date: 2025-12-02
// //----------------------------------------------------------------------------------------------------
//
// #pragma once
//
// #include "Engine/Core/CommandQueueBase.hpp"
// #include "Engine/Renderer/DebugRenderCommand.hpp"
//
// //----------------------------------------------------------------------------------------------------
// // DebugRenderCommandQueue
// //
// // Lock-free SPSC queue for debug rendering commands.
// // Inherits all queue functionality from CommandQueueBase template.
// //
// // Capacity: 500 commands (sufficient for typical frame's debug visuals)
// // Each command: ~144 bytes → Total: ~72 KB memory footprint
// //
// // API (inherited from CommandQueueBase<DebugRenderCommand>):
// //   - Submit(cmd): Add command to queue (producer thread)
// //   - ConsumeAll(processor): Process all pending commands (consumer thread)
// //   - GetCapacity(): Returns 500
// //   - GetApproximateSize(): Returns current queue size (for monitoring)
// //   - IsEmpty(): Returns true if queue is empty
// //   - IsFull(): Returns true if queue is full
// //----------------------------------------------------------------------------------------------------
// class DebugRenderCommandQueue : public CommandQueueBase<DebugRenderCommand>
// {
// public:
// 	//------------------------------------------------------------------------------------------------
// 	// Constants
// 	//------------------------------------------------------------------------------------------------
// 	static constexpr size_t DEFAULT_CAPACITY = 500;  // 500 debug commands per frame
//
// 	//------------------------------------------------------------------------------------------------
// 	// Construction / Destruction
// 	//------------------------------------------------------------------------------------------------
//
// 	// Constructor: Creates queue with default capacity (500 commands)
// 	explicit DebugRenderCommandQueue(size_t capacity = DEFAULT_CAPACITY);
//
// 	// Destructor: Deallocates ring buffer and any pending commands
// 	~DebugRenderCommandQueue() override = default;
//
// 	// Non-copyable, non-movable (inherited from CommandQueueBase)
// 	DebugRenderCommandQueue(DebugRenderCommandQueue const&)            = delete;
// 	DebugRenderCommandQueue& operator=(DebugRenderCommandQueue const&) = delete;
// 	DebugRenderCommandQueue(DebugRenderCommandQueue&&)                 = delete;
// 	DebugRenderCommandQueue& operator=(DebugRenderCommandQueue&&)      = delete;
//
// 	//------------------------------------------------------------------------------------------------
// 	// Producer API (Game Loop Thread)
// 	//------------------------------------------------------------------------------------------------
//
// 	// Submit a debug command to the queue
// 	// Parameters:
// 	//   command - The debug render command to submit (copied into queue)
// 	// Returns:
// 	//   true  - Command successfully submitted to queue
// 	//   false - Queue full (backpressure), command not submitted
// 	//
// 	// Thread Safety: Safe to call from game loop thread only
// 	// Performance: O(1), lock-free, non-blocking
// 	//
// 	// Example:
// 	//   sDebugRenderCommand cmd;
// 	//   cmd.type = eDebugRenderCommandType::ADD_WORLD_SPHERE;
// 	//   cmd.positions[0] = entityPos;
// 	//   cmd.radius = 2.0f;
// 	//   cmd.color = Rgba8(0, 255, 0, 255);
// 	//   cmd.duration = 0.016f;  // One frame
// 	//   if (!debugQueue->Submit(cmd)) {
// 	//       DebuggerPrintf("Warning: Debug command queue full\n");
// 	//   }
// 	//
// 	// Note: Inherited from CommandQueueBase<DebugRenderCommand>
// 	using CommandQueueBase<DebugRenderCommand>::Submit;
//
// 	//------------------------------------------------------------------------------------------------
// 	// Consumer API (Render Thread)
// 	//------------------------------------------------------------------------------------------------
//
// 	// Process all pending debug commands
// 	// Parameters:
// 	//   processor - Callback function invoked for each command in FIFO order
// 	//
// 	// Processor Signature:
// 	//   void processor(DebugRenderCommand const& cmd)
// 	//
// 	// Thread Safety: Safe to call from render thread only
// 	// Performance: O(n) where n = number of pending commands
// 	//
// 	// Example:
// 	//   debugQueue->ConsumeAll([this](DebugRenderCommand const& cmd) {
// 	//       ProcessDebugCommand(cmd);  // Render primitive based on command type
// 	//   });
// 	//
// 	// Note: Inherited from CommandQueueBase<DebugRenderCommand>
// 	using CommandQueueBase<DebugRenderCommand>::ConsumeAll;
//
// 	//------------------------------------------------------------------------------------------------
// 	// Monitoring API
// 	//------------------------------------------------------------------------------------------------
//
// 	// Get approximate number of pending commands in queue
// 	// Warning: Value may be stale due to concurrent producer/consumer
// 	// Returns: Approximate count of commands in queue
// 	// Note: Inherited from CommandQueueBase
// 	using CommandQueueBase<DebugRenderCommand>::GetApproximateSize;
//
// 	// Get fixed queue capacity
// 	// Returns: 500 (number of commands the queue can hold)
// 	// Note: Inherited from CommandQueueBase
// 	using CommandQueueBase<DebugRenderCommand>::GetCapacity;
//
// 	// Check if queue is empty
// 	// Returns: true if no pending commands
// 	// Note: Inherited from CommandQueueBase
// 	using CommandQueueBase<DebugRenderCommand>::IsEmpty;
//
// 	// Check if queue is full
// 	// Returns: true if all capacity is used
// 	// Note: Inherited from CommandQueueBase
// 	using CommandQueueBase<DebugRenderCommand>::IsFull;
//
// protected:
// 	//------------------------------------------------------------------------------------------------
// 	// Virtual Hooks (Optional Customization)
// 	//------------------------------------------------------------------------------------------------
//
// 	// Called when a command is successfully submitted
// 	// Override to add logging, statistics, or validation
// 	// Default: Does nothing
// 	virtual void OnSubmit(DebugRenderCommand const& cmd) {}
//
// 	// Called when a command is consumed (processed)
// 	// Override to add metrics, validation, or cleanup
// 	// Default: Does nothing
// 	virtual void OnConsume(DebugRenderCommand const& cmd) {}
//
// 	// Called when queue is full and a command cannot be submitted
// 	// Override to implement backpressure handling strategies
// 	// Default: Does nothing
// 	virtual void OnQueueFull() {}
// };
//
// //----------------------------------------------------------------------------------------------------
// // Implementation Notes
// //
// // Lock-Free Design:
// //   - Ring buffer with atomic head/tail pointers
// //   - Producer only modifies m_tail, consumer only modifies m_head
// //   - Cache-line separation prevents false sharing
// //   - Memory ordering: acquire/release for head/tail synchronization
// //
// // Capacity Management:
// //   - Fixed 500-command capacity chosen for typical debug rendering needs
// //   - Sufficient for 100+ debug primitives per frame (spheres, lines, text)
// //   - Bounded memory: ~72 KB regardless of frame complexity
// //
// // Performance Characteristics:
// //   - Submit: < 1 microsecond (one atomic store, one pointer increment)
// //   - ConsumeAll: Processes all commands in cache-friendly linear order
// //   - No dynamic allocation (single malloc at construction)
// //   - No virtual dispatch in hot path (inherited interface)
// //
// // Thread Safety Guarantees:
// //   - SPSC model: Exactly one producer (game loop), one consumer (render thread)
// //   - Lock-free: No blocking operations, no priority inversion
// //   - Atomic synchronization: Proper memory ordering for correctness
// //   - No shared mutable state except ring buffer (protected by atomics)
// //
// // Initialization:
// //   - Derived constructor calls base constructor with capacity
// //   - Ring buffer allocated on heap (single allocation)
// //   - All counters initialized to zero
// //   - Ready for use immediately after construction
// //
// // Backpressure Strategy:
// //   - If queue full, Submit() returns false (non-blocking)
// //   - Game loop can:
// //     a) Discard debug command (continue without visualization)
// //     b) Retry next frame
// //     c) Log warning and adjust debug volume
// //   - Prevents catastrophic failure if game generates too many debug commands
// //
// // Future Enhancements:
// //   - OnSubmit() hooks for per-command metrics
// //   - OnQueueFull() hooks for backpressure handling
// //   - Priority-based sorting of commands
// //   - Compression of common command types
// //   - Per-category filtering (enable/disable by primitive type)
// //----------------------------------------------------------------------------------------------------
