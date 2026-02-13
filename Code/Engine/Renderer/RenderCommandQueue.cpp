// //----------------------------------------------------------------------------------------------------
// // RenderCommandQueue.cpp
// // Phase 1: Command Queue Refactoring - Render Command Queue Implementation
// //----------------------------------------------------------------------------------------------------
//
// #include "Engine/Renderer/RenderCommandQueue.hpp"
//
// #include "Engine/Core/ErrorWarningAssert.hpp"
// #include "Engine/Core/LogSubsystem.hpp"
// #include "Engine/Core/EngineCommon.hpp"
//
// //----------------------------------------------------------------------------------------------------
// // Constructor
// //
// // Initializes CommandQueueBase with specified capacity.
// // Logs queue initialization for monitoring.
// //----------------------------------------------------------------------------------------------------
// RenderCommandQueue::RenderCommandQueue(size_t const capacity)
// 	: CommandQueueBase<RenderCommand>(capacity)  // Initialize base template
// {
// 	// Validate capacity (must be > 0)
// 	if (capacity == 0)
// 	{
// 		ERROR_AND_DIE("RenderCommandQueue: Capacity must be greater than zero");
// 	}
//
// 	DAEMON_LOG(LogRenderer, eLogVerbosity::Log,
// 	           Stringf("RenderCommandQueue: Initialized with capacity %llu (%.2f KB)",
// 	               static_cast<uint64_t>(capacity),
// 	               (capacity * sizeof(RenderCommand)) / 1024.f));
// }
//
// //----------------------------------------------------------------------------------------------------
// // Destructor
// //
// // Logs final statistics for debugging/profiling.
// // Base class (CommandQueueBase) handles buffer deallocation.
// //----------------------------------------------------------------------------------------------------
// RenderCommandQueue::~RenderCommandQueue()
// {
// 	// Log final statistics
// 	uint64_t totalSubmitted = GetTotalSubmitted();
// 	uint64_t totalConsumed  = GetTotalConsumed();
//
// 	DAEMON_LOG(LogRenderer, eLogVerbosity::Log,
// 	           Stringf("RenderCommandQueue: Shutdown - Total submitted: %llu, Total consumed: %llu, Lost: %llu",
// 	               totalSubmitted,
// 	               totalConsumed,
// 	               totalSubmitted - totalConsumed));
// }
//
// //----------------------------------------------------------------------------------------------------
// // OnQueueFull (Virtual Hook Override)
// //
// // Called by CommandQueueBase::Submit() when queue is full.
// // Logs warning for monitoring/debugging.
// //----------------------------------------------------------------------------------------------------
// void RenderCommandQueue::OnQueueFull()
// {
// 	DAEMON_LOG(LogRenderer, eLogVerbosity::Warning,
// 	           Stringf("RenderCommandQueue: Queue full! Capacity: %llu, Submitted: %llu, Consumed: %llu",
// 	               static_cast<uint64_t>(GetCapacity()),
// 	               GetTotalSubmitted(),
// 	               GetTotalConsumed()));
// }
//
// //----------------------------------------------------------------------------------------------------
// // Implementation Notes
// //
// // Template Inheritance Benefits:
// //   - Eliminates ~200 lines of duplicate SPSC implementation (Submit, ConsumeAll, monitoring)
// //   - All lock-free ring buffer logic inherited from CommandQueueBase<RenderCommand>
// //   - Zero performance overhead (template instantiation at compile time)
// //   - Maintains exact same public API (zero breaking changes)
// //
// // Lock-Free Progress Guarantee (Inherited from CommandQueueBase):
// //   - Submit() always completes in bounded time (no blocking)
// //   - ConsumeAll() always completes in bounded time (no blocking)
// //   - No mutex, no conditional wait, no priority inversion
// //
// // Backpressure Strategy:
// //   - When queue full, Submit() returns false immediately
// //   - OnQueueFull() hook logs warning for monitoring
// //   - Producer (JavaScript) should handle backpressure (drop or retry)
// //
// // Performance Profiling:
// //   - GetTotalSubmitted() / GetTotalConsumed() for throughput monitoring
// //   - GetApproximateSize() for queue depth monitoring
// //   - OnSubmit() / OnConsume() hooks available for custom profiling
// //
// // Memory Safety (Inherited from CommandQueueBase):
// //   - Ring buffer allocated in base constructor, deallocated in base destructor
// //   - No dangling pointers (buffer lifetime = queue lifetime)
// //   - RenderCommand is copyable, no ownership issues
// //
// // Thread Safety Validation:
// //   - Run under Thread Sanitizer (TSan) to detect data races
// //   - Atomic operations satisfy happens-before relationships
// //   - Cache-line padding prevents false sharing
// //----------------------------------------------------------------------------------------------------
