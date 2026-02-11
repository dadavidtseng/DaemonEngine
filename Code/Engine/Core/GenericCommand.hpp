//----------------------------------------------------------------------------------------------------
// GenericCommand.hpp
// GenericCommand System - Core Data Structure
//
// Purpose:
//   Type-erased command structure for JavaScript → C++ communication via GenericCommandQueue.
//   Replaces the need for per-subsystem ScriptInterface classes by carrying a flexible payload
//   that handlers can interpret at runtime.
//
// Design Decisions:
//   - std::any over v8::Persistent: Keeps Core module V8-free, consistent with RenderCommand pattern
//   - std::any over std::variant: Payload schema is runtime-defined (not compile-time enumerable)
//   - String type field: O(1) handler lookup via std::unordered_map in GenericCommandExecutor
//   - Optional callback: Fire-and-forget commands have callbackId = 0
//   - Default constructor required for CommandQueueBase ring buffer array initialization
//
// Thread Safety:
//   - Immutable after construction (no mutation after submission to queue)
//   - Copyable for CommandQueueBase SPSC ring buffer operations
//   - std::any handles its own deep copy semantics
//
// Memory Layout:
//   - type: ~32 bytes (SSO std::string)
//   - payload: ~64 bytes (std::any with small buffer optimization)
//   - agentId: ~32 bytes (SSO std::string)
//   - callbackId: 8 bytes (uint64_t)
//   - callback: ~64 bytes (std::any)
//   - timestamp: 8 bytes (uint64_t)
//   Total: ~208 bytes per command (varies by payload size)
//
// Author: GenericCommand System - Phase 1
// Date: 2026-02-10
//----------------------------------------------------------------------------------------------------

#pragma once

//----------------------------------------------------------------------------------------------------
#include "Engine/Core/StringUtils.hpp"
//----------------------------------------------------------------------------------------------------
#include <any>
#include <chrono>

//----------------------------------------------------------------------------------------------------
// GenericCommand
//
// Type-erased command for the GenericCommandQueue SPSC ring buffer.
// Carries a string-based type identifier, a flexible std::any payload,
// and an optional callback for async result delivery.
//
// Usage:
//   GenericCommand cmd("CreateMesh", meshData, "agent-01", callbackId, callback);
//   queue->Submit(cmd);
//
// The GenericCommandExecutor dispatches commands to registered handlers
// based on the type field.
//----------------------------------------------------------------------------------------------------
struct GenericCommand
{
	String   type;         // Command type identifier for handler lookup (e.g., "CreateMesh")
	std::any payload;      // Type-erased payload data (interpreted by handler)
	String   agentId;      // Submitting agent identifier (for rate limiting and audit)
	uint64_t callbackId;   // Callback identifier (0 = no callback, fire-and-forget)
	std::any callback;     // Optional callback function (ScriptCallback, stored as std::any)
	uint64_t timestamp;    // Submission timestamp in milliseconds (for audit trail)

	// Default constructor (required for CommandQueueBase ring buffer array initialization)
	GenericCommand()
	    : callbackId(0)
	    , timestamp(0)
	{
	}

	// Explicit constructor for command creation with callback
	GenericCommand(String const& commandType,
	               std::any      commandPayload,
	               String const& submittingAgentId,
	               uint64_t      cmdCallbackId = 0,
	               std::any      cmdCallback   = std::any{})
	    : type(commandType)
	    , payload(std::move(commandPayload))
	    , agentId(submittingAgentId)
	    , callbackId(cmdCallbackId)
	    , callback(std::move(cmdCallback))
	    , timestamp(static_cast<uint64_t>(
	          std::chrono::duration_cast<std::chrono::milliseconds>(
	              std::chrono::steady_clock::now().time_since_epoch())
	              .count()))
	{
	}

	// Check if this command has a callback
	bool HasCallback() const { return callbackId != 0; }
};
