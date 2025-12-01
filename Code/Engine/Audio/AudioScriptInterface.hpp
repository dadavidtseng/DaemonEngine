//----------------------------------------------------------------------------------------------------
// AudioScriptInterface.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
//----------------------------------------------------------------------------------------------------
#include "Game/EngineBuildPreferences.hpp"  // Phase 2: Required for ENGINE_SCRIPTING_ENABLED
#include "Engine/Script/IScriptableObject.hpp"

//----------------------------------------------------------------------------------------------------

//-Forward-Declaration--------------------------------------------------------------------------------
class AudioSystem;

#ifdef ENGINE_SCRIPTING_ENABLED
class AudioCommandQueue;
class CallbackQueue;
#endif

//----------------------------------------------------------------------------------------------------
/// @brief JavaScript interface for AudioSystem integration providing comprehensive audio control
///
/// @remark Exposes FMOD-based audio functionality to JavaScript including sound loading, playback,
///         3D spatial audio, and real-time audio parameter control for game audio programming.
///
/// @remark Implements method registry pattern for efficient JavaScript method dispatch and
///         provides type-safe parameter validation for all audio operations.
///
/// @see AudioSystem for underlying FMOD audio implementation
/// @see IScriptableObject for JavaScript integration framework
//----------------------------------------------------------------------------------------------------
class AudioScriptInterface : public IScriptableObject
{
public:
    /// @brief Construct AudioScriptInterface with AudioSystem reference for audio operations
    ///
    /// @param audioSystem Valid AudioSystem instance for performing audio operations
    ///
    /// @remark AudioSystem must remain valid for lifetime of this interface object.
    /// @remark Automatically initializes method registry for efficient JavaScript dispatch.
    explicit AudioScriptInterface(AudioSystem* audioSystem);

    std::vector<ScriptMethodInfo> GetAvailableMethods() const override;
    StringList                    GetAvailableProperties() const override;

    ScriptMethodResult CallMethod(String const& methodName, ScriptArgs const& args) override;
    std::any           GetProperty(String const& propertyName) const override;
    bool               SetProperty(String const& propertyName, std::any const& value) override;

#ifdef ENGINE_SCRIPTING_ENABLED
    /// @brief Configure command queue for async audio command processing from JavaScript
    ///
    /// @param commandQueue Pointer to AudioCommandQueue for submitting async commands
    /// @param callbackQueue Pointer to CallbackQueue for receiving async results
    ///
    /// @remark Must be called to enable async audio methods (loadSoundAsync, playSoundAsync, etc.)
    /// @remark Pointers stored by reference - caller retains ownership
    /// @warning commandQueue and callbackQueue must remain valid for interface lifetime
    void SetCommandQueue(AudioCommandQueue* commandQueue, CallbackQueue* callbackQueue);
#endif

private:
    AudioSystem* m_audioSystem = nullptr;

    // === METHOD REGISTRY FOR EFFICIENT DISPATCH ===
    void InitializeMethodRegistry() override;

    // === SOUND LOADING AND MANAGEMENT ===
    ScriptMethodResult ExecuteCreateOrGetSound(ScriptArgs const& args);

    // === 2D SOUND PLAYBACK METHODS ===
    ScriptMethodResult ExecuteStartSound(ScriptArgs const& args);
    ScriptMethodResult ExecuteStartSoundAdvanced(ScriptArgs const& args);

    // === 3D SPATIAL SOUND METHODS ===
    ScriptMethodResult ExecuteStartSoundAt(ScriptArgs const& args);
    ScriptMethodResult ExecuteStartSoundAtAdvanced(ScriptArgs const& args);

    // === PLAYBACK CONTROL METHODS ===
    ScriptMethodResult ExecuteStopSound(ScriptArgs const& args);
    ScriptMethodResult ExecuteSetSoundVolume(ScriptArgs const& args);
    ScriptMethodResult ExecuteSetSoundBalance(ScriptArgs const& args);
    ScriptMethodResult ExecuteSetSoundSpeed(ScriptArgs const& args);

    // === 3D LISTENER CONTROL ===
    ScriptMethodResult ExecuteSetNumListeners(ScriptArgs const& args);
    ScriptMethodResult ExecuteUpdateListener(ScriptArgs const& args);

    // === UTILITY METHODS ===
    ScriptMethodResult ExecuteIsValidSoundID(ScriptArgs const& args);
    ScriptMethodResult ExecuteIsValidPlaybackID(ScriptArgs const& args);

#ifdef ENGINE_SCRIPTING_ENABLED
    // === ASYNC AUDIO METHODS (via AudioCommandQueue) ===
    ScriptMethodResult ExecuteLoadSoundAsync(ScriptArgs const& args);
    ScriptMethodResult ExecutePlaySoundAsync(ScriptArgs const& args);
    ScriptMethodResult ExecuteStopSoundAsync(ScriptArgs const& args);
    ScriptMethodResult ExecuteSetVolumeAsync(ScriptArgs const& args);
    ScriptMethodResult ExecuteUpdate3DPositionAsync(ScriptArgs const& args);
#endif

    // === VALIDATION AND SECURITY ===
    bool ValidateSoundPath(String const& soundPath) const;
    bool ValidateVolume(float volume) const;
    bool ValidateBalance(float balance) const;
    bool ValidateSpeed(float speed) const;
    bool ValidatePosition(float x, float y, float z) const;

#ifdef ENGINE_SCRIPTING_ENABLED
private:
    // === ASYNC COMMAND QUEUE (for JavaScript async audio) ===
    AudioCommandQueue* m_commandQueue  = nullptr;
    CallbackQueue*     m_callbackQueue = nullptr;

    // === CALLBACK ID GENERATION ===
    uint64_t m_nextCallbackId = 1; // Auto-increment for unique callback IDs

    uint64_t GenerateCallbackId();
#endif
};
