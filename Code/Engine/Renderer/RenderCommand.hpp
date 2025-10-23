//----------------------------------------------------------------------------------------------------
// RenderCommand.hpp
// Phase 1: Async Architecture - Render Command Definitions
//
// Purpose:
//   Type-safe command structures for JavaScript → C++ render thread communication.
//   Uses std::variant for zero-cost, type-safe payload storage.
//
// Design Decisions:
//   - std::variant over std::any: Zero-cost abstraction, compile-time type checking
//   - EntityID as uint64_t: JavaScript Number type compatibility (53-bit safe integer)
//   - std::optional for partial updates: Efficient field updates without full state
//   - Rgba8 over Vec4: Memory efficiency (4 bytes vs 16 bytes per color)
//
// Thread Safety:
//   - Immutable after construction (no mutation after submission to queue)
//   - Copyable and movable for queue operations
//
// Author: Phase 1 - Async Architecture Implementation
// Date: 2025-10-17
//----------------------------------------------------------------------------------------------------

#pragma once

//----------------------------------------------------------------------------------------------------
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Core/Rgba8.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <variant>

//----------------------------------------------------------------------------------------------------
// EntityID Type Definition
// - Compatible with JavaScript Number type (53-bit safe integer)
// - Unique identifier for all game entities
//----------------------------------------------------------------------------------------------------
using EntityID = uint64_t;

//----------------------------------------------------------------------------------------------------
// RenderCommandType Enumeration
//
// Defines all async command types supported by the render command queue.
// Each type maps to a corresponding payload structure.
//----------------------------------------------------------------------------------------------------
enum class RenderCommandType : uint8_t
{
	CREATE_MESH,        // Create new mesh entity
	UPDATE_ENTITY,      // Update entity position/orientation/color
	DESTROY_ENTITY,     // Remove entity from rendering
	CREATE_CAMERA,      // Create new camera (Phase 2b)
	UPDATE_CAMERA,      // Update camera position/orientation
	SET_ACTIVE_CAMERA,  // Set which camera is active for rendering
	UPDATE_CAMERA_TYPE, // Change camera type (world/screen)
	DESTROY_CAMERA,     // Remove camera from rendering
	CREATE_LIGHT,       // Create new light source
	UPDATE_LIGHT        // Update light properties
};

//----------------------------------------------------------------------------------------------------
// Command Payload Structures
//
// Each structure contains the minimum data required for the corresponding command.
// Design: Immutable, value-semantic, POD-like for efficient queue storage.
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
// MeshCreationData
// Payload for CREATE_MESH command
//----------------------------------------------------------------------------------------------------
struct MeshCreationData
{
	std::string meshType;  // "cube", "sphere", "grid", etc.
	Vec3        position;
	float       radius;    // Uniform scale
	Rgba8       color;
};

//----------------------------------------------------------------------------------------------------
// EntityUpdateData
// Payload for UPDATE_ENTITY command
// Uses std::optional for partial updates (only update specified fields)
//----------------------------------------------------------------------------------------------------
struct EntityUpdateData
{
	std::optional<Vec3>        position;
	std::optional<EulerAngles> orientation;
	std::optional<Rgba8>       color;
	// Note: Scale not supported in Phase 1 (meshes use fixed radius)
};

//----------------------------------------------------------------------------------------------------
// LightCreationData
// Payload for CREATE_LIGHT command
//----------------------------------------------------------------------------------------------------
struct LightCreationData
{
	Vec3  position;
	Rgba8 color;
	float intensity;  // Light strength multiplier
};

//----------------------------------------------------------------------------------------------------
// LightUpdateData
// Payload for UPDATE_LIGHT command
//----------------------------------------------------------------------------------------------------
struct LightUpdateData
{
	std::optional<Vec3>  position;
	std::optional<Rgba8> color;
	std::optional<float> intensity;
};

//----------------------------------------------------------------------------------------------------
// CameraCreationData
// Payload for CREATE_CAMERA command (Phase 2b)
// Uses position + orientation (matches Camera.hpp SetPositionAndOrientation pattern)
// Type determines camera mode: "world" = Perspective, "screen" = Orthographic
//----------------------------------------------------------------------------------------------------
struct CameraCreationData
{
	Vec3        position;      // World-space position (X-forward, Y-left, Z-up)
	EulerAngles orientation;   // Yaw, Pitch, Roll in degrees
	std::string type;          // "world" (3D perspective) or "screen" (2D orthographic)
};

//----------------------------------------------------------------------------------------------------
// CameraUpdateData
// Payload for UPDATE_CAMERA command
//----------------------------------------------------------------------------------------------------
struct CameraUpdateData
{
	Vec3        position;
	EulerAngles orientation;  // Yaw, Pitch, Roll in degrees
};

//----------------------------------------------------------------------------------------------------
// CameraTypeUpdateData
// Payload for UPDATE_CAMERA_TYPE command (Phase 2b)
// Changes camera mode between "world" (3D perspective) and "screen" (2D orthographic)
//----------------------------------------------------------------------------------------------------
struct CameraTypeUpdateData
{
	std::string type;  // "world" or "screen"
};

//----------------------------------------------------------------------------------------------------
// RenderCommand
//
// Type-safe command structure using std::variant for payload storage.
// Guarantees zero-cost abstraction (no virtual function overhead).
//
// Memory Layout:
//   - type: 1 byte (enum)
//   - entityId: 8 bytes (EntityID)
//   - data: ~56 bytes (largest variant = MeshCreationData with std::string)
//   Total: ~72 bytes per command (aligned to cache line)
//
// Thread Safety:
//   - Immutable after construction
//   - Safe to copy across thread boundaries
//----------------------------------------------------------------------------------------------------
struct RenderCommand
{
	RenderCommandType type;
	EntityID          entityId;  // Target entity (0 for camera)

	// Type-safe payload using std::variant
	// std::monostate for commands without payload (e.g., DESTROY_ENTITY, SET_ACTIVE_CAMERA, DESTROY_CAMERA)
	std::variant<std::monostate,
	             MeshCreationData,
	             EntityUpdateData,
	             CameraCreationData,
	             CameraUpdateData,
	             CameraTypeUpdateData,
	             LightCreationData,
	             LightUpdateData>
	    data;

	// Default constructor (required for array initialization)
	RenderCommand()
	    : type(RenderCommandType::UPDATE_CAMERA)
	    , entityId(0)
	    , data(std::monostate{})
	{
	}

	// Explicit constructor for type safety
	RenderCommand(RenderCommandType cmdType, EntityID id, auto&& payload)
	    : type(cmdType)
	    , entityId(id)
	    , data(std::forward<decltype(payload)>(payload))
	{
	}
};

//----------------------------------------------------------------------------------------------------
// Performance Characteristics (Phase 1 Targets)
//
// Command Size: ~72 bytes (cache-friendly)
// Queue Capacity: 1000 commands × 72 bytes = ~72 KB memory overhead
// Submission Latency: < 0.5ms (lock-free atomic operations)
//
// Expected Command Rates:
//   - Typical frame: 10-50 commands (entity updates, camera)
//   - Burst scenario: 200-500 commands (scene transitions, spawning)
//   - Maximum: 1000 commands (queue full → backpressure)
//----------------------------------------------------------------------------------------------------
