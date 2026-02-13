// //----------------------------------------------------------------------------------------------------
// // DebugRenderAPI.cpp
// //----------------------------------------------------------------------------------------------------
// #include "Engine/Renderer/DebugRenderAPI.hpp"
// #include "Engine/Renderer/RenderCommandQueue.hpp"
// #include "Engine/Renderer/RenderCommand.hpp"
// #include "Engine/Renderer/DebugRenderStateBuffer.hpp"
// #include "Engine/Renderer/DebugRenderSystem.hpp"
// #include "Engine/Core/ErrorWarningAssert.hpp"
// #include "Engine/Core/CallbackQueue.hpp"
// #include "Engine/Core/CallbackData.hpp"
// #include "Engine/Core/StringUtils.hpp"
// #include "Engine/Core/EngineCommon.hpp"
// #include "Engine/Core/LogSubsystem.hpp"
//
// //----------------------------------------------------------------------------------------------------
// // Constructor
// //----------------------------------------------------------------------------------------------------
// DebugRenderAPI::DebugRenderAPI(RenderCommandQueue* commandQueue,
//                                ScriptSubsystem* scriptSubsystem,
//                                DebugRenderStateBuffer* stateBuffer,
//                                CallbackQueue* callbackQueue)
// 	: m_commandQueue(commandQueue)
// 	, m_scriptSubsystem(scriptSubsystem)
// 	, m_stateBuffer(stateBuffer)
// 	, m_callbackQueue(callbackQueue)
// {
// 	GUARANTEE_OR_DIE(m_commandQueue != nullptr, "DebugRenderAPI: commandQueue cannot be null");
// 	GUARANTEE_OR_DIE(m_scriptSubsystem != nullptr, "DebugRenderAPI: scriptSubsystem cannot be null");
// 	GUARANTEE_OR_DIE(m_stateBuffer != nullptr, "DebugRenderAPI: stateBuffer cannot be null");
// 	GUARANTEE_OR_DIE(m_callbackQueue != nullptr, "DebugRenderAPI: callbackQueue cannot be null");
// }
//
// //----------------------------------------------------------------------------------------------------
// // AddLine
// //----------------------------------------------------------------------------------------------------
// uint32_t DebugRenderAPI::AddLine(Vec3 const& start, Vec3 const& end,
//                                   Rgba8 const& startColor, Rgba8 const& endColor,
//                                   float radius, float duration)
// {
// 	uint32_t primitiveId = GenerateUniquePrimitiveId();
//
// 	DebugLineData lineData;
// 	lineData.start = start;
// 	lineData.end = end;
// 	lineData.startColor = startColor;
// 	lineData.endColor = endColor;
// 	lineData.radius = radius;
// 	lineData.duration = duration;
//
// 	RenderCommand cmd(RenderCommandType::DEBUG_ADD_LINE, primitiveId, lineData);
// 	m_commandQueue->Submit(cmd);
//
// 	return primitiveId;
// }
//
// //----------------------------------------------------------------------------------------------------
// // AddPoint
// //----------------------------------------------------------------------------------------------------
// uint32_t DebugRenderAPI::AddPoint(Vec3 const& position, Rgba8 const& color,
//                                    float radius, float duration, bool isBillboard)
// {
// 	uint32_t primitiveId = GenerateUniquePrimitiveId();
//
// 	DebugPointData pointData;
// 	pointData.position = position;
// 	pointData.color = color;
// 	pointData.radius = radius;
// 	pointData.duration = duration;
// 	pointData.isBillboard = isBillboard;
//
// 	RenderCommand cmd(RenderCommandType::DEBUG_ADD_POINT, primitiveId, pointData);
// 	m_commandQueue->Submit(cmd);
//
// 	return primitiveId;
// }
//
// //----------------------------------------------------------------------------------------------------
// // AddSphere
// //----------------------------------------------------------------------------------------------------
// uint32_t DebugRenderAPI::AddSphere(Vec3 const& center, float radius,
//                                     Rgba8 const& color, float duration, bool isSolid)
// {
// 	uint32_t primitiveId = GenerateUniquePrimitiveId();
//
// 	DebugSphereData sphereData;
// 	sphereData.center = center;
// 	sphereData.radius = radius;
// 	sphereData.color = color;
// 	sphereData.duration = duration;
// 	sphereData.isSolid = isSolid;
//
// 	RenderCommand cmd(RenderCommandType::DEBUG_ADD_SPHERE, primitiveId, sphereData);
// 	m_commandQueue->Submit(cmd);
//
// 	return primitiveId;
// }
//
// //----------------------------------------------------------------------------------------------------
// // AddAABB
// //----------------------------------------------------------------------------------------------------
// uint32_t DebugRenderAPI::AddAABB(Vec3 const& minBounds, Vec3 const& maxBounds,
//                                   Rgba8 const& color, float duration)
// {
// 	uint32_t primitiveId = GenerateUniquePrimitiveId();
//
// 	DebugAABBData aabbData;
// 	aabbData.minBounds = minBounds;
// 	aabbData.maxBounds = maxBounds;
// 	aabbData.color = color;
// 	aabbData.duration = duration;
//
// 	RenderCommand cmd(RenderCommandType::DEBUG_ADD_AABB, primitiveId, aabbData);
// 	m_commandQueue->Submit(cmd);
//
// 	return primitiveId;
// }
//
// //----------------------------------------------------------------------------------------------------
// // AddBasis
// //----------------------------------------------------------------------------------------------------
// uint32_t DebugRenderAPI::AddBasis(Vec3 const& position,
//                                    Vec3 const& iBasis, Vec3 const& jBasis, Vec3 const& kBasis,
//                                    float duration, float axisLength)
// {
// 	uint32_t primitiveId = GenerateUniquePrimitiveId();
//
// 	DebugBasisData basisData;
// 	basisData.position = position;
// 	basisData.iBasis = iBasis;
// 	basisData.jBasis = jBasis;
// 	basisData.kBasis = kBasis;
// 	basisData.duration = duration;
// 	basisData.axisLength = axisLength;
//
// 	RenderCommand cmd(RenderCommandType::DEBUG_ADD_BASIS, primitiveId, basisData);
// 	m_commandQueue->Submit(cmd);
//
// 	return primitiveId;
// }
//
// //----------------------------------------------------------------------------------------------------
// // AddWorldText
// //----------------------------------------------------------------------------------------------------
// uint64_t DebugRenderAPI::AddWorldText(std::string const& text, Mat44 const& transform,
//                                        float fontSize, Vec2 const& alignment,
//                                        float duration, Rgba8 const& color,
//                                        eDebugRenderMode mode)
// {
// 	uint32_t primitiveId = GenerateUniquePrimitiveId();
//
// 	DebugWorldTextData textData;
// 	textData.text = text;
// 	textData.transform = transform;
// 	textData.fontSize = fontSize;
// 	textData.alignment = alignment;
// 	textData.duration = duration;
// 	textData.color = color;
// 	textData.mode = mode;
//
// 	RenderCommand cmd(RenderCommandType::DEBUG_ADD_WORLD_TEXT, primitiveId, textData);
// 	m_commandQueue->Submit(cmd);
//
// 	return primitiveId;
// }
//
// //----------------------------------------------------------------------------------------------------
// // AddScreenText
// //----------------------------------------------------------------------------------------------------
// uint64_t DebugRenderAPI::AddScreenText(std::string const& text, Vec2 const& position,
//                                         float fontSize, Vec2 const& alignment,
//                                         float duration, Rgba8 const& color)
// {
// 	uint32_t primitiveId = GenerateUniquePrimitiveId();
//
// 	DebugScreenTextData textData;
// 	textData.text = text;
// 	textData.position = position;
// 	textData.fontSize = fontSize;
// 	textData.alignment = alignment;
// 	textData.duration = duration;
// 	textData.color = color;
//
// 	RenderCommand cmd(RenderCommandType::DEBUG_ADD_SCREEN_TEXT, primitiveId, textData);
// 	m_commandQueue->Submit(cmd);
//
// 	return primitiveId;
// }
//
// //----------------------------------------------------------------------------------------------------
// // UpdateColor
// //----------------------------------------------------------------------------------------------------
// void DebugRenderAPI::UpdateColor(uint32_t primitiveId, Rgba8 const& newColor)
// {
// 	DebugColorUpdateData colorData;
// 	colorData.newColor = newColor;
//
// 	RenderCommand cmd(RenderCommandType::DEBUG_UPDATE_COLOR, primitiveId, colorData);
// 	m_commandQueue->Submit(cmd);
// }
//
// //----------------------------------------------------------------------------------------------------
// // Remove
// //----------------------------------------------------------------------------------------------------
// void DebugRenderAPI::Remove(uint32_t primitiveId)
// {
// 	RenderCommand cmd(RenderCommandType::DEBUG_REMOVE, primitiveId, std::monostate{});
// 	m_commandQueue->Submit(cmd);
// }
//
// //----------------------------------------------------------------------------------------------------
// // ClearAll
// //----------------------------------------------------------------------------------------------------
// void DebugRenderAPI::ClearAll()
// {
// 	RenderCommand cmd(RenderCommandType::DEBUG_CLEAR_ALL, 0, std::monostate{});
// 	m_commandQueue->Submit(cmd);
// }
//
// //----------------------------------------------------------------------------------------------------
// // GenerateUniquePrimitiveId
// //----------------------------------------------------------------------------------------------------
// uint32_t DebugRenderAPI::GenerateUniquePrimitiveId()
// {
// 	// Simple atomic increment (Phase 4: No collision detection needed yet)
// 	// Future enhancement: Check m_stateBuffer for collisions if wrapping occurs
// 	return m_nextPrimitiveId++;
// }
//
// //----------------------------------------------------------------------------------------------------
// // ExecutePendingCallbacks
// //----------------------------------------------------------------------------------------------------
// void DebugRenderAPI::ExecutePendingCallbacks(CallbackQueue* callbackQueue)
// {
// 	GUARANTEE_OR_DIE(callbackQueue != nullptr, "DebugRenderAPI::ExecutePendingCallbacks - CallbackQueue is nullptr!");
//
// 	// Iterate through all pending callbacks and enqueue ready ones
// 	for (auto it = m_pendingCallbacks.begin(); it != m_pendingCallbacks.end(); ++it)
// 	{
// 		CallbackID       callbackId = it->first;
// 		PendingCallback& pending    = it->second;
//
// 		// Only process callbacks that are ready (C++ processing complete)
// 		if (pending.ready)
// 		{
// 			// Create CallbackData for JavaScript execution
// 			CallbackData data;
// 			data.callbackId   = callbackId;
// 			data.resultId     = pending.resultId;
// 			data.errorMessage = "";
// 			data.type         = CallbackType::RESOURCE_LOADED;  // Generic completion type
//
// 			// Enqueue to CallbackQueue for JavaScript worker thread execution
// 			bool enqueued = callbackQueue->Enqueue(data);
//
// 			if (!enqueued)
// 			{
// 				DAEMON_LOG(LogDebugRender, eLogVerbosity::Warning,
// 				           Stringf("DebugRenderAPI::ExecutePendingCallbacks - CallbackQueue full! Dropped callback %llu for primitive %llu",
// 				               callbackId, pending.resultId));
// 			}
// 		}
// 	}
//
// 	// Erase all ready callbacks (cleanup after enqueuing)
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
// // NotifyCallbackReady
// //----------------------------------------------------------------------------------------------------
// void DebugRenderAPI::NotifyCallbackReady(CallbackID callbackId, uint64_t resultId)
// {
// 	// Find the pending callback
// 	auto it = m_pendingCallbacks.find(callbackId);
// 	if (it == m_pendingCallbacks.end())
// 	{
// 		DAEMON_LOG(LogDebugRender, eLogVerbosity::Warning,
// 		           Stringf("DebugRenderAPI::NotifyCallbackReady - CallbackID %llu not found in pending callbacks!", callbackId));
// 		return;
// 	}
//
// 	// Mark callback as ready with resultId
// 	PendingCallback& pending = it->second;
// 	pending.resultId = resultId;
// 	pending.ready    = true;
//
// 	DAEMON_LOG(LogDebugRender, eLogVerbosity::Verbose,
// 	           Stringf("DebugRenderAPI::NotifyCallbackReady - Callback %llu ready with result %llu", callbackId, resultId));
// }
//
// //----------------------------------------------------------------------------------------------------
// // GenerateCallbackID
// //----------------------------------------------------------------------------------------------------
// CallbackID DebugRenderAPI::GenerateCallbackID()
// {
// 	// Simple atomic increment (0 reserved for invalid)
// 	return m_nextCallbackId++;
// }
