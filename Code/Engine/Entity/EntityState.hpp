//----------------------------------------------------------------------------------------------------
// EntityState.hpp
// Engine Entity Module - Entity State Data Structure
//
// Purpose:
//   Defines EntityState struct for representing entity render state in double-buffered system.
//   Used by async architecture to safely communicate entity data between worker and main threads.
//
// Design Rationale:
//   - Plain-Old-Data (POD) struct for efficient double-buffering
//   - Contains all data needed for rendering an entity
//   - Used with EntityStateBuffer for thread-safe state updates
//   - Memory-efficient design (color as Rgba8, radius as single float)
//
// Thread Safety:
//   - EntityState itself is copyable and thread-safe to read
//   - Actual thread safety managed by EntityStateBuffer double-buffering
//   - Worker thread writes to back buffer, main thread reads from front buffer
//
// Author: M4-T8 Engine Refactoring
// Date: 2025-10-26
//----------------------------------------------------------------------------------------------------

#pragma once

//----------------------------------------------------------------------------------------------------
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Entity/EntityID.hpp"
#include <string>
#include <unordered_map>

//----------------------------------------------------------------------------------------------------
// EntityState Struct
//
// Represents the complete render state of a game entity.
// Used in double-buffered EntityStateBuffer for async communication between threads.
//
// Design Notes:
//   - position: World-space position for rendering
//   - orientation: World-space rotation (yaw, pitch, roll in degrees)
//   - color: RGBA color (4 bytes, memory efficient)
//   - radius: Uniform scale (single float, no separate scale vector)
//   - meshType: Simple string-based mesh selection ("cube", "sphere", "grid", etc.)
//   - isActive: Active flag (true = render, false = skip)
//   - cameraType: Camera type for entity-based camera selection ("world" or "screen")
//
// Usage:
//   EntityState state;
//   state.position = Vec3(0.0f, 0.0f, 0.0f);
//   state.color = Rgba8::RED;
//   state.meshType = "cube";
//----------------------------------------------------------------------------------------------------
struct EntityState
{
    Vec3        position;          // World-space position
    EulerAngles orientation;       // World-space rotation (yaw, pitch, roll in degrees)
    Rgba8       color;             // RGBA color (4 bytes, memory efficient)
    float       radius;            // Uniform scale (single float, no separate scale vector)
    std::string meshType;          // "cube", "sphere", "grid", etc. (Phase 1 simplicity)
    bool        isActive;          // Active flag (true = render, false = skip)
    std::string cameraType;        // Phase 2: Camera type for entity-based camera selection ("world" or "screen")

    // Default constructor (identity state)
    EntityState()
        : position(Vec3::ZERO),
          orientation(EulerAngles::ZERO),
          color(Rgba8::WHITE),
          radius(1.0f),
          meshType("cube"),
          isActive(true),
          cameraType("world")  // Default to world camera (3D entities)
    {
    }

    // Explicit constructor
    EntityState(Vec3 const& pos, EulerAngles const& orient, Rgba8 const& col, float r, std::string const& type, bool active = true, std::string const& camType = "world")
        : position(pos),
          orientation(orient),
          color(col),
          radius(r),
          meshType(type),
          isActive(active),
          cameraType(camType)  // Default to world camera if not specified
    {
    }
};

//----------------------------------------------------------------------------------------------------
// EntityStateMap Type Definition
//
// Efficient container for mapping EntityID to EntityState.
// Used in EntityStateBuffer for managing all entity states.
//
// Properties:
//   - Fast lookup by EntityID (O(1) hash table)
//   - Efficient iteration over all entities
//   - Used in double-buffering system for thread-safe entity state management
//
// Usage:
//   EntityStateMap entities;
//   entities[entityId] = EntityState(pos, orient, color, radius, "cube");
//----------------------------------------------------------------------------------------------------
using EntityStateMap = std::unordered_map<EntityID, EntityState>;

//----------------------------------------------------------------------------------------------------
// Design Notes
//
// Why Plain Struct?
//   - No virtual functions (no vtable overhead)
//   - Copyable and movable (efficient for double-buffering)
//   - Simple memory layout (cache-friendly)
//
// Why std::string for meshType?
//   - Phase 1 simplicity (easy to add new mesh types from JavaScript)
//   - Can be replaced with enum in Phase 2 for performance
//   - String overhead acceptable for current entity counts
//
// Why Rgba8 instead of Vec4?
//   - Memory efficiency (4 bytes vs 16 bytes)
//   - Matches GPU color format (unsigned byte RGBA)
//   - Sufficient precision for rendering colors
//
// Why single radius instead of Vec3 scale?
//   - Uniform scaling sufficient for Phase 1 entities
//   - Can add non-uniform scale in Phase 2 if needed
//   - Simpler API for JavaScript entity creation
//----------------------------------------------------------------------------------------------------
