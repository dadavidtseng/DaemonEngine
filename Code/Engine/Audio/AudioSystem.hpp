//----------------------------------------------------------------------------------------------------
// AudioSystem.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
//----------------------------------------------------------------------------------------------------
#include "Game/EngineBuildPreferences.hpp"  // Phase 2: Required for ENGINE_SCRIPTING_ENABLED
#include "Engine/Audio/AudioTypes.hpp"
#include "Engine/Core/StringUtils.hpp"
//----------------------------------------------------------------------------------------------------
#include "ThirdParty/fmod/fmod.hpp"
//----------------------------------------------------------------------------------------------------
#include <map>

//-Forward-Declaration--------------------------------------------------------------------------------
struct Vec3;

#ifdef ENGINE_SCRIPTING_ENABLED
class CallbackQueue;
#endif

//----------------------------------------------------------------------------------------------------
/// @brief Audio system dimensionality specification for sound processing
///
/// @remark Determines how FMOD processes spatial audio calculations and performance optimizations.
//----------------------------------------------------------------------------------------------------
enum class eAudioSystemSoundDimension : int8_t
{
    /// @brief 2D audio playback without spatial positioning (stereo/mono)
    /// @remark Optimized for UI sounds, music, and non-positional audio
    Sound2D,

    /// @brief 3D spatial audio with distance attenuation and positioning
    /// @remark Requires Vec3 positioning and listener setup for proper spatial audio
    Sound3D,
};

//----------------------------------------------------------------------------------------------------
/// @brief Configuration structure for AudioSystem initialization parameters
///
/// @remark Currently empty but reserved for future audio system configuration options.
/// @remark Will contain settings like: max channels, sample rate, buffer sizes, device selection.
///
/// @warning Structure reserved for future expansion. Pass default-constructed instance for now.
//----------------------------------------------------------------------------------------------------
struct sAudioSystemConfig
{
};

//----------------------------------------------------------------------------------------------------
/// @brief High-level audio management system providing FMOD-based sound loading and playback
///
/// @remark Manages sound resource lifecycle, 3D spatial audio, and real-time playback control.
/// @remark Provides both 2D (UI/music) and 3D (positioned) audio capabilities through FMOD integration.
/// @remark Thread-safe for most operations, handles resource cleanup automatically.
///
/// @warning Requires FMOD library initialization. Call Startup() before use, Shutdown() before destruction.
/// @see https://fmod.com/docs/2.02/api/core-api-system.html
class AudioSystem
{
public:
    //------------------------------------------------------------------------------------------------
    /// @brief Construct AudioSystem with specified configuration parameters
    ///
    /// @param config Configuration structure containing audio system initialization settings
    ///
    /// @remark Does not initialize FMOD system - call Startup() for actual initialization.
    /// @remark Stores configuration for later use in Startup() method.
    ///
    /// @warning Constructor does not validate FMOD availability. Startup() may fail if FMOD unavailable.
    explicit AudioSystem(sAudioSystemConfig const& config);

    //------------------------------------------------------------------------------------------------
    /// @brief Virtual destructor ensuring proper cleanup of derived audio system classes
    ///
    /// @remark Automatically calls Shutdown() if not explicitly called before destruction.
    /// @remark Virtual to support inheritance and proper polymorphic destruction.
    ///
    /// @warning Ensure Shutdown() called before destruction to avoid FMOD resource leaks.
    //------------------------------------------------------------------------------------------------
    virtual ~AudioSystem() = default;

    //------------------------------------------------------------------------------------------------
    /// @brief Initialize FMOD audio system and prepare for sound loading/playback
    ///
    /// @remark Initializes FMOD::System, sets up default audio channels, and prepares sound management.
    /// @remark Must be called before any sound loading or playback operations.
    ///
    /// @warning Failure to call before audio operations will result in undefined behavior.
    /// @warning Multiple Startup() calls without Shutdown() may cause resource leaks.
    /// @see https://fmod.com/docs/2.02/api/core-api-system.html#system_init
    //----------------------------------------------------------------------------------------------------
    void Startup();

    //----------------------------------------------------------------------------------------------------
    /// @brief Cleanup FMOD resources and stop all active audio playback
    ///
    /// @remark Stops all playing sounds, releases loaded sound resources, and closes FMOD system.
    /// @remark Safe to call multiple times - subsequent calls are ignored.
    ///
    /// @warning All SoundID and SoundPlaybackID handles become invalid after Shutdown().
    /// @see https://fmod.com/docs/2.02/api/core-api-system.html#system_close
    void Shutdown();

    //----------------------------------------------------------------------------------------------------
    /// @brief Update audio system state and process FMOD callbacks (call per frame)
    ///
    /// @remark Updates FMOD internal state, processes streaming, and handles 3D audio calculations.
    /// @remark Virtual to allow derived classes to add pre-frame audio processing.
    ///
    /// @warning Must be called every frame for proper audio system functionality.
    /// @see https://fmod.com/docs/2.02/api/core-api-system.html#system_update
    virtual void BeginFrame();

    //----------------------------------------------------------------------------------------------------
    /// @brief Finalize frame-based audio processing and commit audio state changes
    ///
    /// @remark Completes any pending audio operations and prepares for next frame.
    /// @remark Virtual to allow derived classes to add post-frame audio processing.
    ///
    /// @remark Currently reserved for future frame-end processing requirements.
    virtual void EndFrame();

    //----------------------------------------------------------------------------------------------------
    /// @brief Load sound file and return handle for playback operations
    ///
    /// @param soundFilePath File system path to audio file (WAV, MP3, OGG formats supported by FMOD)
    /// @param dimension Audio dimensionality specification (2D for UI/music, 3D for spatial audio)
    ///
    /// @return SoundID handle for loaded sound resource, or MISSING_SOUND_ID if loading fails
    ///
    /// @remark Caches loaded sounds internally - subsequent calls with same path return existing SoundID.
    /// @remark Automatically determines optimal FMOD sound creation flags based on dimension parameter.
    /// @remark Virtual to allow derived classes to implement custom sound loading strategies.
    ///
    /// @warning Invalid file paths or unsupported formats return MISSING_SOUND_ID.
    /// @warning Large sound files may cause loading delays - consider background loading for large assets.
    /// @see https://fmod.com/docs/2.02/api/core-api-system.html#system_createsound
    virtual SoundID CreateOrGetSound(String const& soundFilePath, eAudioSystemSoundDimension dimension);

    //----------------------------------------------------------------------------------------------------
    /// @brief Start non-positional sound playback with comprehensive audio control parameters
    ///
    /// @param soundID Valid sound resource handle obtained from CreateOrGetSound()
    /// @param isLooped If true, sound loops indefinitely until explicitly stopped
    /// @param volume Playback volume in range [0.0, 1.0] where 0.0=silent, 1.0=full volume
    /// @param balance Stereo balance in range [-1.0, 1.0] where -1.0=left only, 0.0=center, 1.0=right only
    /// @param speed Playback speed multiplier where 1.0=normal speed, 2.0=double speed, 0.5=half speed
    /// @param isPaused If true, sound starts in paused state (call SetSoundPlaybackVolume to resume)
    ///
    /// @return SoundPlaybackID handle for controlling this specific playback instance, or MISSING_SOUND_ID if failed
    ///
    /// @remark Creates new playback instance - multiple simultaneous playbacks of same sound supported.
    /// @remark Virtual to allow derived classes to implement custom playback logic and effects.
    ///
    /// @warning Invalid soundID parameter returns MISSING_SOUND_ID and no audio plays.
    /// @warning Volume parameter clamped to [0.0, 1.0] range - values outside range may cause distortion.
    /// @see StartSoundAt() for 3D positional audio playback
    virtual SoundPlaybackID StartSound(SoundID soundID, bool isLooped = false, float volume = 1.f, float balance = 0.f, float speed = 1.f, bool isPaused = false);

    //----------------------------------------------------------------------------------------------------
    /// @brief Start 3D positional sound playback with spatial audio processing
    ///
    /// @param soundID Valid sound resource handle (must be created with Sound3D dimension)
    /// @param soundPosition 3D world coordinates for sound source location
    /// @param isLooped If true, sound loops indefinitely until explicitly stopped
    /// @param volume 3D audio volume where 10.0=standard 3D volume, scaled by distance attenuation
    /// @param balance Stereo balance (typically 0.0 for 3D audio to allow spatial processing)
    /// @param speed Playback speed multiplier where 1.0=normal, 2.0=double speed, 0.5=half speed
    /// @param isPaused If true, sound starts in paused state
    ///
    /// @return SoundPlaybackID handle for controlling this 3D playback instance, or MISSING_SOUND_ID if failed
    ///
    /// @remark Requires proper listener setup via SetNumListeners() and UpdateListener() for spatial accuracy.
    /// @remark Virtual to allow derived classes to implement custom 3D audio algorithms and effects.
    ///
    /// @warning soundID must be created with Sound3D dimension or spatial processing will be incorrect.
    /// @warning Requires active listener configuration - sounds may be inaudible without proper listener setup.
    /// @see UpdateListener() for 3D listener positioning, SetSoundPosition() for runtime position updates
    virtual SoundPlaybackID StartSoundAt(SoundID soundID, Vec3 const& soundPosition, bool isLooped = false, float volume = 10.0f, float balance = 0.0f, float speed = 1.0f, bool isPaused = false);
    //----------------------------------------------------------------------------------------------------
    /// @brief Immediately stop active sound playback and release playback resources
    ///
    /// @param soundPlaybackID Valid playback handle obtained from StartSound() or StartSoundAt()
    ///
    /// @remark Stops sound immediately without fade-out - use volume ramping for smooth transitions.
    /// @remark Virtual to allow derived classes to implement custom stop behaviors (fade-out, callbacks).
    /// @remark Safe to call with invalid IDs - no operation performed for invalid handles.
    ///
    /// @warning soundPlaybackID becomes invalid after this call - do not reuse for other operations.
    /// @see SetSoundPlaybackVolume() for gradual volume reduction before stopping
    virtual void StopSound(SoundPlaybackID soundPlaybackID);

    //----------------------------------------------------------------------------------------------------
    /// @brief Modify playback volume for active sound instance in real-time
    ///
    /// @param soundPlaybackID Valid playback handle for currently playing sound
    /// @param volume New volume level in range [0.0, 1.0] where 0.0=silent, 1.0=maximum volume
    ///
    /// @remark Changes take effect immediately - suitable for real-time audio mixing and effects.
    /// @remark Virtual to allow derived classes to implement volume scaling, compression, or limiting.
    /// @remark Safe to call multiple times - no performance penalty for frequent volume updates.
    ///
    /// @warning Volume values outside [0.0, 1.0] range may be clamped or cause audio distortion.
    /// @warning Invalid soundPlaybackID results in no operation - check validity before calling.
    virtual void SetSoundPlaybackVolume(SoundPlaybackID soundPlaybackID, float volume);

    //----------------------------------------------------------------------------------------------------
    /// @brief Adjust stereo balance for active sound playback in real-time
    ///
    /// @param soundPlaybackID Valid playback handle for currently playing sound  
    /// @param balance Stereo positioning in range [-1.0, 1.0] where -1.0=left only, 0.0=center, 1.0=right only
    ///
    /// @remark Provides real-time stereo panning control for 2D audio positioning effects.
    /// @remark Virtual to allow derived classes to implement advanced panning algorithms or surround sound.
    /// @remark For 3D sounds, balance may be overridden by spatial audio processing.
    ///
    /// @warning Balance values outside [-1.0, 1.0] range may be clamped or produce unexpected results.
    /// @warning 3D positioned sounds may ignore balance setting in favor of spatial calculations.
    virtual void SetSoundPlaybackBalance(SoundPlaybackID soundPlaybackID, float balance);

    //----------------------------------------------------------------------------------------------------
    /// @brief Modify playback speed and pitch for active sound instance in real-time
    ///
    /// @param soundPlaybackID Valid playback handle for currently playing sound
    /// @param speed Frequency multiplier where 1.0=normal, 2.0=double speed/higher pitch, 0.5=half speed/lower pitch
    ///
    /// @remark Changes both playback rate and pitch simultaneously - affects audio duration and frequency.
    /// @remark Virtual to allow derived classes to implement pitch-shifting without tempo change.
    /// @remark Useful for audio effects, slow-motion sequences, or dynamic music tempo adjustments.
    ///
    /// @warning Extreme speed values (>4.0 or <0.25) may cause audio artifacts or processing limitations.
    /// @warning Speed changes affect both tempo and pitch - consider dedicated pitch-shifting for music.
    virtual void SetSoundPlaybackSpeed(SoundPlaybackID soundPlaybackID, float speed);

    //----------------------------------------------------------------------------------------------------
    /// @brief Validate FMOD operation result and handle errors appropriately  
    ///
    /// @param result FMOD_RESULT value returned from FMOD API function calls
    ///
    /// @remark Checks for FMOD errors and provides appropriate error handling/logging.
    /// @remark Virtual to allow derived classes to implement custom error handling strategies.
    /// @remark Typically called after each FMOD API operation to ensure proper error detection.
    ///
    /// @warning FMOD errors may indicate system failure, resource exhaustion, or invalid parameters.
    /// @see https://fmod.com/docs/2.02/api/core-api-common.html#fmod_result
    virtual void ValidateResult(FMOD_RESULT result);

    //----------------------------------------------------------------------------------------------------
    /// @brief Configure number of 3D audio listeners for spatial audio processing
    ///
    /// @param numListeners Number of active listeners (typically 1 for single-player, 2+ for splitscreen)
    ///
    /// @remark Must be called before UpdateListener() operations for proper 3D audio setup.
    /// @remark FMOD supports multiple listeners for splitscreen/multi-perspective audio scenarios.
    /// @remark Const method as it only configures FMOD system state without modifying class data.
    ///
    /// @warning numListeners must be ≥1 or 3D audio calculations will be undefined.
    /// @warning Changing listener count during audio playback may cause temporary audio artifacts.
    /// @see UpdateListener() for individual listener positioning and orientation
    void SetNumListeners(int numListeners) const;

    //----------------------------------------------------------------------------------------------------
    /// @brief Update 3D listener position and orientation for spatial audio calculations
    ///
    /// @param listenerIndex Zero-based listener identifier (must be < numListeners from SetNumListeners)
    /// @param listenerPosition 3D world coordinates of listener location (camera/player position)
    /// @param listenerForward Forward direction vector of listener (normalized camera forward)
    /// @param listenerUp Up direction vector of listener (normalized camera up, typically world Y-axis)
    ///
    /// @remark Updates FMOD 3D audio engine with current listener state for spatial sound processing.
    /// @remark Should be called every frame for accurate 3D audio tracking with moving listeners.
    /// @remark Const method as it updates FMOD system state without modifying class member data.
    ///
    /// @warning listenerIndex must be valid (0 to numListeners-1) or operation will fail silently.
    /// @warning Forward and Up vectors should be normalized and perpendicular for proper spatial calculation.
    /// @see SetNumListeners() for listener configuration, StartSoundAt() for 3D sound positioning
    void UpdateListener(int listenerIndex, Vec3 const& listenerPosition, Vec3 const& listenerForward, Vec3 const& listenerUp) const;

    //----------------------------------------------------------------------------------------------------
    /// @brief Update 3D position of actively playing spatial sound in real-time
    ///
    /// @param soundPlaybackID Valid playback handle for 3D sound instance
    /// @param soundPosition New 3D world coordinates for sound source location
    ///
    /// @remark Enables dynamic positioning of moving sound sources (vehicles, projectiles, NPCs).
    /// @remark Virtual to allow derived classes to implement interpolation, physics integration, or occlusion.
    /// @remark Position changes take effect immediately for responsive spatial audio tracking.
    ///
    /// @warning soundPlaybackID must be for 3D sound created with Sound3D dimension or operation is ignored.
    /// @warning Invalid soundPlaybackID results in no operation - verify handle validity before calling.
    /// @see StartSoundAt() for initial 3D sound positioning, UpdateListener() for listener positioning
    virtual void SetSoundPosition(SoundPlaybackID soundPlaybackID, const Vec3& soundPosition);

    //------------------------------------------------------------------------------------------------
    /// @brief Check if specified sound playback instance is currently active and playing
    ///
    /// @param soundPlaybackID Playback handle to query for active status
    ///
    /// @return bool True if sound is actively playing, false if stopped, paused, or invalid ID
    ///
    /// @remark Useful for audio state management, preventing duplicate playbacks, and audio cleanup.
    /// @remark Distinguishes between paused sounds (not playing) and stopped sounds (not active).
    ///
    /// @warning Invalid soundPlaybackID always returns false - cannot distinguish from stopped sounds.
    /// @see StartSound(), StartSoundAt() for playback initiation, StopSound() for playback termination
    //------------------------------------------------------------------------------------------------
    bool IsPlaying(SoundPlaybackID soundPlaybackID);

    //------------------------------------------------------------------------------------------------
    /// @brief Convert engine Vec3 to FMOD_VECTOR format for FMOD API compatibility
    ///
    /// @param vectorToCast Engine Vec3 vector to convert to FMOD coordinate system
    ///
    /// @return FMOD_VECTOR Converted vector in FMOD-compatible format for spatial audio calculations
    ///
    /// @remark Handles coordinate system conversion between engine and FMOD spatial representations.
    /// @remark Const method as conversion doesn't modify class state - pure utility function.
    ///
    /// @warning Ensure coordinate system consistency between all Vec3 inputs for proper spatial audio.
    /// @see UpdateListener(), SetSoundPosition() for usage in 3D audio positioning
    //------------------------------------------------------------------------------------------------
    FMOD_VECTOR CastVec3ToFmodVec(Vec3 const& vectorToCast) const;

    //------------------------------------------------------------------------------------------------
    /// @brief Create zero-initialized FMOD_VECTOR for initialization and default values
    ///
    /// @return FMOD_VECTOR Zero vector {0.0f, 0.0f, 0.0f} in FMOD format
    ///
    /// @remark Utility for initializing FMOD vectors to known zero state for calculations.
    /// @remark Const method as it creates new vector without modifying class state.
    ///
    /// @see CastVec3ToFmodVec() for Vec3-to-FMOD conversion, FMOD documentation for vector usage
    //------------------------------------------------------------------------------------------------
    FMOD_VECTOR CreateZeroVector() const;

protected:
    //------------------------------------------------------------------------------------------------
    /// @brief FMOD system instance for low-level audio operations and resource management
    ///
    /// @remark Core FMOD interface for sound creation, playback control, and system configuration.
    /// @remark Protected to allow derived classes direct FMOD access for specialized audio features.
    ///
    /// @warning Direct manipulation requires careful FMOD API knowledge - prefer high-level AudioSystem methods.
    /// @see https://fmod.com/docs/2.02/api/core-api-system.html
    //------------------------------------------------------------------------------------------------
    FMOD::System* m_fmodSystem;

    //------------------------------------------------------------------------------------------------
    /// @brief Cached mapping of sound file paths to loaded SoundID handles for resource management
    ///
    /// @remark Prevents duplicate loading of same sound files - maps file paths to existing SoundIDs.
    /// @remark String keys are normalized file paths, values are valid SoundID handles for reuse.
    /// @remark Protected to allow derived classes to implement custom caching strategies.
    ///
    /// @warning Manual modification may cause resource leaks or invalid sound references.
    /// @see CreateOrGetSound() for automatic cache management and sound loading
    //------------------------------------------------------------------------------------------------
    std::map<String, SoundID> m_registeredSoundIDs;

    //------------------------------------------------------------------------------------------------
    /// @brief Sequential storage of loaded FMOD::Sound resources indexed by SoundID
    ///
    /// @remark Vector index corresponds to SoundID value for O(1) sound resource lookup.
    /// @remark Contains FMOD::Sound pointers managed by FMOD system - do not manually delete.
    /// @remark Protected to allow derived classes to access sound resources for advanced operations.
    ///
    /// @warning Direct pointer access requires FMOD API knowledge - invalid operations may crash.
    /// @warning SoundID values must be valid vector indices - bounds checking required for safety.
    /// @see CreateOrGetSound() for sound loading and ID assignment
    //------------------------------------------------------------------------------------------------
    std::vector<FMOD::Sound*> m_registeredSounds;

private:
    //------------------------------------------------------------------------------------------------
    /// @brief Stored configuration parameters for audio system initialization and behavior
    ///
    /// @remark Contains settings passed to constructor, used during Startup() for FMOD initialization.
    /// @remark Private to ensure configuration consistency - modifications require class redesign.
    ///
    /// @warning Currently unused but reserved for future audio system configuration expansion.
    /// @see AudioSystem constructor for configuration parameter passing
    //------------------------------------------------------------------------------------------------
    sAudioSystemConfig m_audioConfig;
};
