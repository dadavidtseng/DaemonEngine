//----------------------------------------------------------------------------------------------------
// AudioState.hpp
// Engine Audio Module - Audio State Data Structure
//
// Purpose:
//   Defines AudioState struct for representing audio source state in double-buffered system.
//   Used by async architecture to safely communicate audio data between worker and main threads.
//
// Design Rationale:
//   - Plain-Old-Data (POD) struct for efficient double-buffering
//   - Contains all data needed for managing audio playback state
//   - Used with AudioStateBuffer for thread-safe state updates
//   - Memory-efficient design (8 fields, ~72 bytes per instance)
//
// Thread Safety:
//   - AudioState itself is copyable and thread-safe to read
//   - Actual thread safety managed by AudioStateBuffer double-buffering
//   - Worker thread writes to back buffer, main thread reads from front buffer
//
// Author: Phase 5 - Audio Async Pattern Implementation
// Date: 2025-12-07
//----------------------------------------------------------------------------------------------------

#pragma once

//----------------------------------------------------------------------------------------------------
#include "Engine/Audio/AudioTypes.hpp"
#include "Engine/Math/Vec3.hpp"
#include <string>
#include <unordered_map>

//----------------------------------------------------------------------------------------------------
// AudioState Struct
//
// Represents the complete playback state of an audio source.
// Used in double-buffered AudioStateBuffer for async communication between threads.
//
// Design Notes:
//   - soundId: Unique identifier for this sound (from AudioSystem::CreateOrGetSound)
//   - soundPath: File path for loaded audio resource (e.g., "Data/Audio/explosion.mp3")
//   - position: 3D world-space position for spatial audio (X-forward, Y-left, Z-up)
//   - volume: Playback volume (0.0 = silent, 1.0 = full volume, clamped by AudioSystem)
//   - isPlaying: Current playback state (true = actively playing, false = stopped)
//   - isLooped: Loop flag (true = continuous loop, false = one-shot playback)
//   - isLoaded: Load state (true = ready for playback, false = still loading)
//   - isActive: Active flag (true = valid entry, false = marked for removal)
//
// Usage:
//   AudioState state;
//   state.soundId = 42;
//   state.soundPath = "Data/Audio/music.mp3";
//   state.position = Vec3(10.0f, 0.0f, 2.0f);
//   state.volume = 0.8f;
//   state.isPlaying = true;
//----------------------------------------------------------------------------------------------------
struct AudioState
{
	SoundID     soundId;      // Unique sound identifier (uint64_t from AudioTypes.hpp)
	std::string soundPath;    // File path to audio resource (relative to Run/ directory)
	Vec3        position;     // 3D world-space position (X-forward, Y-left, Z-up)
	float       volume;       // Playback volume (0.0 - 1.0, clamped by AudioSystem)
	bool        isPlaying;    // Playback state (true = playing, false = stopped)
	bool        isLooped;     // Loop flag (true = loop continuously, false = one-shot)
	bool        isLoaded;     // Load state (true = ready, false = loading)
	bool        isActive;     // Active flag (true = valid, false = marked for removal)

	// Default constructor (safe initial state)
	AudioState()
		: soundId(0)
		, soundPath("")
		, position(Vec3::ZERO)
		, volume(1.0f)
		, isPlaying(false)
		, isLooped(false)
		, isLoaded(false)
		, isActive(true)  // New entries are active by default
	{
	}

	// Explicit constructor (for command processing)
	AudioState(SoundID id, std::string const& path, Vec3 const& pos, float vol, bool playing, bool looped, bool loaded, bool active = true)
		: soundId(id)
		, soundPath(path)
		, position(pos)
		, volume(vol)
		, isPlaying(playing)
		, isLooped(looped)
		, isLoaded(loaded)
		, isActive(active)
	{
	}
};

//----------------------------------------------------------------------------------------------------
// AudioStateMap Type Definition
//
// Efficient container for mapping SoundID to AudioState.
// Used in AudioStateBuffer for managing all audio source states.
//
// Properties:
//   - Fast lookup by SoundID (O(1) hash table)
//   - Efficient iteration over all audio sources
//   - Used in double-buffering system for thread-safe audio state management
//
// Usage:
//   AudioStateMap audioSources;
//   audioSources[soundId] = AudioState(soundId, "Data/Audio/sound.mp3", Vec3::ZERO, 1.0f, true, false, true);
//----------------------------------------------------------------------------------------------------
using AudioStateMap = std::unordered_map<SoundID, AudioState>;

//----------------------------------------------------------------------------------------------------
// Design Notes
//
// Why Plain Struct?
//   - No virtual functions (no vtable overhead)
//   - Copyable and movable (efficient for double-buffering)
//   - Simple memory layout (cache-friendly, ~72 bytes)
//
// Why std::string for soundPath?
//   - Phase 5 simplicity (flexible file path handling)
//   - Matches AudioCommand::SoundLoadData pattern
//   - String overhead acceptable for typical audio source counts (10-50 active sounds)
//
// Why separate isPlaying/isLooped/isLoaded/isActive flags?
//   - Clear semantic meaning for each state dimension
//   - Easy to query individual state aspects
//   - Future extensions: add isPaused, isFadingOut, etc.
//
// Why float volume instead of int (0-255)?
//   - Matches FMOD audio system API (0.0f - 1.0f range)
//   - Avoids integer/float conversion in hot path
//   - Sufficient precision for audio volume control
//
// Why Vec3 position instead of separate x/y/z?
//   - Consistency with EntityState position field
//   - Efficient 3D math operations (vector operations)
//   - Matches existing engine Vec3 usage patterns
//
// Memory Layout (x64 platform, approximate):
//   - soundId: 8 bytes (uint64_t)
//   - soundPath: 32 bytes (std::string SSO, typical implementation)
//   - position: 12 bytes (Vec3 = 3 floats)
//   - volume: 4 bytes (float)
//   - isPlaying/isLooped/isLoaded/isActive: 4 bytes (4 bools, compiler padding)
//   - Total: ~60-72 bytes per AudioState (cache-friendly)
//
// Thread Safety Guarantees:
//   - AudioState struct is POD-like (safe to copy across threads)
//   - No internal pointers or shared state (each instance independent)
//   - soundPath uses std::string (internally manages memory, safe to copy)
//   - AudioStateBuffer manages thread safety through double-buffering
//----------------------------------------------------------------------------------------------------
