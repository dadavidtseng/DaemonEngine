//----------------------------------------------------------------------------------------------------
// AudioSystem.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Audio/AudioSystem.hpp"

#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/StringUtils.hpp"

//----------------------------------------------------------------------------------------------------
// To disable audio entirely (and remove requirement for fmod.dll / fmod64.dll) for any game,
//	#define ENGINE_DISABLE_AUDIO in your game's Code/Game/EngineBuildPreferences.hpp file.
//
// Note that this #include is an exception to the rule "engine code doesn't know about game code".
//	Purpose: Each game can now direct the engine via #defines to build differently for that game.
//	Downside: ALL games must now have this Code/Game/EngineBuildPreferences.hpp file.
//
// SD1 NOTE: THIS MEANS *EVERY* GAME MUST HAVE AN EngineBuildPreferences.hpp FILE IN ITS CODE/GAME FOLDER!!
#include "Engine/Math/Vec3.hpp"
#include "Game/EngineBuildPreferences.hpp"
#if !defined( ENGINE_DISABLE_AUDIO )

#ifdef ENGINE_SCRIPTING_ENABLED
#include "Engine/Audio/AudioCommand.hpp"
#include "Engine/Audio/AudioCommandQueue.hpp"
#include "Engine/Core/CallbackQueue.hpp"
#include "Engine/Core/CallbackData.hpp"
#include "Engine/Core/LogSubsystem.hpp"
#include <variant>
#endif


//----------------------------------------------------------------------------------------------------
// Link in the appropriate FMOD static library (32-bit or 64-bit)
//
#if defined( _WIN64 )
#pragma comment( lib, "ThirdParty/fmod/fmod64_vc.lib" )
#else
#pragma comment( lib, "ThirdParty/fmod/fmod_vc.lib" )
#endif


//----------------------------------------------------------------------------------------------------
// Initialization code based on example from "FMOD Studio Programmers API for Windows"
//
AudioSystem::AudioSystem(sAudioSystemConfig const& config)
    : m_fmodSystem(nullptr)
{
    m_audioConfig = config;
}

//----------------------------------------------------------------------------------------------------
void AudioSystem::Startup()
{
    FMOD_RESULT result = FMOD::System_Create(&m_fmodSystem);
    ValidateResult(result);

    result = m_fmodSystem->init(512, FMOD_INIT_3D_RIGHTHANDED, nullptr);
    ValidateResult(result);
}

//------------------------------------------------------------------------------------------------
void AudioSystem::Shutdown()
{
    FMOD_RESULT const result = m_fmodSystem->release();
    ValidateResult(result);

    m_fmodSystem = nullptr; //TODO: do we delete/free the object also, or just do this?
}

//----------------------------------------------------------------------------------------------------
void AudioSystem::BeginFrame()
{
#ifdef ENGINE_SCRIPTING_ENABLED
    ProcessPendingCommands();
#endif
    m_fmodSystem->update();
}

//----------------------------------------------------------------------------------------------------
void AudioSystem::EndFrame()
{
}

//----------------------------------------------------------------------------------------------------
SoundID AudioSystem::CreateOrGetSound(String const&                    soundFilePath,
                                      eAudioSystemSoundDimension const dimension)
{
    std::map<std::string, SoundID>::iterator const found = m_registeredSoundIDs.find(soundFilePath);

    if (found != m_registeredSoundIDs.end())
    {
        return found->second;
    }
    else
    {
        FMOD::Sound* newSound = nullptr;
        if (dimension == eAudioSystemSoundDimension::Sound2D)
        {
            m_fmodSystem->createSound(soundFilePath.c_str(), FMOD_DEFAULT, nullptr, &newSound);
        }
        else if (dimension == eAudioSystemSoundDimension::Sound3D)
        {
            m_fmodSystem->createSound(soundFilePath.c_str(), FMOD_3D, nullptr, &newSound);
        }
        if (newSound)
        {
            SoundID newSoundID                  = m_registeredSounds.size();
            m_registeredSoundIDs[soundFilePath] = newSoundID;
            m_registeredSounds.push_back(newSound);
            return newSoundID;
        }
    }

    return MISSING_SOUND_ID;
}

//----------------------------------------------------------------------------------------------------
SoundPlaybackID AudioSystem::StartSound(SoundID const soundID,
                                        bool const    isLooped,
                                        float const   volume,
                                        float const   balance,
                                        float const   speed,
                                        bool const    isPaused)
{
    size_t const numSounds = m_registeredSounds.size();

    if (soundID < 0 || soundID >= numSounds) return MISSING_SOUND_ID;

    FMOD::Sound* sound = m_registeredSounds[soundID];

    if (!sound) return MISSING_SOUND_ID;

    FMOD::Channel* channelAssignedToSound = nullptr;
    m_fmodSystem->playSound(sound, nullptr, isPaused, &channelAssignedToSound);

    if (channelAssignedToSound)
    {
        int const          loopCount    = isLooped ? -1 : 0;
        unsigned int const playbackMode = isLooped ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF;
        float              frequency;
        channelAssignedToSound->setMode(playbackMode);
        channelAssignedToSound->getFrequency(&frequency);
        channelAssignedToSound->setFrequency(frequency * speed);
        channelAssignedToSound->setVolume(volume);
        channelAssignedToSound->setPan(balance);
        channelAssignedToSound->setLoopCount(loopCount);
    }

    return (SoundPlaybackID)channelAssignedToSound;
}

//----------------------------------------------------------------------------------------------------
SoundPlaybackID AudioSystem::StartSoundAt(SoundID const soundID,
                                          Vec3 const&   soundPosition,
                                          bool const    isLooped,
                                          float const   volume,
                                          float const   balance,
                                          float const   speed,
                                          bool const    isPaused)
{
    size_t const numSounds = m_registeredSounds.size();

    if (soundID < 0 || soundID >= numSounds) return MISSING_SOUND_ID;

    FMOD::Sound* sound = m_registeredSounds[soundID];

    if (!sound) return MISSING_SOUND_ID;

    FMOD::Channel* channelAssignedToSound = nullptr;
    m_fmodSystem->playSound(sound, nullptr, isPaused, &channelAssignedToSound);
    if (channelAssignedToSound)
    {
        int const    loopCount    = isLooped ? -1 : 0;
        unsigned int playbackMode = isLooped ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF;
        playbackMode |= FMOD_3D;
        float             frequency;
        FMOD_VECTOR const position = CastVec3ToFmodVec(soundPosition);
        FMOD_VECTOR const velocity = CreateZeroVector();
        channelAssignedToSound->setMode(playbackMode);
        channelAssignedToSound->getFrequency(&frequency);
        channelAssignedToSound->setFrequency(frequency * speed);
        channelAssignedToSound->setVolume(volume);
        channelAssignedToSound->setPan(balance);
        channelAssignedToSound->setLoopCount(loopCount);
        channelAssignedToSound->set3DAttributes(&position, &velocity);
    }

    return (SoundPlaybackID)channelAssignedToSound;
}

//----------------------------------------------------------------------------------------------------
void AudioSystem::StopSound(SoundPlaybackID const soundPlaybackID)
{
    if (soundPlaybackID == MISSING_SOUND_ID)
    {
        ERROR_RECOVERABLE("WARNING: attempt to stop sound on missing sound playback ID!");
        return;
    }

    FMOD::Channel* channelAssignedToSound = (FMOD::Channel*)soundPlaybackID;
    channelAssignedToSound->stop();
}


//-----------------------------------------------------------------------------------------------
// Volume is in [0,1]
//
void AudioSystem::SetSoundPlaybackVolume(SoundPlaybackID const soundPlaybackID,
                                         float const           volume)
{
    if (soundPlaybackID == MISSING_SOUND_ID)
    {
        ERROR_RECOVERABLE("WARNING: attempt to set volume on missing sound playback ID!");
        return;
    }

    FMOD::Channel* channelAssignedToSound = (FMOD::Channel*)soundPlaybackID;
    channelAssignedToSound->setVolume(volume);
}


//-----------------------------------------------------------------------------------------------
// Balance is in [-1,1], where 0 is L/R centered
//
void AudioSystem::SetSoundPlaybackBalance(SoundPlaybackID const soundPlaybackID,
                                          float const           balance)
{
    if (soundPlaybackID == MISSING_SOUND_ID)
    {
        ERROR_RECOVERABLE("WARNING: attempt to set balance on missing sound playback ID!");
        return;
    }

    FMOD::Channel* channelAssignedToSound = (FMOD::Channel*)soundPlaybackID;
    channelAssignedToSound->setPan(balance);
}


//-----------------------------------------------------------------------------------------------
// Speed is frequency multiplier (1.0 == normal)
//	A speed of 2.0 gives 2x frequency, i.e. exactly one octave higher
//	A speed of 0.5 gives 1/2 frequency, i.e. exactly one octave lower
//
void AudioSystem::SetSoundPlaybackSpeed(SoundPlaybackID const soundPlaybackID, float const speed)
{
    if (soundPlaybackID == MISSING_SOUND_ID)
    {
        ERROR_RECOVERABLE("WARNING: attempt to set speed on missing sound playback ID!")
        return;
    }

    FMOD::Channel* channelAssignedToSound = (FMOD::Channel*)soundPlaybackID;
    float          frequency;
    FMOD::Sound*   currentSound = nullptr;
    channelAssignedToSound->getCurrentSound(&currentSound);
    if (!currentSound) return;

    int ignored = 0;
    currentSound->getDefaults(&frequency, &ignored);
    channelAssignedToSound->setFrequency(frequency * speed);
}


//-----------------------------------------------------------------------------------------------
void AudioSystem::ValidateResult(FMOD_RESULT const result)
{
    if (result != FMOD_OK)
    {
        ERROR_RECOVERABLE(Stringf( "Engine/Audio SYSTEM ERROR: Got error result code %i - error codes listed in fmod_common.h\n", result ))
    }
}

//----------------------------------------------------------------------------------------------------
void AudioSystem::SetNumListeners(int const numListeners) const
{
    m_fmodSystem->set3DNumListeners(numListeners);
}

//----------------------------------------------------------------------------------------------------
void AudioSystem::UpdateListener(int const   listenerIndex,
                                 Vec3 const& listenerPosition,
                                 Vec3 const& listenerForward,
                                 Vec3 const& listenerUp) const
{
    FMOD_VECTOR const position = CastVec3ToFmodVec(listenerPosition);
    FMOD_VECTOR const velocity = CreateZeroVector();
    FMOD_VECTOR const forward  = CastVec3ToFmodVec(listenerForward);
    FMOD_VECTOR const up       = CastVec3ToFmodVec(listenerUp);

    m_fmodSystem->set3DListenerAttributes(listenerIndex, &position, &velocity, &forward, &up);
}

//----------------------------------------------------------------------------------------------------
void AudioSystem::SetSoundPosition(SoundPlaybackID const soundPlaybackID,
                                   Vec3 const&           soundPosition)
{
    if (!IsPlaying(soundPlaybackID))
    {
        return;
    }

    if (soundPlaybackID == MISSING_SOUND_ID)
    {
        ERROR_RECOVERABLE("WARNING: attempt to set speed on missing sound playback ID!");
        return;
    }

    FMOD::Channel* channelAssignedToSound = (FMOD::Channel*)soundPlaybackID;
    FMOD::Sound*   currentSound           = nullptr;
    channelAssignedToSound->getCurrentSound(&currentSound);
    if (!currentSound) return;

    FMOD_VECTOR position = CastVec3ToFmodVec(soundPosition);
    FMOD_VECTOR velocity = CreateZeroVector();
    channelAssignedToSound->set3DAttributes(&position, &velocity);
}

bool AudioSystem::IsPlaying(SoundPlaybackID const soundPlaybackID)
{
    if (soundPlaybackID == MISSING_SOUND_ID)
    {
        ERROR_RECOVERABLE("WARNING: attempt to set speed on missing sound playback ID!")
        return true;
    }

    FMOD::Channel* channelAssignedToSound = (FMOD::Channel*)soundPlaybackID;
    FMOD::Sound*   currentSound           = nullptr;
    channelAssignedToSound->getCurrentSound(&currentSound);
    if (!currentSound) return false;
    bool isPlaying = false;
    channelAssignedToSound->isPlaying(&isPlaying);
    return isPlaying;
}

//----------------------------------------------------------------------------------------------------
FMOD_VECTOR AudioSystem::CastVec3ToFmodVec(Vec3 const& vectorToCast) const
{
    FMOD_VECTOR position;
    position.x = -vectorToCast.y;
    position.y = vectorToCast.z;
    position.z = -vectorToCast.x;
    return position;
}

//----------------------------------------------------------------------------------------------------
FMOD_VECTOR AudioSystem::CreateZeroVector() const
{
    FMOD_VECTOR vector;

    vector.x = 0.f;
    vector.y = 0.f;
    vector.z = 0.f;

    return vector;
}

#ifdef ENGINE_SCRIPTING_ENABLED
//----------------------------------------------------------------------------------------------------
// SetCommandQueue
//
// Configure command queue for async audio command processing from JavaScript.
// Stores pointers to externally-owned queues (caller retains ownership).
//----------------------------------------------------------------------------------------------------
void AudioSystem::SetCommandQueue(AudioCommandQueue* commandQueue, CallbackQueue* callbackQueue)
{
    m_commandQueue  = commandQueue;
    m_callbackQueue = callbackQueue;

    DAEMON_LOG(LogAudio, eLogVerbosity::Log,
               Stringf("AudioSystem: Command queue configured (commandQueue=%p, callbackQueue=%p)",
                   commandQueue, callbackQueue));
}

//----------------------------------------------------------------------------------------------------
// ProcessPendingCommands
//
// Consume all pending audio commands from JavaScript and execute corresponding operations.
// Called automatically from BeginFrame() when command queue is configured.
//
// Command Processing:
//   LOAD_SOUND        → CreateOrGetSound() + CallbackQueue result
//   PLAY_SOUND        → StartSound() or StartSoundAt() based on position
//   STOP_SOUND        → StopSound()
//   SET_VOLUME        → SetSoundPlaybackVolume() (or master volume if soundId == 0)
//   UPDATE_3D_POSITION → SetSoundPosition()
//----------------------------------------------------------------------------------------------------
void AudioSystem::ProcessPendingCommands()
{
    if (!m_commandQueue)
    {
        return; // Command queue not configured - skip processing
    }

    m_commandQueue->ConsumeAll([this](AudioCommand const& cmd) {
        switch (cmd.type)
        {
        case AudioCommandType::LOAD_SOUND:
        {
            auto const& data = std::get<SoundLoadData>(cmd.data);

            // Attempt to load sound file
            SoundID const loadedSoundId = CreateOrGetSound(data.soundPath, eAudioSystemSoundDimension::Sound2D);

            // Send result back to JavaScript via callback queue
            if (m_callbackQueue)
            {
                if (loadedSoundId != MISSING_SOUND_ID)
                {
                    // Success: Return SoundID
                    CallbackData result;
                    result.callbackId    = data.callbackId;
                    result.resultId      = loadedSoundId;
                    result.errorMessage  = ""; // Empty = success
                    result.type          = CallbackType::RESOURCE_LOADED;
                    m_callbackQueue->Submit(result);

                    DAEMON_LOG(LogAudio, eLogVerbosity::Log,
                               Stringf("AudioSystem: LOAD_SOUND success - path='%s', soundId=%llu, callbackId=%llu",
                                   data.soundPath.c_str(), loadedSoundId, data.callbackId));
                }
                else
                {
                    // Failure: Return error message
                    CallbackData result;
                    result.callbackId    = data.callbackId;
                    result.resultId      = MISSING_SOUND_ID;
                    result.errorMessage  = "Failed to load sound: " + data.soundPath;
                    result.type          = CallbackType::RESOURCE_LOADED;
                    m_callbackQueue->Submit(result);

                    DAEMON_LOG(LogAudio, eLogVerbosity::Warning,
                               Stringf("AudioSystem: LOAD_SOUND failed - path='%s', callbackId=%llu",
                                   data.soundPath.c_str(), data.callbackId));
                }
            }
            break;
        }

        case AudioCommandType::PLAY_SOUND:
        {
            auto const& data = std::get<SoundPlayData>(cmd.data);

            SoundPlaybackID playbackId = MISSING_SOUND_ID;

            // Check if 3D positioned sound (position != zero)
            if (data.position.x != 0.f || data.position.y != 0.f || data.position.z != 0.f)
            {
                // 3D positioned sound
                playbackId = StartSoundAt(cmd.soundId, data.position, data.looped, data.volume);
            }
            else
            {
                // 2D non-positioned sound
                playbackId = StartSound(cmd.soundId, data.looped, data.volume);
            }

            if (playbackId != MISSING_SOUND_ID)
            {
                DAEMON_LOG(LogAudio, eLogVerbosity::Log,
                           Stringf("AudioSystem: PLAY_SOUND - soundId=%llu, playbackId=%llu, volume=%.2f, looped=%d",
                               cmd.soundId, playbackId, data.volume, data.looped));
            }
            else
            {
                DAEMON_LOG(LogAudio, eLogVerbosity::Warning,
                           Stringf("AudioSystem: PLAY_SOUND failed - soundId=%llu (invalid or not loaded)",
                               cmd.soundId));
            }
            break;
        }

        case AudioCommandType::STOP_SOUND:
        {
            // cmd.soundId is actually SoundPlaybackID for STOP_SOUND
            StopSound(cmd.soundId);

            DAEMON_LOG(LogAudio, eLogVerbosity::Log,
                       Stringf("AudioSystem: STOP_SOUND - playbackId=%llu", cmd.soundId));
            break;
        }

        case AudioCommandType::SET_VOLUME:
        {
            auto const& data = std::get<VolumeUpdateData>(cmd.data);

            if (cmd.soundId == 0)
            {
                // Master volume control (not yet implemented in AudioSystem)
                // Future: Add m_masterVolume member and apply to all playback instances
                DAEMON_LOG(LogAudio, eLogVerbosity::Warning,
                           Stringf("AudioSystem: SET_VOLUME for master volume not yet implemented (volume=%.2f)",
                               data.volume));
            }
            else
            {
                // Per-playback volume (cmd.soundId is SoundPlaybackID)
                SetSoundPlaybackVolume(cmd.soundId, data.volume);

                DAEMON_LOG(LogAudio, eLogVerbosity::Log,
                           Stringf("AudioSystem: SET_VOLUME - playbackId=%llu, volume=%.2f",
                               cmd.soundId, data.volume));
            }
            break;
        }

        case AudioCommandType::UPDATE_3D_POSITION:
        {
            auto const& data = std::get<Position3DUpdateData>(cmd.data);

            // cmd.soundId is SoundPlaybackID for 3D position updates
            SetSoundPosition(cmd.soundId, data.position);

            DAEMON_LOG(LogAudio, eLogVerbosity::Log,
                       Stringf("AudioSystem: UPDATE_3D_POSITION - playbackId=%llu, pos=(%.2f, %.2f, %.2f)",
                           cmd.soundId, data.position.x, data.position.y, data.position.z));
            break;
        }

        default:
            DAEMON_LOG(LogAudio, eLogVerbosity::Warning,
                       Stringf("AudioSystem: Unknown AudioCommandType=%d", static_cast<int>(cmd.type)));
            break;
        }
    });
}
#endif // ENGINE_SCRIPTING_ENABLED

#endif // !defined( ENGINE_DISABLE_AUDIO )
