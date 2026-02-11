//----------------------------------------------------------------------------------------------------
// GenericCommandQueue.hpp
// GenericCommand System - Command Queue
//
// Purpose:
//   Thread-safe, lock-free Single-Producer-Single-Consumer (SPSC) ring buffer for
//   JavaScript worker thread → Main render thread GenericCommand transport.
//   Inherits from CommandQueueBase<GenericCommand> template.
//
// Design Rationale:
//   - Inherits SPSC implementation from CommandQueueBase (template does heavy lifting)
//   - Adds OnQueueFull() logging for backpressure monitoring
//   - Follows exact same pattern as RenderCommandQueue and CallbackQueue
//
// Thread Safety Model:
//   - Producer (JS Worker): Calls Submit() to enqueue GenericCommands
//   - Consumer (Main Thread): Calls ConsumeAll() to process GenericCommands
//   - Inherited from CommandQueueBase: Cache-line separated atomic indices
//
// Performance Characteristics:
//   - Submission: O(1), lock-free
//   - Consumption: O(n) where n = commands per frame
//   - Memory: Fixed ~44 KB (500 commands × ~88 bytes per GenericCommand)
//
// Capacity Choice (500):
//   - Between RenderCommandQueue (1000) and CallbackQueue (100)
//   - GenericCommand is larger than RenderCommand (std::any overhead)
//   - 500 commands at 60 FPS = ~30,000 commands/sec throughput ceiling
//   - Typical frame: 10-50 generic commands (under 10% capacity)
//
// Author: GenericCommand System - Phase 2
// Date: 2026-02-10
//----------------------------------------------------------------------------------------------------

#pragma once

//----------------------------------------------------------------------------------------------------
#include "Engine/Core/CommandQueueBase.hpp"
#include "Engine/Core/GenericCommand.hpp"

//----------------------------------------------------------------------------------------------------
// GenericCommandQueue
//
// Lock-free SPSC ring buffer for asynchronous GenericCommand delivery.
// Inherits core SPSC implementation from CommandQueueBase<GenericCommand>.
//
// Usage Pattern:
//
// Producer (JavaScript Worker Thread via GenericCommandScriptInterface):
//   GenericCommand cmd("entity.create", payload, "agent-1", callbackId, callback);
//   bool submitted = queue->Submit(cmd);
//   if (!submitted) {
//       // Queue full - backpressure triggered
//       // Return false to JavaScript
//   }
//
// Consumer (Main Render Thread via App::ProcessGenericCommands):
//   queue->ConsumeAll([&executor](GenericCommand const& cmd) {
//       executor.ExecuteCommand(cmd);
//   });
//----------------------------------------------------------------------------------------------------
class GenericCommandQueue : public CommandQueueBase<GenericCommand>
{
public:
	//------------------------------------------------------------------------------------------------
	// Constants
	//------------------------------------------------------------------------------------------------
	static constexpr size_t DEFAULT_CAPACITY = 500;   // 500 commands ≈ 44 KB

	//------------------------------------------------------------------------------------------------
	// Construction / Destruction
	//------------------------------------------------------------------------------------------------
	explicit GenericCommandQueue(size_t capacity = DEFAULT_CAPACITY);
	~GenericCommandQueue();

	// Non-copyable, non-movable (inherited from base)
	GenericCommandQueue(GenericCommandQueue const&)            = delete;
	GenericCommandQueue& operator=(GenericCommandQueue const&) = delete;
	GenericCommandQueue(GenericCommandQueue&&)                 = delete;
	GenericCommandQueue& operator=(GenericCommandQueue&&)      = delete;

	//------------------------------------------------------------------------------------------------
	// Public API (inherited from CommandQueueBase)
	//------------------------------------------------------------------------------------------------
	// bool Submit(GenericCommand const& command);
	// template <typename ProcessorFunc> void ConsumeAll(ProcessorFunc&& processor);
	// size_t GetApproximateSize() const;
	// size_t GetCapacity() const;
	// bool IsEmpty() const;
	// bool IsFull() const;
	// uint64_t GetTotalSubmitted() const;
	// uint64_t GetTotalConsumed() const;

protected:
	//------------------------------------------------------------------------------------------------
	// Virtual Hooks (Override from CommandQueueBase)
	//------------------------------------------------------------------------------------------------
	// Called when queue is full during Submit()
	void OnQueueFull() override;
};
