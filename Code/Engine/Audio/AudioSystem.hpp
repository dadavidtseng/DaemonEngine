//----------------------------------------------------------------------------------------------------
// AudioSystem.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include <map>
#include <string>
#include <vector>

#include "Engine/Core/StringUtils.hpp"
#include "ThirdParty/fmod/fmod.hpp"

//----------------------------------------------------------------------------------------------------
typedef size_t   SoundID;
typedef size_t   SoundPlaybackID;
size_t constexpr MISSING_SOUND_ID = static_cast<size_t>(-1); // for bad SoundIDs and SoundPlaybackIDs

//-Forward-Declaration--------------------------------------------------------------------------------
struct Vec3;

enum class eAudioSystemSoundDimension : int8_t
{
    Sound2D,
    Sound3D,
};

//----------------------------------------------------------------------------------------------------
struct sAudioSystemConfig
{
};

/////////////////////////////////////////////////////////////////////////////////////////////////
class AudioSystem
{
public:
    explicit AudioSystem(sAudioSystemConfig const& config);
    virtual  ~AudioSystem() = default;

    void         Startup();
    void         Shutdown();
    virtual void BeginFrame();
    virtual void EndFrame();

    virtual SoundID         CreateOrGetSound(String const& soundFilePath, eAudioSystemSoundDimension dimension);
    virtual SoundPlaybackID StartSound(SoundID soundID, bool isLooped = false, float volume = 1.f, float balance = 0.f, float speed = 1.f, bool isPaused = false);
    virtual SoundPlaybackID StartSoundAt(SoundID soundID, Vec3 const& soundPosition, bool isLooped = false, float volume = 10.0f, float balance = 0.0f, float speed = 1.0f, bool isPaused = false);
    virtual void            StopSound(SoundPlaybackID soundPlaybackID);
    virtual void            SetSoundPlaybackVolume(SoundPlaybackID soundPlaybackID, float volume);     // volume is in [0,1]
    virtual void            SetSoundPlaybackBalance(SoundPlaybackID soundPlaybackID, float balance);   // balance is in [-1,1], where 0 is L/R centered
    virtual void            SetSoundPlaybackSpeed(SoundPlaybackID soundPlaybackID, float speed);       // speed is frequency multiplier (1.0 == normal)
    virtual void            ValidateResult(FMOD_RESULT result);

    void         SetNumListeners(int numListeners) const;
    void         UpdateListener(int listenerIndex, Vec3 const& listenerPosition, Vec3 const& listenerForward, Vec3 const& listenerUp) const;
    virtual void SetSoundPosition(SoundPlaybackID soundPlaybackID, const Vec3& soundPosition);
    bool         IsPlaying(SoundPlaybackID soundPlaybackID);
    FMOD_VECTOR  CastVec3ToFmodVec(Vec3 const& vectorToCast) const;
    FMOD_VECTOR  CreateZeroVector() const;

protected:
    FMOD::System*             m_fmodSystem;
    std::map<String, SoundID> m_registeredSoundIDs;
    std::vector<FMOD::Sound*> m_registeredSounds;

private:
    sAudioSystemConfig m_audioConfig;
};
