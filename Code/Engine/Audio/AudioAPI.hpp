//----------------------------------------------------------------------------------------------------
// AudioAPI.hpp
// Engine Audio Module - Audio Management API
//
// Purpose:
//   Provides high-level audio management API for JavaScript integration.
//   Handles sound loading, playback control, and volume management through audio command queue.
//
// Design Philosophy:
//   - Single Responsibility: Audio-specific operations only
//   - Async callbacks for loading operations (error resilience)
//   - Thread-safe command submission to AudioCommandQueue
//   - JavaScript errors must NOT crash C++ audio system
//
// API Surface:
//   Sound Loading:
//     - LoadSoundAsync(path, callback) - Async, returns SoundID via callback
//
//   Sound Playback:
//     - PlaySound(soundId, volume, looped, position) - Start playback
//     - StopSound(soundId) - Stop playback
//
//   Sound Updates:
//     - SetVolume(soundId, volume) - Adjust sound volume (0.0-1.0)
//     - Update3DPosition(soundId, position) - Update spatial audio position
//
// Coordinate System:
//   X-forward, Y-left, Z-up (right-handed)
//   +X = forward, +Y = left, +Z = up
//
// Thread Safety:
//   - Methods submit AudioCommands to AudioCommandQueue (lock-free)
//   - Callbacks executed on JavaScript worker thread (V8 isolation required)
//   - C++ audio system continues even if JavaScript callbacks throw errors
//
// Author: Phase 5 - Audio Async Pattern Implementation
// Date: 2025-12-07
//----------------------------------------------------------------------------------------------------
#pragma once
//----------------------------------------------------------------------------------------------------
#include "Engine/Audio/AudioTypes.hpp"
#include "Engine/Script/ScriptCommon.hpp"
//----------------------------------------------------------------------------------------------------
#include <any>
#include <unordered_map>
#include <unordered_set>

//-Forward-Declaration--------------------------------------------------------------------------------
class AudioCommandQueue;
class ScriptSubsystem;
class CallbackQueue;
struct Vec3;

//----------------------------------------------------------------------------------------------------
// CallbackID Type Definition (shared with EntityAPI, CameraAPI)
using CallbackID = uint64_t;

//----------------------------------------------------------------------------------------------------
// ScriptCallback Type Definition (shared with EntityAPI, CameraAPI)
using ScriptCallback = std::any;

///----------------------------------------------------------------------------------------------------
/// AudioAPI
///
/// High-level audio management API for JavaScript integration.
/// Handles sound loading, playback control, and volume management through audio command queue.
///
/// Usage Pattern (from JavaScript):
///
/// Sound Loading (Async):
///   audio.loadSound('Data/Audio/explosion.mp3', (soundId) => {
///       console.log('Sound loaded:', soundId);
///       audio.playSound(soundId, 1.0, false, {x: 5, y: 0, z: 0});
///   });
///
/// Sound Playback (Sync):
///   audio.playSound(soundId, 0.8, false, {x: 10, y: 0, z: 2});  // One-shot, 80% volume, at position
///   audio.playSound(musicId, 0.5, true, {x: 0, y: 0, z: 0});    // Looped, 50% volume, non-spatial
///
/// Sound Control (Sync):
///   audio.stopSound(soundId);                          // Stop playback
///   audio.setVolume(soundId, 0.6);                     // Adjust volume to 60%
///   audio.update3DPosition(soundId, {x: 15, y: 2, z: 1});  // Update spatial position
///
/// Error Resilience:
///   - JavaScript callback errors are caught and logged
///   - C++ audio system continues with last valid state
///   - Invalid soundIds are ignored with warning logs
///----------------------------------------------------------------------------------------------------
class AudioAPI
{
public:
	//------------------------------------------------------------------------------------------------
	// Construction / Destruction
	//------------------------------------------------------------------------------------------------
	explicit AudioAPI(AudioCommandQueue* commandQueue,
	                  ScriptSubsystem*    scriptSubsystem,
	                  CallbackQueue*      callbackQueue);  // NO OWNERSHIP - pointers must outlive this object
	~AudioAPI();

	// Non-copyable (manages callback state)
	AudioAPI(AudioAPI const&)            = delete;
	AudioAPI& operator=(AudioAPI const&) = delete;

	//------------------------------------------------------------------------------------------------
	// Sound Loading
	//------------------------------------------------------------------------------------------------

	// Load audio file asynchronously (returns SoundID via callback)
	// Parameters:
	//   - soundPath: Relative path to audio file (e.g., "Data/Audio/explosion.mp3")
	//   - callback: JavaScript function (soundId) => {...}
	// Returns: CallbackID (for internal tracking, not exposed to JavaScript)
	CallbackID LoadSoundAsync(std::string const&    soundPath,
	                          ScriptCallback const& callback);

	//------------------------------------------------------------------------------------------------
	// Sound Playback
	//------------------------------------------------------------------------------------------------

	// Play loaded sound
	// Parameters:
	//   - soundId: SoundID returned from LoadSoundAsync callback
	//   - volume: Playback volume (0.0 = silent, 1.0 = full volume)
	//   - looped: true = continuous loop, false = one-shot playback
	//   - position: 3D world-space position (X-forward, Y-left, Z-up)
	void PlaySound(SoundID soundId, float volume, bool looped, Vec3 const& position);

	// Stop sound playback
	void StopSound(SoundID soundId);

	//------------------------------------------------------------------------------------------------
	// Sound Updates
	//------------------------------------------------------------------------------------------------

	// Set sound volume (0.0-1.0, clamped automatically)
	void SetVolume(SoundID soundId, float volume);

	// Update 3D spatial position of sound source
	void Update3DPosition(SoundID soundId, Vec3 const& position);

	//------------------------------------------------------------------------------------------------
	// Callback Execution (called by App::Update() on main thread)
	//------------------------------------------------------------------------------------------------

	// Execute pending callbacks with results
	// Called by App::Update() after processing audio commands
	// Executes callbacks on JavaScript worker thread with V8 locking
	void ExecutePendingCallbacks(CallbackQueue* callbackQueue);

	// Register a callback completion (called by command processor)
	void NotifyCallbackReady(CallbackID callbackId, SoundID resultId);

private:
	//------------------------------------------------------------------------------------------------
	// Private Members
	//------------------------------------------------------------------------------------------------
	AudioCommandQueue* m_commandQueue;     // NO OWNERSHIP - Command queue for audio operations
	ScriptSubsystem*   m_scriptSubsystem;  // NO OWNERSHIP - V8 script subsystem reference
	CallbackQueue*     m_callbackQueue;    // NO OWNERSHIP - Callback queue for async results

	// Callback ID generation
	CallbackID m_nextCallbackId = 1;  // Unique callback ID generator (0 reserved)

	// Callback storage (CallbackID → {ScriptCallback, resultId, ready})
	struct PendingCallback
	{
		ScriptCallback callback;
		SoundID        resultId;
		bool           ready;  // True when C++ has processed command and resultId is available
	};
	std::unordered_map<CallbackID, PendingCallback> m_pendingCallbacks;

	//------------------------------------------------------------------------------------------------
	// Private Helper Methods
	//------------------------------------------------------------------------------------------------

	// Generate unique callback ID (atomic increment)
	CallbackID GenerateCallbackID();
};
