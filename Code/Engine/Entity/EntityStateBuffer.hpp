//----------------------------------------------------------------------------------------------------
// EntityStateBuffer.hpp
// Engine Entity Module - Entity State Double-Buffered Container
//
// Purpose:
//   Typedef specialization of StateBuffer<T> for entity state management.
//   Provides double-buffered entity state storage for async architecture.
//
// Design Rationale:
//   - Specialization of generic StateBuffer template
//   - No additional functionality beyond StateBuffer<EntityStateMap>
//   - Simplifies type usage throughout codebase
//
// Usage:
//   EntityStateBuffer* buffer = new EntityStateBuffer();
//   EntityStateMap* backBuffer = buffer->GetBackBuffer();
//   (*backBuffer)[entityId] = EntityState(...);
//   buffer->SwapBuffers();
//
// Thread Safety:
//   - GetFrontBuffer(): Lock-free reads (main thread)
//   - GetBackBuffer(): Lock-free writes (worker thread)
//   - SwapBuffers(): Brief locked operation (main thread only)
//
// Author: M4-T8 Engine Refactoring
// Date: 2025-10-26
//----------------------------------------------------------------------------------------------------

#pragma once

//----------------------------------------------------------------------------------------------------
#include "Engine/Core/StateBuffer.hpp"
#include "Engine/Entity/EntityState.hpp"

//----------------------------------------------------------------------------------------------------
// EntityStateBuffer Type Definition
//
// Double-buffered container for entity state management.
// Specialization of StateBuffer<EntityStateMap> template.
//
// Properties:
//   - Type: StateBuffer<EntityStateMap>
//   - Container: std::unordered_map<EntityID, EntityState>
//   - Thread Safety: Double-buffered for lock-free read/write
//
// Methods (inherited from StateBuffer<T>):
//   - GetFrontBuffer(): Get front buffer for rendering (main thread, lock-free)
//   - GetBackBuffer(): Get back buffer for writing (worker thread, lock-free)
//   - SwapBuffers(): Swap and copy buffers (main thread, brief lock)
//   - GetElementCount(): Get approximate element count for monitoring
//   - GetTotalSwaps(): Get total swaps performed for profiling
//
// Usage Pattern:
//   Worker Thread:
//     EntityStateMap* backBuffer = buffer->GetBackBuffer();
//     (*backBuffer)[entityId].position = newPosition;
//
//   Main Thread (Frame Boundary):
//     buffer->SwapBuffers();
//
//   Main Thread (Rendering):
//     EntityStateMap const* frontBuffer = buffer->GetFrontBuffer();
//     for (auto const& [id, state] : *frontBuffer) {
//         if (state.isActive) RenderEntity(state);
//     }
//----------------------------------------------------------------------------------------------------
using EntityStateBuffer = StateBuffer<EntityStateMap>;

//----------------------------------------------------------------------------------------------------
// Design Notes
//
// Why Typedef Instead of Class?
//   - No additional functionality needed beyond StateBuffer<T>
//   - Avoids code duplication
//   - Cleaner inheritance model (uses template specialization)
//   - Easier to maintain (single source of truth)
//
// Alternative Considered:
//   - Custom EntityStateBuffer class inheriting from StateBuffer<EntityStateMap>
//   - Rejected: Adds no value, creates unnecessary inheritance hierarchy
//
// Future Extensions:
//   - If game-specific helper methods needed, create wrapper class
//   - Example: GetPlayerEntity(), GetEntitiesByTag(), etc.
//   - For now, simple typedef is sufficient
//----------------------------------------------------------------------------------------------------
