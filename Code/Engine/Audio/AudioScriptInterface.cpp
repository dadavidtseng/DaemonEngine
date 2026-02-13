// //----------------------------------------------------------------------------------------------------
// // AudioScriptInterface.cpp
// //----------------------------------------------------------------------------------------------------
//
// //----------------------------------------------------------------------------------------------------
// #include "Engine/Audio/AudioScriptInterface.hpp"
//
// //----------------------------------------------------------------------------------------------------
// #include "Engine/Audio/AudioSystem.hpp"
// #include "Engine/Core/EngineCommon.hpp"
// #include "Engine/Core/ErrorWarningAssert.hpp"
// #include "Engine/Core/LogSubsystem.hpp"
// #include "Engine/Math/Vec3.hpp"
// #include "Engine/Script/ScriptTypeExtractor.hpp"
//
// #ifdef ENGINE_SCRIPTING_ENABLED
// #include "Engine/Audio/AudioCommand.hpp"
// #include "Engine/Audio/AudioCommandQueue.hpp"
// #include "Engine/Core/CallbackQueue.hpp"
// #include "Engine/Core/CallbackData.hpp"
// #endif
//
// //----------------------------------------------------------------------------------------------------
// AudioScriptInterface::AudioScriptInterface(AudioSystem* audioSystem)
//     : m_audioSystem(audioSystem)
// {
//     if (!m_audioSystem)
//     {
//         ERROR_AND_DIE("AudioScriptInterface: AudioSystem pointer cannot be null")
//     }
//
//     // Initialize method registry for efficient dispatch
//     AudioScriptInterface::InitializeMethodRegistry();
// }
//
// //----------------------------------------------------------------------------------------------------
// void AudioScriptInterface::InitializeMethodRegistry()
// {
//     // === SOUND LOADING AND MANAGEMENT ===
//     m_methodRegistry["createOrGetSound"] = [this](ScriptArgs const& args) { return ExecuteCreateOrGetSound(args); };
//
//     // === 2D SOUND PLAYBACK METHODS ===
//     m_methodRegistry["startSound"]         = [this](ScriptArgs const& args) { return ExecuteStartSound(args); };
//     m_methodRegistry["startSoundAdvanced"] = [this](ScriptArgs const& args) { return ExecuteStartSoundAdvanced(args); };
//
//     // === 3D SPATIAL SOUND METHODS ===
//     m_methodRegistry["startSoundAt"]         = [this](ScriptArgs const& args) { return ExecuteStartSoundAt(args); };
//     m_methodRegistry["startSoundAtAdvanced"] = [this](ScriptArgs const& args) { return ExecuteStartSoundAtAdvanced(args); };
//
//     // === PLAYBACK CONTROL METHODS ===
//     m_methodRegistry["stopSound"]       = [this](ScriptArgs const& args) { return ExecuteStopSound(args); };
//     m_methodRegistry["setSoundVolume"]  = [this](ScriptArgs const& args) { return ExecuteSetSoundVolume(args); };
//     m_methodRegistry["setSoundBalance"] = [this](ScriptArgs const& args) { return ExecuteSetSoundBalance(args); };
//     m_methodRegistry["setSoundSpeed"]   = [this](ScriptArgs const& args) { return ExecuteSetSoundSpeed(args); };
//
//     // === 3D LISTENER CONTROL ===
//     m_methodRegistry["setNumListeners"] = [this](ScriptArgs const& args) { return ExecuteSetNumListeners(args); };
//     m_methodRegistry["updateListener"]  = [this](ScriptArgs const& args) { return ExecuteUpdateListener(args); };
//
//     // === UTILITY METHODS ===
//     m_methodRegistry["isValidSoundID"]    = [this](ScriptArgs const& args) { return ExecuteIsValidSoundID(args); };
//     m_methodRegistry["isValidPlaybackID"] = [this](ScriptArgs const& args) { return ExecuteIsValidPlaybackID(args); };
//
// #ifdef ENGINE_SCRIPTING_ENABLED
//     // === ASYNC AUDIO METHODS (via AudioCommandQueue) ===
//     m_methodRegistry["loadSoundAsync"]         = [this](ScriptArgs const& args) { return ExecuteLoadSoundAsync(args); };
//     m_methodRegistry["playSoundAsync"]         = [this](ScriptArgs const& args) { return ExecutePlaySoundAsync(args); };
//     m_methodRegistry["stopSoundAsync"]         = [this](ScriptArgs const& args) { return ExecuteStopSoundAsync(args); };
//     m_methodRegistry["setVolumeAsync"]         = [this](ScriptArgs const& args) { return ExecuteSetVolumeAsync(args); };
//     m_methodRegistry["update3DPositionAsync"]  = [this](ScriptArgs const& args) { return ExecuteUpdate3DPositionAsync(args); };
// #endif
// }
//
// //----------------------------------------------------------------------------------------------------
// std::vector<ScriptMethodInfo> AudioScriptInterface::GetAvailableMethods() const
// {
//     return {
//         // === SOUND LOADING AND MANAGEMENT ===
//         ScriptMethodInfo("createOrGetSound",
//                          "Load sound file and return sound ID for playback",
//                          {"string", "string"},
//                          "number"),
//
//         // === 2D SOUND PLAYBACK METHODS ===
//         ScriptMethodInfo("startSound",
//                          "Start 2D sound playback with basic parameters",
//                          {"number"},
//                          "number"),
//
//         ScriptMethodInfo("startSoundAdvanced",
//                          "Start 2D sound with advanced control (looped, volume, balance, speed, paused)",
//                          {"number", "bool", "number", "number", "number", "bool"},
//                          "number"),
//
//         // === 3D SPATIAL SOUND METHODS ===
//         ScriptMethodInfo("startSoundAt",
//                          "Start 3D positioned sound at specified world coordinates",
//                          {"number", "number", "number", "number"},
//                          "number"),
//
//         ScriptMethodInfo("startSoundAtAdvanced",
//                          "Start 3D sound with full spatial control parameters",
//                          {"number", "number", "number", "number", "bool", "number", "number", "number", "bool"},
//                          "number"),
//
//         // === PLAYBACK CONTROL METHODS ===
//         ScriptMethodInfo("stopSound",
//                          "Stop active sound playback immediately",
//                          {"number"},
//                          "void"),
//
//         ScriptMethodInfo("setSoundVolume",
//                          "Change volume of playing sound (0.0 to 1.0)",
//                          {"number", "number"},
//                          "void"),
//
//         ScriptMethodInfo("setSoundBalance",
//                          "Change stereo balance of playing sound (-1.0 to 1.0)",
//                          {"number", "number"},
//                          "void"),
//
//         ScriptMethodInfo("setSoundSpeed",
//                          "Change playback speed/pitch of playing sound",
//                          {"number", "number"},
//                          "void"),
//
//         // === 3D LISTENER CONTROL ===
//         ScriptMethodInfo("setNumListeners",
//                          "Configure number of 3D audio listeners",
//                          {"int"},
//                          "void"),
//
//         ScriptMethodInfo("updateListener",
//                          "Update 3D listener position and orientation",
//                          {"int", "number", "number", "number", "number", "number", "number", "number", "number", "number"},
//                          "void"),
//
//         // === UTILITY METHODS ===
//         ScriptMethodInfo("isValidSoundID",
//                          "Check if sound ID is valid",
//                          {"number"},
//                          "bool"),
//
//         ScriptMethodInfo("isValidPlaybackID",
//                          "Check if playback ID is valid",
//                          {"number"},
//                          "bool"),
//
// #ifdef ENGINE_SCRIPTING_ENABLED
//         // === ASYNC AUDIO METHODS (via AudioCommandQueue) ===
//         ScriptMethodInfo("loadSoundAsync",
//                          "Load sound file asynchronously (returns callbackId immediately)",
//                          {"string"},
//                          "number"),
//
//         ScriptMethodInfo("playSoundAsync",
//                          "Play sound asynchronously with volume and loop control",
//                          {"number", "number", "bool"},
//                          "void"),
//
//         ScriptMethodInfo("stopSoundAsync",
//                          "Stop sound playback asynchronously",
//                          {"number"},
//                          "void"),
//
//         ScriptMethodInfo("setVolumeAsync",
//                          "Set playback volume asynchronously",
//                          {"number", "number"},
//                          "void"),
//
//         ScriptMethodInfo("update3DPositionAsync",
//                          "Update 3D spatial position asynchronously",
//                          {"number", "number", "number", "number"},
//                          "void")
// #endif
//     };
// }
//
// //----------------------------------------------------------------------------------------------------
// StringList AudioScriptInterface::GetAvailableProperties() const
// {
//     return {
//         // Audio system doesn't currently expose properties
//         // Could add master volume, listener count, etc. in future
//     };
// }
//
// //----------------------------------------------------------------------------------------------------
// ScriptMethodResult AudioScriptInterface::CallMethod(String const& methodName, ScriptArgs const& args)
// {
//     try
//     {
//         auto it = m_methodRegistry.find(methodName);
//         if (it != m_methodRegistry.end())
//         {
//             return it->second(args);
//         }
//
//         return ScriptMethodResult::Error("Unknown audio method: " + methodName);
//     }
//     catch (std::exception const& e)
//     {
//         return ScriptMethodResult::Error("Audio method execution failed: " + String(e.what()));
//     }
// }
//
// //----------------------------------------------------------------------------------------------------
// std::any AudioScriptInterface::GetProperty(String const& propertyName) const
// {
//     // No properties currently implemented
//     UNUSED(propertyName)
//     return std::any{};
// }
//
// //----------------------------------------------------------------------------------------------------
// bool AudioScriptInterface::SetProperty(String const& propertyName, std::any const& value)
// {
//     // No properties currently implemented
//     UNUSED(propertyName)
//     UNUSED(value)
//     return false;
// }
//
// //----------------------------------------------------------------------------------------------------
// // SOUND LOADING AND MANAGEMENT
// //----------------------------------------------------------------------------------------------------
//
// ScriptMethodResult AudioScriptInterface::ExecuteCreateOrGetSound(ScriptArgs const& args)
// {
//     auto result = ScriptTypeExtractor::ValidateArgCount(args, 2, "createOrGetSound");
//     if (!result.success) return result;
//
//     try
//     {
//         String soundPath    = ScriptTypeExtractor::ExtractString(args[0]);
//         String dimensionStr = ScriptTypeExtractor::ExtractString(args[1]);
//
//         if (!ValidateSoundPath(soundPath))
//         {
//             return ScriptMethodResult::Error("Invalid sound file path: " + soundPath);
//         }
//
//         // Convert dimension string to enum
//         eAudioSystemSoundDimension dimension;
//         if (dimensionStr == "Sound2D" || dimensionStr == "2D")
//         {
//             dimension = eAudioSystemSoundDimension::Sound2D;
//         }
//         else if (dimensionStr == "Sound3D" || dimensionStr == "3D")
//         {
//             dimension = eAudioSystemSoundDimension::Sound3D;
//         }
//         else
//         {
//             return ScriptMethodResult::Error("Invalid sound dimension. Use 'Sound2D' or 'Sound3D'");
//         }
//
//         // Debug logging for AudioSystem call
//         DAEMON_LOG(LogAudio, eLogVerbosity::Log, StringFormat("Attempting to load sound: {} with dimension: {}", soundPath.c_str(), (int)dimension));
//
//         SoundID soundID = m_audioSystem->CreateOrGetSound(soundPath, dimension);
//
//         DAEMON_LOG(LogAudio, eLogVerbosity::Log, StringFormat("AudioSystem->CreateOrGetSound returned SoundID: {} (MISSING_SOUND_ID = {})", soundID, MISSING_SOUND_ID));
//
//         if (soundID == MISSING_SOUND_ID)
//         {
//             DAEMON_LOG(LogAudio, eLogVerbosity::Warning, StringFormat("Failed to load sound file: '{}' - AudioSystem returned MISSING_SOUND_ID", soundPath.c_str()));
//             return ScriptMethodResult::Error("Failed to load sound: " + soundPath);
//         }
//
//         return ScriptMethodResult::Success(static_cast<double>(soundID));
//     }
//     catch (const std::exception& e)
//     {
//         return ScriptMethodResult::Error("Failed to create sound: " + String(e.what()));
//     }
// }
//
// //----------------------------------------------------------------------------------------------------
// // 2D SOUND PLAYBACK METHODS
// //----------------------------------------------------------------------------------------------------
//
// ScriptMethodResult AudioScriptInterface::ExecuteStartSound(ScriptArgs const& args)
// {
//     auto result = ScriptTypeExtractor::ValidateArgCount(args, 1, "startSound");
//     if (!result.success) return result;
//
//     try
//     {
//         SoundID soundID = static_cast<SoundID>(ScriptTypeExtractor::ExtractDouble(args[0]));
//
//         SoundPlaybackID playbackID = m_audioSystem->StartSound(soundID);
//
//         if (playbackID == MISSING_SOUND_ID)
//         {
//             return ScriptMethodResult::Error("Failed to start sound playback");
//         }
//
//         return ScriptMethodResult::Success(static_cast<double>(playbackID));
//     }
//     catch (const std::exception& e)
//     {
//         return ScriptMethodResult::Error("Failed to start sound: " + String(e.what()));
//     }
// }
//
// //----------------------------------------------------------------------------------------------------
// ScriptMethodResult AudioScriptInterface::ExecuteStartSoundAdvanced(ScriptArgs const& args)
// {
//     auto result = ScriptTypeExtractor::ValidateArgCount(args, 6, "startSoundAdvanced");
//     if (!result.success) return result;
//
//     try
//     {
//         SoundID soundID  = static_cast<SoundID>(ScriptTypeExtractor::ExtractDouble(args[0]));
//         bool    isLooped = ScriptTypeExtractor::ExtractBool(args[1]);
//         float   volume   = ScriptTypeExtractor::ExtractFloat(args[2]);
//         float   balance  = ScriptTypeExtractor::ExtractFloat(args[3]);
//         float   speed    = ScriptTypeExtractor::ExtractFloat(args[4]);
//         bool    isPaused = ScriptTypeExtractor::ExtractBool(args[5]);
//
//         if (!ValidateVolume(volume))
//         {
//             return ScriptMethodResult::Error("Volume must be between 0.0 and 1.0");
//         }
//         if (!ValidateBalance(balance))
//         {
//             return ScriptMethodResult::Error("Balance must be between -1.0 and 1.0");
//         }
//         if (!ValidateSpeed(speed))
//         {
//             return ScriptMethodResult::Error("Speed must be between 0.1 and 10.0");
//         }
//
//         SoundPlaybackID playbackID = m_audioSystem->StartSound(soundID, isLooped, volume, balance, speed, isPaused);
//
//         if (playbackID == MISSING_SOUND_ID)
//         {
//             return ScriptMethodResult::Error("Failed to start advanced sound playback");
//         }
//
//         return ScriptMethodResult::Success(static_cast<double>(playbackID));
//     }
//     catch (const std::exception& e)
//     {
//         return ScriptMethodResult::Error("Failed to start advanced sound: " + String(e.what()));
//     }
// }
//
// //----------------------------------------------------------------------------------------------------
// // 3D SPATIAL SOUND METHODS
// //----------------------------------------------------------------------------------------------------
//
// ScriptMethodResult AudioScriptInterface::ExecuteStartSoundAt(ScriptArgs const& args)
// {
//     auto result = ScriptTypeExtractor::ValidateArgCount(args, 4, "startSoundAt");
//     if (!result.success) return result;
//
//     try
//     {
//         SoundID soundID = static_cast<SoundID>(ScriptTypeExtractor::ExtractDouble(args[0]));
//         float   x       = ScriptTypeExtractor::ExtractFloat(args[1]);
//         float   y       = ScriptTypeExtractor::ExtractFloat(args[2]);
//         float   z       = ScriptTypeExtractor::ExtractFloat(args[3]);
//
//         if (!ValidatePosition(x, y, z))
//         {
//             return ScriptMethodResult::Error("Invalid 3D position coordinates");
//         }
//
//         Vec3            position(x, y, z);
//         SoundPlaybackID playbackID = m_audioSystem->StartSoundAt(soundID, position);
//
//         if (playbackID == MISSING_SOUND_ID)
//         {
//             return ScriptMethodResult::Error("Failed to start 3D sound playback");
//         }
//
//         return ScriptMethodResult::Success(static_cast<double>(playbackID));
//     }
//     catch (const std::exception& e)
//     {
//         return ScriptMethodResult::Error("Failed to start 3D sound: " + String(e.what()));
//     }
// }
//
// //----------------------------------------------------------------------------------------------------
// ScriptMethodResult AudioScriptInterface::ExecuteStartSoundAtAdvanced(ScriptArgs const& args)
// {
//     auto result = ScriptTypeExtractor::ValidateArgCount(args, 9, "startSoundAtAdvanced");
//     if (!result.success) return result;
//
//     try
//     {
//         SoundID soundID  = static_cast<SoundID>(ScriptTypeExtractor::ExtractDouble(args[0]));
//         float   x        = ScriptTypeExtractor::ExtractFloat(args[1]);
//         float   y        = ScriptTypeExtractor::ExtractFloat(args[2]);
//         float   z        = ScriptTypeExtractor::ExtractFloat(args[3]);
//         bool    isLooped = ScriptTypeExtractor::ExtractBool(args[4]);
//         float   volume   = ScriptTypeExtractor::ExtractFloat(args[5]);
//         float   balance  = ScriptTypeExtractor::ExtractFloat(args[6]);
//         float   speed    = ScriptTypeExtractor::ExtractFloat(args[7]);
//         bool    isPaused = ScriptTypeExtractor::ExtractBool(args[8]);
//
//         if (!ValidatePosition(x, y, z))
//         {
//             return ScriptMethodResult::Error("Invalid 3D position coordinates");
//         }
//         if (!ValidateVolume(volume))
//         {
//             return ScriptMethodResult::Error("Volume must be between 0.0 and 10.0 for 3D audio");
//         }
//         if (!ValidateBalance(balance))
//         {
//             return ScriptMethodResult::Error("Balance must be between -1.0 and 1.0");
//         }
//         if (!ValidateSpeed(speed))
//         {
//             return ScriptMethodResult::Error("Speed must be between 0.1 and 10.0");
//         }
//
//         Vec3            position(x, y, z);
//         SoundPlaybackID playbackID = m_audioSystem->StartSoundAt(soundID, position, isLooped, volume, balance, speed, isPaused);
//
//         if (playbackID == MISSING_SOUND_ID)
//         {
//             return ScriptMethodResult::Error("Failed to start advanced 3D sound playback");
//         }
//
//         return ScriptMethodResult::Success(static_cast<double>(playbackID));
//     }
//     catch (const std::exception& e)
//     {
//         return ScriptMethodResult::Error("Failed to start advanced 3D sound: " + String(e.what()));
//     }
// }
//
// //----------------------------------------------------------------------------------------------------
// // PLAYBACK CONTROL METHODS
// //----------------------------------------------------------------------------------------------------
//
// ScriptMethodResult AudioScriptInterface::ExecuteStopSound(ScriptArgs const& args)
// {
//     auto result = ScriptTypeExtractor::ValidateArgCount(args, 1, "stopSound");
//     if (!result.success) return result;
//
//     try
//     {
//         SoundPlaybackID playbackID = static_cast<SoundPlaybackID>(ScriptTypeExtractor::ExtractDouble(args[0]));
//
//         m_audioSystem->StopSound(playbackID);
//
//         return ScriptMethodResult::Success("Sound stopped successfully");
//     }
//     catch (const std::exception& e)
//     {
//         return ScriptMethodResult::Error("Failed to stop sound: " + String(e.what()));
//     }
// }
//
// //----------------------------------------------------------------------------------------------------
// ScriptMethodResult AudioScriptInterface::ExecuteSetSoundVolume(ScriptArgs const& args)
// {
//     auto result = ScriptTypeExtractor::ValidateArgCount(args, 2, "setSoundVolume");
//     if (!result.success) return result;
//
//     try
//     {
//         SoundPlaybackID playbackID = static_cast<SoundPlaybackID>(ScriptTypeExtractor::ExtractDouble(args[0]));
//         float           volume     = ScriptTypeExtractor::ExtractFloat(args[1]);
//
//         if (!ValidateVolume(volume))
//         {
//             return ScriptMethodResult::Error("Volume must be between 0.0 and 1.0");
//         }
//
//         m_audioSystem->SetSoundPlaybackVolume(playbackID, volume);
//
//         return ScriptMethodResult::Success("Volume set successfully");
//     }
//     catch (const std::exception& e)
//     {
//         return ScriptMethodResult::Error("Failed to set volume: " + String(e.what()));
//     }
// }
//
// //----------------------------------------------------------------------------------------------------
// ScriptMethodResult AudioScriptInterface::ExecuteSetSoundBalance(ScriptArgs const& args)
// {
//     auto result = ScriptTypeExtractor::ValidateArgCount(args, 2, "setSoundBalance");
//     if (!result.success) return result;
//
//     try
//     {
//         SoundPlaybackID playbackID = static_cast<SoundPlaybackID>(ScriptTypeExtractor::ExtractDouble(args[0]));
//         float           balance    = ScriptTypeExtractor::ExtractFloat(args[1]);
//
//         if (!ValidateBalance(balance))
//         {
//             return ScriptMethodResult::Error("Balance must be between -1.0 and 1.0");
//         }
//
//         m_audioSystem->SetSoundPlaybackBalance(playbackID, balance);
//
//         return ScriptMethodResult::Success("Balance set successfully");
//     }
//     catch (const std::exception& e)
//     {
//         return ScriptMethodResult::Error("Failed to set balance: " + String(e.what()));
//     }
// }
//
// //----------------------------------------------------------------------------------------------------
// ScriptMethodResult AudioScriptInterface::ExecuteSetSoundSpeed(ScriptArgs const& args)
// {
//     auto result = ScriptTypeExtractor::ValidateArgCount(args, 2, "setSoundSpeed");
//     if (!result.success) return result;
//
//     try
//     {
//         SoundPlaybackID playbackID = static_cast<SoundPlaybackID>(ScriptTypeExtractor::ExtractDouble(args[0]));
//         float           speed      = ScriptTypeExtractor::ExtractFloat(args[1]);
//
//         if (!ValidateSpeed(speed))
//         {
//             return ScriptMethodResult::Error("Speed must be between 0.1 and 10.0");
//         }
//
//         m_audioSystem->SetSoundPlaybackSpeed(playbackID, speed);
//
//         return ScriptMethodResult::Success("Speed set successfully");
//     }
//     catch (const std::exception& e)
//     {
//         return ScriptMethodResult::Error("Failed to set speed: " + String(e.what()));
//     }
// }
//
// //----------------------------------------------------------------------------------------------------
// // 3D LISTENER CONTROL
// //----------------------------------------------------------------------------------------------------
//
// ScriptMethodResult AudioScriptInterface::ExecuteSetNumListeners(ScriptArgs const& args)
// {
//     auto result = ScriptTypeExtractor::ValidateArgCount(args, 1, "setNumListeners");
//     if (!result.success) return result;
//
//     try
//     {
//         int numListeners = ScriptTypeExtractor::ExtractInt(args[0]);
//
//         if (numListeners < 1 || numListeners > 8)
//         {
//             return ScriptMethodResult::Error("Number of listeners must be between 1 and 8");
//         }
//
//         m_audioSystem->SetNumListeners(numListeners);
//
//         return ScriptMethodResult::Success("Number of listeners set successfully");
//     }
//     catch (const std::exception& e)
//     {
//         return ScriptMethodResult::Error("Failed to set number of listeners: " + String(e.what()));
//     }
// }
//
// //----------------------------------------------------------------------------------------------------
// ScriptMethodResult AudioScriptInterface::ExecuteUpdateListener(ScriptArgs const& args)
// {
//     auto result = ScriptTypeExtractor::ValidateArgCount(args, 10, "updateListener");
//     if (!result.success) return result;
//
//     try
//     {
//         int   listenerIndex = ScriptTypeExtractor::ExtractInt(args[0]);
//         float posX          = ScriptTypeExtractor::ExtractFloat(args[1]);
//         float posY          = ScriptTypeExtractor::ExtractFloat(args[2]);
//         float posZ          = ScriptTypeExtractor::ExtractFloat(args[3]);
//         float forwardX      = ScriptTypeExtractor::ExtractFloat(args[4]);
//         float forwardY      = ScriptTypeExtractor::ExtractFloat(args[5]);
//         float forwardZ      = ScriptTypeExtractor::ExtractFloat(args[6]);
//         float upX           = ScriptTypeExtractor::ExtractFloat(args[7]);
//         float upY           = ScriptTypeExtractor::ExtractFloat(args[8]);
//         float upZ           = ScriptTypeExtractor::ExtractFloat(args[9]);
//
//         if (listenerIndex < 0 || listenerIndex > 7)
//         {
//             return ScriptMethodResult::Error("Listener index must be between 0 and 7");
//         }
//
//         Vec3 position(posX, posY, posZ);
//         Vec3 forward(forwardX, forwardY, forwardZ);
//         Vec3 up(upX, upY, upZ);
//
//         m_audioSystem->UpdateListener(listenerIndex, position, forward, up);
//
//         return ScriptMethodResult::Success("Listener updated successfully");
//     }
//     catch (const std::exception& e)
//     {
//         return ScriptMethodResult::Error("Failed to update listener: " + String(e.what()));
//     }
// }
//
// //----------------------------------------------------------------------------------------------------
// // UTILITY METHODS
// //----------------------------------------------------------------------------------------------------
//
// ScriptMethodResult AudioScriptInterface::ExecuteIsValidSoundID(ScriptArgs const& args)
// {
//     auto result = ScriptTypeExtractor::ValidateArgCount(args, 1, "isValidSoundID");
//     if (!result.success) return result;
//
//     try
//     {
//         SoundID soundID = static_cast<SoundID>(ScriptTypeExtractor::ExtractDouble(args[0]));
//         bool    isValid = (soundID != MISSING_SOUND_ID);
//
//         return ScriptMethodResult::Success(isValid);
//     }
//     catch (const std::exception& e)
//     {
//         return ScriptMethodResult::Error("Failed to check sound ID validity: " + String(e.what()));
//     }
// }
//
// //----------------------------------------------------------------------------------------------------
// ScriptMethodResult AudioScriptInterface::ExecuteIsValidPlaybackID(ScriptArgs const& args)
// {
//     auto result = ScriptTypeExtractor::ValidateArgCount(args, 1, "isValidPlaybackID");
//     if (!result.success) return result;
//
//     try
//     {
//         SoundPlaybackID playbackID = static_cast<SoundPlaybackID>(ScriptTypeExtractor::ExtractDouble(args[0]));
//         bool            isValid    = (playbackID != MISSING_SOUND_ID);
//
//         return ScriptMethodResult::Success(isValid);
//     }
//     catch (const std::exception& e)
//     {
//         return ScriptMethodResult::Error("Failed to check playback ID validity: " + String(e.what()));
//     }
// }
//
// //----------------------------------------------------------------------------------------------------
// // VALIDATION AND SECURITY
// //----------------------------------------------------------------------------------------------------
//
// bool AudioScriptInterface::ValidateSoundPath(String const& soundPath) const
// {
//     // Basic path validation - ensure it's not empty and has reasonable length
//     if (soundPath.empty() || soundPath.length() > 260)
//     {
//         return false;
//     }
//
//     // Ensure path starts with Data/ for security
//     if (soundPath.find("Data/") != 0)
//     {
//         return false;
//     }
//
//     // Check for valid audio file extensions
//     String lowerPath = soundPath;
//     std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(), [](unsigned char c) { return static_cast<char>(::tolower(c)); });
//
//     return (lowerPath.ends_with(".mp3") ||
//         lowerPath.ends_with(".wav") ||
//         lowerPath.ends_with(".ogg") ||
//         lowerPath.ends_with(".m4a"));
// }
//
// //----------------------------------------------------------------------------------------------------
// bool AudioScriptInterface::ValidateVolume(float volume) const
// {
//     return (volume >= 0.0f && volume <= 10.0f); // Allow up to 10.0 for 3D audio
// }
//
// //----------------------------------------------------------------------------------------------------
// bool AudioScriptInterface::ValidateBalance(float balance) const
// {
//     return (balance >= -1.0f && balance <= 1.0f);
// }
//
// //----------------------------------------------------------------------------------------------------
// bool AudioScriptInterface::ValidateSpeed(float speed) const
// {
//     return (speed >= 0.1f && speed <= 10.0f);
// }
//
// //----------------------------------------------------------------------------------------------------
// bool AudioScriptInterface::ValidatePosition(float x, float y, float z) const
// {
//     // Check for reasonable 3D world coordinates (not NaN or infinite)
//     return (std::isfinite(x) && std::isfinite(y) && std::isfinite(z) &&
//         std::abs(x) < 10000.0f && std::abs(y) < 10000.0f && std::abs(z) < 10000.0f);
// }
//
// #ifdef ENGINE_SCRIPTING_ENABLED
// //----------------------------------------------------------------------------------------------------
// // ASYNC AUDIO METHODS (via AudioCommandQueue)
// //----------------------------------------------------------------------------------------------------
//
// //----------------------------------------------------------------------------------------------------
// // SetCommandQueue
// //
// // Configure command queue for async audio operations from JavaScript.
// //----------------------------------------------------------------------------------------------------
// void AudioScriptInterface::SetCommandQueue(AudioCommandQueue* commandQueue, CallbackQueue* callbackQueue)
// {
//     m_commandQueue  = commandQueue;
//     m_callbackQueue = callbackQueue;
//
//     DAEMON_LOG(LogAudio, eLogVerbosity::Log,
//                Stringf("AudioScriptInterface: Command queue configured (commandQueue=%p, callbackQueue=%p)",
//                    commandQueue, callbackQueue));
// }
//
// //----------------------------------------------------------------------------------------------------
// // GenerateCallbackId
// //
// // Generate unique callback ID for async operation tracking.
// //----------------------------------------------------------------------------------------------------
// uint64_t AudioScriptInterface::GenerateCallbackId()
// {
//     return m_nextCallbackId++;
// }
//
// //----------------------------------------------------------------------------------------------------
// // ExecuteLoadSoundAsync
// //
// // JavaScript signature: loadSoundAsync(soundPath) → returns callbackId
// // Parameters:
// //   - soundPath: string (e.g., "Data/Audio/explosion.mp3")
// // Returns: callbackId (number) for tracking async result
// //----------------------------------------------------------------------------------------------------
// ScriptMethodResult AudioScriptInterface::ExecuteLoadSoundAsync(ScriptArgs const& args)
// {
//     auto result = ScriptTypeExtractor::ValidateArgCount(args, 1, "loadSoundAsync");
//     if (!result.success) return result;
//
//     // Check if command queue is configured
//     if (!m_commandQueue || !m_callbackQueue)
//     {
//         return ScriptMethodResult::Error("Async audio not enabled - call SetCommandQueue first");
//     }
//
//     try
//     {
//         String soundPath = ScriptTypeExtractor::ExtractString(args[0]);
//
//         // Validate sound path
//         if (!ValidateSoundPath(soundPath))
//         {
//             return ScriptMethodResult::Error("Invalid sound file path: " + soundPath);
//         }
//
//         // Generate unique callback ID
//         uint64_t callbackId = GenerateCallbackId();
//
//         // Create LOAD_SOUND command
//         AudioCommand cmd{
//             AudioCommandType::LOAD_SOUND,
//             0, // soundId not used for LOAD_SOUND
//             SoundLoadData{soundPath, callbackId}
//         };
//
//         // Submit command to queue
//         bool submitted = m_commandQueue->Submit(cmd);
//         if (!submitted)
//         {
//             return ScriptMethodResult::Error("Audio command queue full - try again later");
//         }
//
//         DAEMON_LOG(LogAudio, eLogVerbosity::Log,
//                    Stringf("AudioScriptInterface: loadSoundAsync submitted - path='%s', callbackId=%llu",
//                        soundPath.c_str(), callbackId));
//
//         // Return callbackId immediately (async result via CallbackQueue later)
//         return ScriptMethodResult::Success(static_cast<double>(callbackId));
//     }
//     catch (const std::exception& e)
//     {
//         return ScriptMethodResult::Error("Failed to submit loadSoundAsync: " + String(e.what()));
//     }
// }
//
// //----------------------------------------------------------------------------------------------------
// // ExecutePlaySoundAsync
// //
// // JavaScript signature: playSoundAsync(soundId, volume, looped)
// // Parameters:
// //   - soundId: number (from loadSoundAsync callback)
// //   - volume: number (0.0 - 1.0)
// //   - looped: boolean (true = loop continuously)
// //----------------------------------------------------------------------------------------------------
// ScriptMethodResult AudioScriptInterface::ExecutePlaySoundAsync(ScriptArgs const& args)
// {
//     auto result = ScriptTypeExtractor::ValidateArgCount(args, 3, "playSoundAsync");
//     if (!result.success) return result;
//
//     // Check if command queue is configured
//     if (!m_commandQueue)
//     {
//         return ScriptMethodResult::Error("Async audio not enabled - call SetCommandQueue first");
//     }
//
//     try
//     {
//         SoundID soundId = static_cast<SoundID>(ScriptTypeExtractor::ExtractDouble(args[0]));
//         float   volume  = ScriptTypeExtractor::ExtractFloat(args[1]);
//         bool    looped  = ScriptTypeExtractor::ExtractBool(args[2]);
//
//         // Validate volume
//         if (!ValidateVolume(volume))
//         {
//             return ScriptMethodResult::Error("Volume must be between 0.0 and 1.0");
//         }
//
//         // Create PLAY_SOUND command (2D, no position)
//         AudioCommand cmd{
//             AudioCommandType::PLAY_SOUND,
//             soundId,
//             SoundPlayData{volume, looped, Vec3{0.f, 0.f, 0.f}}
//         };
//
//         // Submit command to queue
//         bool submitted = m_commandQueue->Submit(cmd);
//         if (!submitted)
//         {
//             return ScriptMethodResult::Error("Audio command queue full - try again later");
//         }
//
//         DAEMON_LOG(LogAudio, eLogVerbosity::Log,
//                    Stringf("AudioScriptInterface: playSoundAsync submitted - soundId=%llu, volume=%.2f, looped=%d",
//                        soundId, volume, looped));
//
//         return ScriptMethodResult::Success("Sound playback queued successfully");
//     }
//     catch (const std::exception& e)
//     {
//         return ScriptMethodResult::Error("Failed to submit playSoundAsync: " + String(e.what()));
//     }
// }
//
// //----------------------------------------------------------------------------------------------------
// // ExecuteStopSoundAsync
// //
// // JavaScript signature: stopSoundAsync(playbackId)
// // Parameters:
// //   - playbackId: number (from StartSound result)
// //----------------------------------------------------------------------------------------------------
// ScriptMethodResult AudioScriptInterface::ExecuteStopSoundAsync(ScriptArgs const& args)
// {
//     auto result = ScriptTypeExtractor::ValidateArgCount(args, 1, "stopSoundAsync");
//     if (!result.success) return result;
//
//     // Check if command queue is configured
//     if (!m_commandQueue)
//     {
//         return ScriptMethodResult::Error("Async audio not enabled - call SetCommandQueue first");
//     }
//
//     try
//     {
//         SoundPlaybackID playbackId = static_cast<SoundPlaybackID>(ScriptTypeExtractor::ExtractDouble(args[0]));
//
//         // Create STOP_SOUND command (soundId field is actually playbackId)
//         AudioCommand cmd{
//             AudioCommandType::STOP_SOUND,
//             playbackId,
//             SoundStopData{}
//         };
//
//         // Submit command to queue
//         bool submitted = m_commandQueue->Submit(cmd);
//         if (!submitted)
//         {
//             return ScriptMethodResult::Error("Audio command queue full - try again later");
//         }
//
//         DAEMON_LOG(LogAudio, eLogVerbosity::Log,
//                    Stringf("AudioScriptInterface: stopSoundAsync submitted - playbackId=%llu", playbackId));
//
//         return ScriptMethodResult::Success("Sound stop queued successfully");
//     }
//     catch (const std::exception& e)
//     {
//         return ScriptMethodResult::Error("Failed to submit stopSoundAsync: " + String(e.what()));
//     }
// }
//
// //----------------------------------------------------------------------------------------------------
// // ExecuteSetVolumeAsync
// //
// // JavaScript signature: setVolumeAsync(playbackId, volume)
// // Parameters:
// //   - playbackId: number
// //   - volume: number (0.0 - 1.0)
// //----------------------------------------------------------------------------------------------------
// ScriptMethodResult AudioScriptInterface::ExecuteSetVolumeAsync(ScriptArgs const& args)
// {
//     auto result = ScriptTypeExtractor::ValidateArgCount(args, 2, "setVolumeAsync");
//     if (!result.success) return result;
//
//     // Check if command queue is configured
//     if (!m_commandQueue)
//     {
//         return ScriptMethodResult::Error("Async audio not enabled - call SetCommandQueue first");
//     }
//
//     try
//     {
//         SoundPlaybackID playbackId = static_cast<SoundPlaybackID>(ScriptTypeExtractor::ExtractDouble(args[0]));
//         float           volume     = ScriptTypeExtractor::ExtractFloat(args[1]);
//
//         // Validate volume
//         if (!ValidateVolume(volume))
//         {
//             return ScriptMethodResult::Error("Volume must be between 0.0 and 1.0");
//         }
//
//         // Create SET_VOLUME command (soundId field is actually playbackId)
//         AudioCommand cmd{
//             AudioCommandType::SET_VOLUME,
//             playbackId,
//             VolumeUpdateData{volume}
//         };
//
//         // Submit command to queue
//         bool submitted = m_commandQueue->Submit(cmd);
//         if (!submitted)
//         {
//             return ScriptMethodResult::Error("Audio command queue full - try again later");
//         }
//
//         DAEMON_LOG(LogAudio, eLogVerbosity::Log,
//                    Stringf("AudioScriptInterface: setVolumeAsync submitted - playbackId=%llu, volume=%.2f",
//                        playbackId, volume));
//
//         return ScriptMethodResult::Success("Volume update queued successfully");
//     }
//     catch (const std::exception& e)
//     {
//         return ScriptMethodResult::Error("Failed to submit setVolumeAsync: " + String(e.what()));
//     }
// }
//
// //----------------------------------------------------------------------------------------------------
// // ExecuteUpdate3DPositionAsync
// //
// // JavaScript signature: update3DPositionAsync(playbackId, x, y, z)
// // Parameters:
// //   - playbackId: number
// //   - x, y, z: number (world-space 3D coordinates)
// //----------------------------------------------------------------------------------------------------
// ScriptMethodResult AudioScriptInterface::ExecuteUpdate3DPositionAsync(ScriptArgs const& args)
// {
//     auto result = ScriptTypeExtractor::ValidateArgCount(args, 4, "update3DPositionAsync");
//     if (!result.success) return result;
//
//     // Check if command queue is configured
//     if (!m_commandQueue)
//     {
//         return ScriptMethodResult::Error("Async audio not enabled - call SetCommandQueue first");
//     }
//
//     try
//     {
//         SoundPlaybackID playbackId = static_cast<SoundPlaybackID>(ScriptTypeExtractor::ExtractDouble(args[0]));
//         float           x          = ScriptTypeExtractor::ExtractFloat(args[1]);
//         float           y          = ScriptTypeExtractor::ExtractFloat(args[2]);
//         float           z          = ScriptTypeExtractor::ExtractFloat(args[3]);
//
//         // Validate position
//         if (!ValidatePosition(x, y, z))
//         {
//             return ScriptMethodResult::Error("Invalid 3D position coordinates");
//         }
//
//         // Create UPDATE_3D_POSITION command (soundId field is actually playbackId)
//         AudioCommand cmd{
//             AudioCommandType::UPDATE_3D_POSITION,
//             playbackId,
//             Position3DUpdateData{Vec3{x, y, z}}
//         };
//
//         // Submit command to queue
//         bool submitted = m_commandQueue->Submit(cmd);
//         if (!submitted)
//         {
//             return ScriptMethodResult::Error("Audio command queue full - try again later");
//         }
//
//         DAEMON_LOG(LogAudio, eLogVerbosity::Log,
//                    Stringf("AudioScriptInterface: update3DPositionAsync submitted - playbackId=%llu, pos=(%.2f, %.2f, %.2f)",
//                        playbackId, x, y, z));
//
//         return ScriptMethodResult::Success("3D position update queued successfully");
//     }
//     catch (const std::exception& e)
//     {
//         return ScriptMethodResult::Error("Failed to submit update3DPositionAsync: " + String(e.what()));
//     }
// }
//
// #endif // ENGINE_SCRIPTING_ENABLED
