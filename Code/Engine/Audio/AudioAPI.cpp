// //----------------------------------------------------------------------------------------------------
// // AudioAPI.cpp
// // Engine Audio Module - Audio Management API Implementation
// //----------------------------------------------------------------------------------------------------
//
// #include "Engine/Audio/AudioAPI.hpp"
// #include "Engine/Audio/AudioCommand.hpp"
// #include "Engine/Audio/AudioCommandQueue.hpp"
// #include "Engine/Core/CallbackQueue.hpp"
// #include "Engine/Core/CallbackData.hpp"
// #include "Engine/Script/ScriptSubsystem.hpp"
// #include "Engine/Core/ErrorWarningAssert.hpp"
// #include "Engine/Core/StringUtils.hpp"
// #include "Engine/Core/EngineCommon.hpp"
// #include "Engine/Core/LogSubsystem.hpp"
// #include "Engine/Math/Vec3.hpp"
//
// //----------------------------------------------------------------------------------------------------
// // Construction / Destruction
// //----------------------------------------------------------------------------------------------------
//
// AudioAPI::AudioAPI(AudioCommandQueue* commandQueue,
//                    ScriptSubsystem*   scriptSubsystem,
//                    CallbackQueue*     callbackQueue)
// 	: m_commandQueue(commandQueue)
// 	, m_scriptSubsystem(scriptSubsystem)
// 	, m_callbackQueue(callbackQueue)
// 	, m_nextCallbackId(1)  // Start callback IDs at 1 (0 reserved for invalid)
// {
// 	GUARANTEE_OR_DIE(m_commandQueue != nullptr, "AudioAPI: AudioCommandQueue is nullptr!");
// 	GUARANTEE_OR_DIE(m_scriptSubsystem != nullptr, "AudioAPI: ScriptSubsystem is nullptr!");
// 	GUARANTEE_OR_DIE(m_callbackQueue != nullptr, "AudioAPI: CallbackQueue is nullptr!");
//
// 	DAEMON_LOG(LogAudio, eLogVerbosity::Display, "AudioAPI: Initialized (Phase 5)");
// }
//
// //----------------------------------------------------------------------------------------------------
// AudioAPI::~AudioAPI()
// {
// 	// Log any pending callbacks that were never executed
// 	if (!m_pendingCallbacks.empty())
// 	{
// 		DAEMON_LOG(LogAudio, eLogVerbosity::Warning,
// 		           Stringf("AudioAPI: Warning - %zu pending callbacks not executed at shutdown",
// 		                   m_pendingCallbacks.size()));
// 	}
// }
//
// //----------------------------------------------------------------------------------------------------
// // Sound Loading
// //----------------------------------------------------------------------------------------------------
//
// CallbackID AudioAPI::LoadSoundAsync(std::string const&    soundPath,
//                                     ScriptCallback const& callback)
// {
// 	// Generate unique callback ID
// 	CallbackID callbackId = GenerateCallbackID();
//
// 	DAEMON_LOG(LogAudio, eLogVerbosity::Verbose,
// 	           Stringf("AudioAPI::LoadSoundAsync - soundPath=%s, callbackId=%llu",
// 	                   soundPath.c_str(), callbackId));
//
// 	// Store callback in pending map (resultId=0, ready=false until command processed)
// 	PendingCallback pending;
// 	pending.callback = callback;
// 	pending.resultId = 0;
// 	pending.ready    = false;
// 	m_pendingCallbacks[callbackId] = pending;
//
// 	// Create sound load command
// 	SoundLoadData loadData;
// 	loadData.soundPath  = soundPath;
// 	loadData.callbackId = callbackId;
//
// 	AudioCommand command(AudioCommandType::LOAD_SOUND, 0, loadData);
//
// 	// Submit command to queue
// 	m_commandQueue->Submit(command);
//
// 	DAEMON_LOG(LogAudio, eLogVerbosity::Verbose,
// 	           "AudioAPI::LoadSoundAsync - Command submitted successfully to queue");
//
// 	return callbackId;
// }
//
// //----------------------------------------------------------------------------------------------------
// // Sound Playback
// //----------------------------------------------------------------------------------------------------
//
// void AudioAPI::PlaySound(SoundID soundId, float volume, bool looped, Vec3 const& position)
// {
// 	DAEMON_LOG(LogAudio, eLogVerbosity::Verbose,
// 	           Stringf("AudioAPI::PlaySound - soundId=%llu, volume=%.2f, looped=%d, pos=(%.1f,%.1f,%.1f)",
// 	                   soundId, volume, looped, position.x, position.y, position.z));
//
// 	// Create sound play command
// 	SoundPlayData playData;
// 	playData.volume   = volume;
// 	playData.looped   = looped;
// 	playData.position = position;
//
// 	AudioCommand command(AudioCommandType::PLAY_SOUND, soundId, playData);
//
// 	// Submit command to queue
// 	m_commandQueue->Submit(command);
// }
//
// //----------------------------------------------------------------------------------------------------
// void AudioAPI::StopSound(SoundID soundId)
// {
// 	DAEMON_LOG(LogAudio, eLogVerbosity::Verbose,
// 	           Stringf("AudioAPI::StopSound - soundId=%llu", soundId));
//
// 	// Create sound stop command
// 	SoundStopData stopData;
//
// 	AudioCommand command(AudioCommandType::STOP_SOUND, soundId, stopData);
//
// 	// Submit command to queue
// 	m_commandQueue->Submit(command);
// }
//
// //----------------------------------------------------------------------------------------------------
// // Sound Updates
// //----------------------------------------------------------------------------------------------------
//
// void AudioAPI::SetVolume(SoundID soundId, float volume)
// {
// 	DAEMON_LOG(LogAudio, eLogVerbosity::Verbose,
// 	           Stringf("AudioAPI::SetVolume - soundId=%llu, volume=%.2f", soundId, volume));
//
// 	// Create volume update command
// 	VolumeUpdateData volumeData;
// 	volumeData.volume = volume;
//
// 	AudioCommand command(AudioCommandType::SET_VOLUME, soundId, volumeData);
//
// 	// Submit command to queue
// 	m_commandQueue->Submit(command);
// }
//
// //----------------------------------------------------------------------------------------------------
// void AudioAPI::Update3DPosition(SoundID soundId, Vec3 const& position)
// {
// 	DAEMON_LOG(LogAudio, eLogVerbosity::Verbose,
// 	           Stringf("AudioAPI::Update3DPosition - soundId=%llu, pos=(%.1f,%.1f,%.1f)",
// 	                   soundId, position.x, position.y, position.z));
//
// 	// Create position update command
// 	Position3DUpdateData positionData;
// 	positionData.position = position;
//
// 	AudioCommand command(AudioCommandType::UPDATE_3D_POSITION, soundId, positionData);
//
// 	// Submit command to queue
// 	m_commandQueue->Submit(command);
// }
//
// //----------------------------------------------------------------------------------------------------
// // Callback Execution
// //----------------------------------------------------------------------------------------------------
//
// void AudioAPI::ExecutePendingCallbacks(CallbackQueue* callbackQueue)
// {
// 	// Phase 5: Enqueue callbacks to CallbackQueue for JavaScript worker thread execution
// 	// Note: This is called on C++ main thread, enqueues for JavaScript worker thread
//
// 	GUARANTEE_OR_DIE(callbackQueue != nullptr, "AudioAPI::ExecutePendingCallbacks - CallbackQueue is nullptr!");
//
// 	// Iterate pending callbacks and enqueue ready ones
// 	for (auto it = m_pendingCallbacks.begin(); it != m_pendingCallbacks.end(); ++it)
// 	{
// 		CallbackID       callbackId = it->first;
// 		PendingCallback& pending    = it->second;
//
// 		if (pending.ready)
// 		{
// 			// Create CallbackData from pending callback
// 			CallbackData data;
// 			data.callbackId   = callbackId;
// 			data.resultId     = pending.resultId;
// 			data.errorMessage = "";  // Empty = success
// 			data.type         = CallbackType::RESOURCE_LOADED;  // Audio sound loading
//
// 			// Enqueue to CallbackQueue for JavaScript execution
// 			bool enqueued = callbackQueue->Enqueue(data);
//
// 			if (!enqueued)
// 			{
// 				// Queue full - log warning and continue (callback dropped)
// 				DAEMON_LOG(LogAudio, eLogVerbosity::Warning,
// 				           Stringf("AudioAPI::ExecutePendingCallbacks - CallbackQueue full! Dropped callback %llu for sound %llu",
// 				               callbackId, pending.resultId));
// 			}
//
// 			DAEMON_LOG(LogAudio, eLogVerbosity::Verbose,
// 			           Stringf("AudioAPI::ExecutePendingCallbacks - Callback %llu enqueued for JavaScript execution (soundId=%llu)",
// 			                   callbackId, pending.resultId));
//
// 			// Phase 5: Do NOT erase here! Callback will be erased after execution
// 			// Callback ownership remains in m_pendingCallbacks until executed
// 			// Note: Unlike EntityAPI, AudioAPI doesn't have ExecuteCallback() method yet
// 			// For now, we'll erase immediately after enqueuing (simplified pattern)
// 			// TODO: Add ExecuteCallback() method in future phases for consistency
// 		}
// 	}
//
// 	// Phase 5: Erase all ready callbacks (simplified pattern for now)
// 	// TODO: Move this to ExecuteCallback() method in future phases
// 	for (auto it = m_pendingCallbacks.begin(); it != m_pendingCallbacks.end(); )
// 	{
// 		if (it->second.ready)
// 		{
// 			it = m_pendingCallbacks.erase(it);
// 		}
// 		else
// 		{
// 			++it;
// 		}
// 	}
// }
//
// //----------------------------------------------------------------------------------------------------
// void AudioAPI::NotifyCallbackReady(CallbackID callbackId, SoundID resultId)
// {
// 	// Find callback in pending map
// 	auto it = m_pendingCallbacks.find(callbackId);
// 	if (it == m_pendingCallbacks.end())
// 	{
// 		DAEMON_LOG(LogAudio, eLogVerbosity::Warning,
// 		           Stringf("AudioAPI::NotifyCallbackReady - CallbackID %llu not found in pending callbacks!",
// 		                   callbackId));
// 		return;
// 	}
//
// 	// Mark callback as ready and store resultId
// 	it->second.ready    = true;
// 	it->second.resultId = resultId;
//
// 	DAEMON_LOG(LogAudio, eLogVerbosity::Verbose,
// 	           Stringf("AudioAPI::NotifyCallbackReady - CallbackID %llu marked ready with soundId %llu",
// 	                   callbackId, resultId));
//
// 	// Note: Actual callback execution happens in ExecutePendingCallbacks()
// 	// This separation allows main thread to control when callbacks are processed
// }
//
// //----------------------------------------------------------------------------------------------------
// // Private Helper Methods
// //----------------------------------------------------------------------------------------------------
//
// CallbackID AudioAPI::GenerateCallbackID()
// {
// 	// Simple atomic increment (Phase 5: No collision detection needed yet)
// 	// Future enhancement: Check for wraparound or collision if needed
// 	return m_nextCallbackId++;
// }
