//----------------------------------------------------------------------------------------------------
// CameraState.hpp
// Engine Renderer Module - Camera State Data Structure
//
// Purpose:
//   Defines CameraState struct for representing camera render state in double-buffered system.
//   Used by async architecture to safely communicate camera data between worker and main threads.
//
// Design Rationale:
//   - Value-semantic struct (copyable, no pointers)
//   - Stores Camera configuration, not Camera* pointers
//   - Supports both perspective (world) and orthographic (screen) cameras
//   - Used with StateBuffer<CameraStateMap> for thread-safe state updates
//
// Thread Safety:
//   - CameraState itself is copyable and thread-safe to read
//   - Actual thread safety managed by StateBuffer double-buffering
//   - Worker thread writes to back buffer, main thread reads from front buffer
//
// Author: M4-T8 Engine Refactoring
// Date: 2025-10-26
//----------------------------------------------------------------------------------------------------

#pragma once

//----------------------------------------------------------------------------------------------------
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Entity/EntityID.hpp"
#include <string>
#include <unordered_map>

//----------------------------------------------------------------------------------------------------
// CameraState Struct
//
// Snapshot of camera rendering state.
// Immutable after creation (value-semantic, copyable).
//
// Design Notes:
//   - type: "world" (perspective) or "screen" (orthographic)
//   - isActive: Whether this camera is currently rendering
//   - Camera mode auto-configured based on type during creation
//   - Stores Camera configuration, not Camera* pointer (value semantics)
//
// Camera Types:
//   - "world": 3D perspective camera for world-space rendering
//   - "screen": 2D orthographic camera for screen-space rendering (UI, HUD)
//
// Usage:
//   CameraState state;
//   state.position = Vec3(0.0f, 0.0f, 10.0f);
//   state.type = "world";
//   state.perspectiveFOV = 75.0f;
//----------------------------------------------------------------------------------------------------
struct CameraState
{
	Vec3        position;     // World-space position
	EulerAngles orientation;  // World-space rotation (yaw, pitch, roll in degrees)
	std::string type;         // "world" (3D perspective) or "screen" (2D orthographic)
	bool        isActive;     // Active flag (true = rendering, false = inactive)

	// Camera mode configuration (auto-set based on type)
	Camera::Mode mode;        // eMode_Perspective or eMode_Orthographic

	// Perspective camera properties (for "world" type)
	float perspectiveFOV;     // Field of view in degrees (default: 60.0f)
	float perspectiveAspect;  // Aspect ratio (default: 16/9)
	float perspectiveNear;    // Near plane (default: 0.1f)
	float perspectiveFar;     // Far plane (default: 100.0f)

	// Orthographic camera properties (for "screen" type)
	float orthoLeft;          // Left bound (default: 0.0f)
	float orthoBottom;        // Bottom bound (default: 0.0f)
	float orthoRight;         // Right bound (default: 1920.0f)
	float orthoTop;           // Top bound (default: 1080.0f)
	float orthoNear;          // Near plane (default: 0.0f)
	float orthoFar;           // Far plane (default: 1.0f)

	// Viewport configuration (normalized 0-1 coordinates)
	AABB2 viewport;           // Viewport bounds (default: full screen AABB2(0, 0, 1, 1))

	// Default constructor (perspective camera at origin)
	CameraState()
	    : position(Vec3::ZERO)
	    , orientation(EulerAngles::ZERO)
	    , type("world")
	    , isActive(false)
	    , mode(Camera::eMode_Perspective)
	    , perspectiveFOV(60.0f)
	    , perspectiveAspect(16.0f / 9.0f)
	    , perspectiveNear(0.1f)
	    , perspectiveFar(100.0f)
	    , orthoLeft(0.0f)
	    , orthoBottom(0.0f)
	    , orthoRight(1920.0f)
	    , orthoTop(1080.0f)
	    , orthoNear(0.0f)
	    , orthoFar(1.0f)
	    , viewport(0.0f, 0.0f, 1.0f, 1.0f)  // Full screen viewport (normalized)
	{
	}

	// Explicit constructor
	CameraState(Vec3 const& pos, EulerAngles const& orient, std::string const& camType)
	    : position(pos)
	    , orientation(orient)
	    , type(camType)
	    , isActive(false)
	    , viewport(0.0f, 0.0f, 1.0f, 1.0f)  // Full screen viewport (normalized)
	{
		// Auto-configure camera mode and properties based on type
		if (camType == "world")
		{
			mode = Camera::eMode_Perspective;
			perspectiveFOV = 60.0f;
			perspectiveAspect = 16.0f / 9.0f;
			perspectiveNear = 0.1f;
			perspectiveFar = 100.0f;
		}
		else if (camType == "screen")
		{
			mode = Camera::eMode_Orthographic;
			orthoLeft = 0.0f;
			orthoBottom = 0.0f;
			orthoRight = 1920.0f;
			orthoTop = 1080.0f;
			orthoNear = 0.0f;
			orthoFar = 1.0f;
		}
	}
};

//----------------------------------------------------------------------------------------------------
// CameraStateMap Type Definition
//
// Efficient container for mapping CameraID to CameraState.
// Used in StateBuffer<CameraStateMap> for managing all camera states.
//
// Properties:
//   - Fast lookup by CameraID (O(1) hash table)
//   - Efficient iteration over all cameras
//   - Used in double-buffering system for thread-safe camera state management
//
// Usage:
//   CameraStateMap cameras;
//   cameras[cameraId] = CameraState(pos, orient, "world");
//----------------------------------------------------------------------------------------------------
using CameraStateMap = std::unordered_map<EntityID, CameraState>;

//----------------------------------------------------------------------------------------------------
// Design Notes
//
// Why Value Semantics?
//   - No Camera* pointers (avoids lifetime management issues)
//   - Copyable and movable (efficient for double-buffering)
//   - Simple memory layout (cache-friendly)
//
// Why std::string for type?
//   - Simple API for JavaScript camera creation
//   - Easy to extend with new camera types
//   - String overhead acceptable for camera counts
//
// Why Separate Perspective and Orthographic Properties?
//   - Explicit configuration for each mode
//   - No ambiguity about which properties are used
//   - Clear API for setting camera parameters
//
// Why AABB2 viewport?
//   - Normalized coordinates (0-1 range) for resolution independence
//   - Supports split-screen rendering (multiple viewports)
//   - Standard representation across engine systems
//
// Camera ID vs Entity ID:
//   - EntityID reused as CameraID (same type)
//   - Cameras can be attached to entities (same ID space)
//   - Simplifies engine-wide ID management
//----------------------------------------------------------------------------------------------------
