//----------------------------------------------------------------------------------------------------
// EntityID.hpp
// Engine Entity Module - Entity Identifier Type
//
// Purpose:
//   Defines EntityID type for uniquely identifying entities across the engine.
//   Used by Entity, Renderer, and other subsystems for entity references.
//
// Design Rationale:
//   - uint64_t provides 18 quintillion unique IDs (no exhaustion risk)
//   - Simple typedef, no overhead
//   - Compatible with JavaScript number type (safe up to 2^53)
//   - Centralizes entity ID definition for engine-wide consistency
//
// Author: M4-T8 Engine Refactoring
// Date: 2025-10-26
//----------------------------------------------------------------------------------------------------

#pragma once

//----------------------------------------------------------------------------------------------------
#include <cstdint>

//----------------------------------------------------------------------------------------------------
// EntityID Type Definition
//
// Unique identifier for game entities.
// Used across engine subsystems (Entity, Renderer, Physics, etc.)
//
// Properties:
//   - Type: uint64_t (64-bit unsigned integer)
//   - Range: 0 to 18,446,744,073,709,551,615
//   - JavaScript Safe Range: 0 to 9,007,199,254,740,991 (2^53 - 1)
//   - Collision Risk: Negligible with incremental generation
//
// Usage:
//   EntityID id = 12345;
//   std::unordered_map<EntityID, EntityState> entities;
//----------------------------------------------------------------------------------------------------
using EntityID = uint64_t;

//----------------------------------------------------------------------------------------------------
// Design Notes
//
// Why uint64_t?
//   - Large enough for incremental ID generation (trillions of entities)
//   - No GUID overhead (simpler, faster)
//   - Compatible with most serialization formats
//
// JavaScript Compatibility:
//   - JavaScript Number.MAX_SAFE_INTEGER = 2^53 - 1 (9 quadrillion)
//   - Engine unlikely to exceed this in practice
//   - Alternative: Use strings if >2^53 IDs needed (future optimization)
//
// Thread Safety:
//   - ID generation should use atomic increment or main-thread-only generation
//   - EntityID itself is a simple value type (thread-safe to read/copy)
//----------------------------------------------------------------------------------------------------
