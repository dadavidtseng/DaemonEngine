//----------------------------------------------------------------------------------------------------
// HandlerResult.hpp
// GenericCommand System - Handler Result Structure
//
// Purpose:
//   Structured return value from GenericCommand handlers. Carries a key-value data map
//   (type-erased via std::any) and an error message for failure reporting.
//   Used by GenericCommandExecutor to deliver results back to JavaScript via CallbackQueue.
//
// Design Decisions:
//   - std::unordered_map<String, std::any> over single std::any: Handlers return structured
//     results with named fields (e.g., {"entityId": 42, "position": Vec3(1,2,3)}), which
//     maps naturally to JavaScript objects at the ScriptInterface boundary (task 3.1)
//   - No V8 dependency: V8 object creation from HandlerResult is handled in
//     GenericCommandScriptInterface (anti-corruption layer pattern)
//   - Factory methods over constructors: Explicit Success/Error semantics, consistent with
//     ScriptMethodResult pattern in Engine/Script/ScriptCommon.hpp
//   - Empty error string = success: Simple boolean-equivalent check without extra field
//
// Supported std::any Value Types (for ScriptInterface V8 conversion):
//   - int, float, double
//   - String (std::string)
//   - uint64_t (EntityID, CameraID)
//   - Vec3 (3D vector)
//   - bool
//
// Thread Safety:
//   - Immutable after factory method construction
//   - Copyable for CallbackQueue SPSC ring buffer operations
//   - std::any and std::unordered_map handle their own deep copy semantics
//
// Memory Layout:
//   - data: ~56 bytes (std::unordered_map overhead, empty)
//   - error: ~32 bytes (SSO std::string)
//   Total: ~88 bytes per result (varies by data content)
//
// Author: GenericCommand System - Phase 1
// Date: 2026-02-10
//----------------------------------------------------------------------------------------------------

#pragma once

//----------------------------------------------------------------------------------------------------
#include "Engine/Core/StringUtils.hpp"
//----------------------------------------------------------------------------------------------------
#include <any>
#include <unordered_map>

//----------------------------------------------------------------------------------------------------
// HandlerResult
//
// Type-erased result from a GenericCommand handler.
// Carries structured key-value data for success, or an error message for failure.
//
// Usage:
//   // Success with data
//   HandlerResult result = HandlerResult::Success({
//       {"entityId", std::any(entityId)},
//       {"position", std::any(position)}
//   });
//
//   // Success without data (acknowledgement)
//   HandlerResult result = HandlerResult::Success();
//
//   // Error
//   HandlerResult result = HandlerResult::Error("Entity not found");
//
// The GenericCommandScriptInterface converts data values to V8 objects
// for JavaScript callback delivery.
//----------------------------------------------------------------------------------------------------
struct HandlerResult
{
	std::unordered_map<String, std::any> data;   // Key-value result data (empty for error or ack)
	String                               error;  // Error message (empty = success)

	// Check if this result represents a successful operation
	bool IsSuccess() const { return error.empty(); }

	// Check if this result represents a failed operation
	bool IsError() const { return !error.empty(); }

	// Factory method: Create a success result with optional data
	static HandlerResult Success(std::unordered_map<String, std::any> resultData = {});

	// Factory method: Create an error result with a descriptive message
	static HandlerResult Error(String const& message);
};
