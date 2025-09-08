[Root Directory](../../../CLAUDE.md) > [Code](../../) > [Engine](../) > **Audio**

# Audio Module Documentation

## Module Responsibilities

The Audio module provides a comprehensive FMOD-based 3D audio system enabling immersive sound experiences with spatial audio, dynamic sound loading, real-time audio parameter control, and multi-listener support for complex audio scenarios.

## Entry and Startup

### Primary Entry Point
- `AudioSystem.hpp` - Main audio system interface and management

### Initialization Pattern
```cpp
sAudioSystemConfig config;
AudioSystem* audioSystem = new AudioSystem(config);
audioSystem->Startup();

// Basic audio usage
SoundID soundId = audioSystem->CreateOrGetSound("Data/Audio/music.wav", eAudioSystemSoundDimension::Sound2D);
SoundPlaybackID playbackId = audioSystem->StartSound(soundId, false, 1.0f, 0.0f, 1.0f, false);

// 3D positioned sound
SoundID sound3D = audioSystem->CreateOrGetSound("Data/Audio/effect.wav", eAudioSystemSoundDimension::Sound3D);
Vec3 soundPosition(10.0f, 0.0f, 5.0f);
SoundPlaybackID playback3D = audioSystem->StartSoundAt(sound3D, soundPosition, false, 10.0f);
```

## External Interfaces

### Core Audio API
```cpp
class AudioSystem {
    // System lifecycle
    void Startup();
    void Shutdown();
    void BeginFrame();
    void EndFrame();
    
    // Sound loading and management
    SoundID CreateOrGetSound(String const& soundFilePath, eAudioSystemSoundDimension dimension);
    
    // 2D sound playback
    SoundPlaybackID StartSound(SoundID soundID, bool isLooped = false, float volume = 1.f, 
                              float balance = 0.f, float speed = 1.f, bool isPaused = false);
    
    // 3D spatial sound playback
    SoundPlaybackID StartSoundAt(SoundID soundID, Vec3 const& soundPosition, 
                                bool isLooped = false, float volume = 10.0f, 
                                float balance = 0.0f, float speed = 1.0f, bool isPaused = false);
    
    // Playback control
    void StopSound(SoundPlaybackID soundPlaybackID);
    void SetSoundPlaybackVolume(SoundPlaybackID soundPlaybackID, float volume);
    void SetSoundPlaybackBalance(SoundPlaybackID soundPlaybackID, float balance);
    void SetSoundPlaybackSpeed(SoundPlaybackID soundPlaybackID, float speed);
    void SetSoundPosition(SoundPlaybackID soundPlaybackID, const Vec3& soundPosition);
    bool IsPlaying(SoundPlaybackID soundPlaybackID);
};
```

### 3D Audio Listener System
```cpp
// Multi-listener support for complex audio scenarios
void SetNumListeners(int numListeners) const;
void UpdateListener(int listenerIndex, Vec3 const& listenerPosition, 
                   Vec3 const& listenerForward, Vec3 const& listenerUp) const;
```

### Sound Dimensions
```cpp
enum class eAudioSystemSoundDimension : int8_t {
    Sound2D,    // Traditional stereo/mono sounds
    Sound3D     // Spatially positioned sounds with distance attenuation
};
```

## Key Dependencies and Configuration

### External Dependencies
- **FMOD Studio API**: Professional audio middleware (`fmod.lib`, `fmodstudio.lib`)
- **FMOD Core**: Low-level audio processing engine
- FMOD headers from `ThirdParty/fmod/fmod.hpp`

### Internal Dependencies
- Math module for 3D vector operations (`Vec3`)
- Core module for string utilities and basic types
- Platform module for file system access

### Configuration Structure
```cpp
struct sAudioSystemConfig {
    // Currently minimal configuration
    // Future expansion: sample rates, buffer sizes, channel counts
};
```

### Sound Management
```cpp
typedef size_t SoundID;
typedef size_t SoundPlaybackID;
size_t constexpr MISSING_SOUND_ID = static_cast<size_t>(-1);

// Internal sound registry
std::map<String, SoundID> m_registeredSoundIDs;
std::vector<FMOD::Sound*> m_registeredSounds;
```

## Data Models

### FMOD Integration Layer
```cpp
class AudioSystem {
protected:
    FMOD::System* m_fmodSystem;                    // FMOD system instance
    std::map<String, SoundID> m_registeredSoundIDs; // Sound path to ID mapping
    std::vector<FMOD::Sound*> m_registeredSounds;   // Loaded sound assets
};
```

### Vector Conversion Utilities
```cpp
// Conversion between engine Vec3 and FMOD_VECTOR
FMOD_VECTOR CastVec3ToFmodVec(Vec3 const& vectorToCast) const;
FMOD_VECTOR CreateZeroVector() const;
```

### Error Handling
```cpp
// FMOD result validation with comprehensive error reporting
void ValidateResult(FMOD_RESULT result);
```

## Testing and Quality

### Built-in Quality Features
- **FMOD Error Checking**: Automatic validation of all FMOD API calls
- **Sound Asset Validation**: File existence and format checking
- **Memory Management**: Automatic cleanup of FMOD resources
- **Performance Monitoring**: Built-in FMOD performance profiling

### Current Testing Approach
- Manual audio testing through developer console commands
- 3D spatial audio verification through listener movement
- Volume and pitch parameter validation
- Memory usage tracking for loaded audio assets

### Quality Assurance Features
- Automatic sound format support detection
- Robust error handling for missing audio files
- Thread-safe audio operations
- Graceful degradation when audio hardware is unavailable

### Recommended Testing Additions
- Automated audio unit tests for API functions
- 3D spatial audio accuracy testing
- Performance benchmarks for large-scale audio scenarios
- Cross-platform audio compatibility testing

## FAQ

### Q: What audio formats are supported?
A: FMOD supports a wide range of formats including WAV, MP3, OGG, FLAC, AIFF, and many others. Compressed formats are automatically decoded.

### Q: How does 3D spatial audio work?
A: 3D sounds use FMOD's built-in spatial processing with distance attenuation, doppler effects, and listener orientation for realistic positional audio.

### Q: Can I play multiple instances of the same sound?
A: Yes, each `StartSound()` call returns a unique `SoundPlaybackID` for independent control of multiple playback instances.

### Q: How do I implement music with seamless looping?
A: Use `StartSound()` with `isLooped = true`. FMOD handles seamless looping for properly prepared audio files.

### Q: Is the audio system thread-safe?
A: FMOD operations are internally thread-safe, but it's recommended to call audio functions from the main thread for consistency.

### Q: How do I adjust global volume or implement audio categories?
A: Future enhancements will include master volume controls and audio category management through FMOD Studio integration.

## Related Files

### Core Implementation
- `AudioSystem.cpp` - Main audio system implementation with FMOD integration

### Configuration and Constants
- `AudioSystem.hpp` - Public API declarations and type definitions

### Integration Points
The Audio module integrates with:
- **Math Module**: Vec3 for 3D positioning and listener orientation
- **Core Module**: String utilities for file path management
- **Event System**: For audio-triggered events and callbacks
- **Resource System**: For efficient audio asset loading and caching

### Planned Extensions
- FMOD Studio integration for advanced audio features
- Audio streaming for large music files
- Real-time audio effects and filters
- Audio occlusion and obstruction simulation
- Dynamic music system with interactive transitions

## Changelog

- 2025-09-06 21:17:11: Initial Audio module documentation created
- Recent developments: 3D spatial audio system, multi-listener support, comprehensive FMOD integration