//----------------------------------------------------------------------------------------------------
// AudioCommand.hpp
// Phase 2: Audio Command Queue - Audio Command Definitions
//
// Purpose:
//   Type-safe command structures for JavaScript → C++ audio thread communication.
//   Uses std::variant for zero-cost, type-safe payload storage.
//
// Design Decisions:
//   - std::variant over std::any: Zero-cost abstraction, compile-time type checking
//   - SoundID as uint64_t: JavaScript Number type compatibility (53-bit safe integer)
//   - std::optional for partial updates: Efficient field updates without full state
//   - Vec3 for 3D audio positioning: World-space coordinates (X-forward, Y-left, Z-up)
//
// Thread Safety:
//   - Immutable after construction (no mutation after submission to queue)
//   - Copyable and movable for queue operations
//
// Author: Phase 2 - Audio Command Queue Implementation
// Date: 2025-11-30
//----------------------------------------------------------------------------------------------------

#pragma once

//----------------------------------------------------------------------------------------------------
#include "Engine/Audio/AudioTypes.hpp"
#include "Engine/Math/Vec3.hpp"

#include <optional>
#include <string>
#include <variant>

//----------------------------------------------------------------------------------------------------
// AudioCommandType Enumeration
//
// Defines all async command types supported by the audio command queue.
// Each type maps to a corresponding payload structure.
//
// Command Flow:
//   JavaScript → AudioCommandQueue → AudioSystem (C++ main thread)
//
// Usage Examples:
//   LOAD_SOUND:        Load audio file asynchronously, returns SoundID via callback
//   PLAY_SOUND:        Start playback of loaded sound (returns PlaybackID)
//   STOP_SOUND:        Stop active playback
//   SET_VOLUME:        Adjust global or per-sound volume (0.0 - 1.0)
//   UPDATE_3D_POSITION: Update 3D spatial audio position for sound source
//----------------------------------------------------------------------------------------------------
enum class AudioCommandType : uint8_t
{
	LOAD_SOUND,         // Load audio file from disk (async operation)
	PLAY_SOUND,         // Play loaded sound (one-shot or looped)
	STOP_SOUND,         // Stop active sound playback
	SET_VOLUME,         // Set volume for sound or master volume
	UPDATE_3D_POSITION  // Update 3D spatial position of sound source
};

//----------------------------------------------------------------------------------------------------
// Command Payload Structures
//
// Each structure contains the minimum data required for the corresponding command.
// Design: Immutable, value-semantic, POD-like for efficient queue storage.
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
// SoundLoadData
// Payload for LOAD_SOUND command
//
// Usage:
//   AudioCommand cmd{AudioCommandType::LOAD_SOUND, 0,
//                    SoundLoadData{"Data/Audio/explosion.mp3", callbackId}};
//
// Behavior:
//   - AudioSystem loads sound file from soundPath (relative to Run/ directory)
//   - On success: Callback invoked with resultId = SoundID (unique identifier)
//   - On failure: Callback invoked with errorMessage (file not found, invalid format, etc.)
//----------------------------------------------------------------------------------------------------
struct SoundLoadData
{
	std::string soundPath;    // Relative path to audio file (e.g., "Data/Audio/music.mp3")
	uint64_t    callbackId;   // JavaScript callback ID for async result notification
};

//----------------------------------------------------------------------------------------------------
// SoundPlayData
// Payload for PLAY_SOUND command
//
// Usage:
//   AudioCommand cmd{AudioCommandType::PLAY_SOUND, soundId,
//                    SoundPlayData{1.0f, false, Vec3{0.f, 0.f, 0.f}}};
//
// Behavior:
//   - Starts playback of sound identified by SoundID (from LOAD_SOUND result)
//   - volume: 0.0 (silent) to 1.0 (full volume), clamped automatically
//   - looped: true = continuous loop, false = play once and stop
//   - position: 3D world-space position for spatial audio (Vec3::ZERO for non-spatial)
//----------------------------------------------------------------------------------------------------
struct SoundPlayData
{
	float volume;      // Playback volume (0.0 - 1.0)
	bool  looped;      // true = loop continuously, false = one-shot playback
	Vec3  position;    // 3D world-space position (X-forward, Y-left, Z-up)
};

//----------------------------------------------------------------------------------------------------
// SoundStopData
// Payload for STOP_SOUND command
//
// Usage:
//   AudioCommand cmd{AudioCommandType::STOP_SOUND, soundId, SoundStopData{}};
//
// Behavior:
//   - Stops all active playback instances of the specified sound
//   - If sound is not playing, command is silently ignored (no error)
//   - Immediate stop (no fade-out in Phase 2, future enhancement)
//----------------------------------------------------------------------------------------------------
struct SoundStopData
{
	// Empty payload - SoundID in RenderCommand.entityId field identifies target sound
	// (Using empty struct for consistency with command pattern, may add fadeOutTime in future)
};

//----------------------------------------------------------------------------------------------------
// VolumeUpdateData
// Payload for SET_VOLUME command
//
// Usage (per-sound volume):
//   AudioCommand cmd{AudioCommandType::SET_VOLUME, soundId, VolumeUpdateData{0.5f}};
//
// Usage (master volume):
//   AudioCommand cmd{AudioCommandType::SET_VOLUME, 0, VolumeUpdateData{0.8f}};
//
// Behavior:
//   - If soundId != 0: Set volume for specific sound (affects all playback instances)
//   - If soundId == 0: Set master volume (global multiplier for all sounds)
//   - volume: 0.0 (mute) to 1.0 (full volume), values outside range are clamped
//----------------------------------------------------------------------------------------------------
struct VolumeUpdateData
{
	float volume;  // Target volume (0.0 - 1.0, clamped)
};

//----------------------------------------------------------------------------------------------------
// Position3DUpdateData
// Payload for UPDATE_3D_POSITION command
//
// Usage:
//   AudioCommand cmd{AudioCommandType::UPDATE_3D_POSITION, soundId,
//                    Position3DUpdateData{Vec3{10.f, 0.f, 2.f}}};
//
// Behavior:
//   - Updates 3D spatial position of sound source for positional audio
//   - Only affects sounds playing with 3D positioning enabled
//   - Position in world-space coordinates (X-forward, Y-left, Z-up)
//   - AudioSystem calculates attenuation/panning based on listener position
//----------------------------------------------------------------------------------------------------
struct Position3DUpdateData
{
	Vec3 position;  // World-space 3D position (X-forward, Y-left, Z-up)
};

//----------------------------------------------------------------------------------------------------
// AudioCommand
//
// Type-safe command structure using std::variant for payload storage.
// Guarantees zero-cost abstraction (no virtual function overhead).
//
// Memory Layout:
//   - type: 1 byte (enum AudioCommandType)
//   - soundId: 8 bytes (SoundID = uint64_t)
//   - data: ~256 bytes (largest variant = SoundLoadData with std::string)
//   Total: ~280 bytes per command (aligned to cache line)
//
// Thread Safety:
//   - Immutable after construction
//   - Safe to copy across thread boundaries (JavaScript worker → C++ main thread)
//
// Usage Pattern:
//   JavaScript Worker Thread:
//     AudioCommand cmd{AudioCommandType::PLAY_SOUND, soundId,
//                      SoundPlayData{1.0f, false, Vec3::ZERO}};
//     audioQueue->Submit(cmd);
//
//   C++ Main Thread (AudioSystem):
//     audioQueue->ConsumeAll([](AudioCommand const& cmd) {
//         std::visit(overloaded{
//             [](SoundPlayData const& data) { /* handle play */ },
//             [](SoundStopData const& data) { /* handle stop */ },
//             // ... other handlers
//         }, cmd.data);
//     });
//----------------------------------------------------------------------------------------------------
struct AudioCommand
{
	AudioCommandType type;
	SoundID          soundId;  // Target sound (0 for master volume commands)

	// Type-safe payload using std::variant
	// std::monostate for commands without payload (future use)
	std::variant<std::monostate,
	             SoundLoadData,
	             SoundPlayData,
	             SoundStopData,
	             VolumeUpdateData,
	             Position3DUpdateData>
	    data;

	// Default constructor (required for array initialization in CommandQueueBase)
	AudioCommand()
	    : type(AudioCommandType::PLAY_SOUND)
	    , soundId(0)
	    , data(std::monostate{})
	{
	}

	// Explicit constructor for type safety
	AudioCommand(AudioCommandType cmdType, SoundID id, auto&& payload)
	    : type(cmdType)
	    , soundId(id)
	    , data(std::forward<decltype(payload)>(payload))
	{
	}
};

//----------------------------------------------------------------------------------------------------
// Performance Characteristics (Phase 2 Targets)
//
// Command Size: ~280 bytes (cache-friendly, larger than RenderCommand due to string paths)
// Queue Capacity: 500 commands × 280 bytes = ~140 KB memory overhead
// Submission Latency: < 0.5ms (lock-free atomic operations)
//
// Expected Command Rates:
//   - Typical frame: 1-10 commands (background music, sound effects)
//   - Burst scenario: 20-50 commands (combat, explosions, UI feedback)
//   - Maximum: 500 commands (queue full → backpressure)
//
// Design Notes:
//   - Larger struct size than RenderCommand (~280 bytes vs ~72 bytes) acceptable
//     because audio commands are less frequent (1-10/frame vs 10-50/frame for rendering)
//   - SoundLoadData uses std::string (dynamic allocation) for file paths - acceptable
//     because LOAD_SOUND is infrequent (asset loading phase, not runtime hot path)
//   - Future optimization: Replace std::string with fixed-size char array if profiling
//     shows allocations are problematic (unlikely given low command frequency)
//----------------------------------------------------------------------------------------------------
