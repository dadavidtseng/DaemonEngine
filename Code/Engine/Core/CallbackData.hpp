//----------------------------------------------------------------------------------------------------
// CallbackData.hpp
// Phase 1: Async Architecture - Callback Data Structures
//
// Purpose:
//   Data structures for C++→JavaScript callback communication.
//   Extracted from CallbackQueue.hpp to support template-based command queue refactoring.
//
// Author: Phase 1 - Command Queue Refactoring
// Date: 2025-11-30
//----------------------------------------------------------------------------------------------------

#pragma once

//----------------------------------------------------------------------------------------------------
#include <cstdint>
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
