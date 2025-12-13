//----------------------------------------------------------------------------------------------------
// AudioStateBuffer.hpp
// Engine Audio Module - Audio State Double-Buffered Container
//
// Purpose:
//   Typedef specialization of StateBuffer<T> for audio state management.
//   Provides double-buffered audio state storage for async architecture.
//
// Design Rationale:
//   - Specialization of generic StateBuffer template
//   - No additional functionality beyond StateBuffer<AudioStateMap>
//   - Simplifies type usage throughout codebase
//
// Usage:
//   AudioStateBuffer* buffer = new AudioStateBuffer();
//   AudioStateMap* backBuffer = buffer->GetBackBuffer();
//   (*backBuffer)[soundId] = AudioState(...);
//   buffer->SwapBuffers();
//
// Thread Safety:
//   - GetFrontBuffer(): Lock-free reads (main thread)
//   - GetBackBuffer(): Lock-free writes (worker thread)
//   - SwapBuffers(): Brief locked operation (main thread only)
//
// Author: Phase 5 - Audio Async Pattern Implementation
// Date: 2025-12-07
//----------------------------------------------------------------------------------------------------

#pragma once

//----------------------------------------------------------------------------------------------------
#include "Engine/Core/StateBuffer.hpp"
#include "Engine/Audio/AudioState.hpp"

//----------------------------------------------------------------------------------------------------
// AudioStateBuffer Type Definition
//
// Double-buffered container for audio state management.
// Specialization of StateBuffer<AudioStateMap> template.
//
// Properties:
//   - Type: StateBuffer<AudioStateMap>
//   - Container: std::unordered_map<SoundID, AudioState>
//   - Thread Safety: Double-buffered for lock-free read/write
//
// Methods (inherited from StateBuffer<T>):
//   - GetFrontBuffer(): Get front buffer for audio processing (main thread, lock-free)
//   - GetBackBuffer(): Get back buffer for writing (worker thread, lock-free)
//   - SwapBuffers(): Swap and copy buffers (main thread, brief lock)
//   - EnableDirtyTracking(bool): Enable O(d) optimization for sparse updates
//   - MarkDirty(SoundID): Mark sound as dirty for per-key copy optimization
//   - GetElementCount(): Get approximate sound count for monitoring
//   - GetTotalSwaps(): Get total swaps performed for profiling
//
// Usage Pattern:
//   Worker Thread (JavaScript/Command Processing):
//     AudioStateMap* backBuffer = buffer->GetBackBuffer();
//     (*backBuffer)[soundId].position = newPosition;
//     (*backBuffer)[soundId].volume = newVolume;
//     buffer->MarkDirty(soundId);  // O(d) optimization
//
//   Main Thread (Frame Boundary):
//     buffer->SwapBuffers();  // Copy only dirty entries when O(d) enabled
//
//   Main Thread (Audio Processing):
//     AudioStateMap const* frontBuffer = buffer->GetFrontBuffer();
//     for (auto const& [id, state] : *frontBuffer) {
//         if (state.isActive && state.isPlaying) ProcessSound(state);
//     }
//----------------------------------------------------------------------------------------------------
using AudioStateBuffer = StateBuffer<AudioStateMap>;

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
//   - Custom AudioStateBuffer class inheriting from StateBuffer<AudioStateMap>
//   - Rejected: Adds no value, creates unnecessary inheritance hierarchy
//
// Performance Characteristics:
//   - Full-copy SwapBuffers(): O(n) where n = active sound count (typically 10-50)
//   - With EnableDirtyTracking(true): O(d) where d = dirty sound count (typically 1-5)
//   - Expected speedup: 10-1000x for sparse updates (typical gameplay scenarios)
//
// Memory Footprint:
//   - Front buffer: ~72 bytes × active sound count
//   - Back buffer: ~72 bytes × active sound count
//   - Dirty tracking set: 8 bytes × dirty sound count (when enabled)
//   - Total overhead: minimal (<1KB for 10 sounds)
//
// Thread Safety Model:
//   - Main thread reads front buffer (lock-free)
//   - Worker thread writes back buffer (lock-free)
//   - SwapBuffers() briefly locks to copy dirty entries (main thread only)
//   - No contention in typical usage (worker and main operate on separate buffers)
//
// Integration Pattern:
//   - AudioCommandQueue → App::ProcessAudioCommands() → AudioStateBuffer back buffer write
//   - App::Update() → SwapBuffers() → synchronize front/back buffers
//   - App::Update() → AudioSystem → AudioStateBuffer front buffer read → FMOD API calls
//
// Future Extensions:
//   - If audio-specific helper methods needed, create wrapper class
//   - Example: GetSoundsByTag(), GetPlayingSounds(), GetSpatialSounds()
//   - For now, simple typedef is sufficient
//----------------------------------------------------------------------------------------------------
