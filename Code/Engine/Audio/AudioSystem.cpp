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
#endif // !defined( ENGINE_DISABLE_AUDIO )
