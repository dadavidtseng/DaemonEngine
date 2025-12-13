//----------------------------------------------------------------------------------------------------
// DebugRenderStateBuffer.hpp
//----------------------------------------------------------------------------------------------------
// Phase 4: Async Debug Render System
// Double-buffered state storage for debug primitives to enable thread-safe async command submission
//
// Pattern matches EntityStateBuffer and CameraStateBuffer:
// - Worker thread writes to back buffer
// - Main thread reads from front buffer
// - SwapBuffers() copies back → front (with dirty tracking optimization)
//
// Architecture:
// - DebugRenderAPI submits DEBUG_* commands to RenderCommandQueue
// - App::ProcessRenderCommands() processes commands → updates back buffer
// - App::Render() calls RenderDebugPrimitives() → reads front buffer
//----------------------------------------------------------------------------------------------------
#pragma once
#include "Engine/Core/StateBuffer.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Mat44.hpp"
#include "Engine/Core/Rgba8.hpp"
#include <unordered_map>
#include <cstdint>
#include <string>

//----------------------------------------------------------------------------------------------------
// Debug Primitive Type
//----------------------------------------------------------------------------------------------------
enum class DebugPrimitiveType : uint8_t
{
	LINE,          // 3D line segment with thickness
	POINT,         // 3D point/billboard
	SPHERE,        // 3D sphere (wireframe or solid)
	AABB,          // 3D axis-aligned bounding box
	BASIS,         // 3D coordinate system visualization (XYZ arrows)
	TEXT_2D,       // 2D screen-space text
	TEXT_3D        // 3D world-space text
};

//----------------------------------------------------------------------------------------------------
// Debug Primitive State
//----------------------------------------------------------------------------------------------------
struct DebugPrimitive
{
	uint64_t primitiveId = 0;               // Unique identifier for runtime modification (EntityID)
	DebugPrimitiveType type = DebugPrimitiveType::LINE;

	// Geometry data (interpretation depends on type)
	Vec3 startPos = Vec3::ZERO;             // LINE: start, POINT: position, SPHERE: center, TEXT_3D: position
	Vec3 endPos = Vec3::ZERO;               // LINE: end, AABB: max corner (min is startPos)
	Vec3 basisI = Vec3::ZERO;               // BASIS: I-axis direction
	Vec3 basisJ = Vec3::ZERO;               // BASIS: J-axis direction
	Vec3 basisK = Vec3::ZERO;               // BASIS: K-axis direction

	// Text-specific properties (TEXT_2D/TEXT_3D only)
	std::string text = "";                  // Text content for TEXT_2D/TEXT_3D
	float fontSize = 1.0f;                  // Text height/size
	Vec2 textAlignment = Vec2(0.5f, 0.5f);  // Text alignment (0-1 range, 0.5=center)
	Mat44 textTransform = Mat44();          // World-space transform for TEXT_3D

	// Visual properties
	Rgba8 startColor = Rgba8::WHITE;        // LINE: start color, others: primary color
	Rgba8 endColor = Rgba8::WHITE;          // LINE: end color
	float radius = 0.1f;                    // LINE/POINT: thickness, SPHERE: radius
	float duration = 0.0f;                  // Duration in seconds (0 = permanent, -1 = single frame)
	float timeRemaining = 0.0f;             // Time left before expiry

	// State
	bool isActive = true;                   // false = removed from rendering
	bool isBillboard = false;               // POINT: face camera
	bool isSolid = false;                   // SPHERE: solid vs wireframe
};

//----------------------------------------------------------------------------------------------------
// Type Aliases
//----------------------------------------------------------------------------------------------------
using DebugPrimitiveMap = std::unordered_map<uint64_t, DebugPrimitive>;
using DebugRenderStateBuffer = StateBuffer<DebugPrimitiveMap>;

//----------------------------------------------------------------------------------------------------
// USAGE EXAMPLE:
//----------------------------------------------------------------------------------------------------
// // In App.cpp:
// m_debugRenderStateBuffer = new DebugRenderStateBuffer();
// m_debugRenderStateBuffer->EnableDirtyTracking(true);  // O(d) swap optimization
//
// // In App::ProcessRenderCommands():
// case RenderCommandType::DEBUG_ADD_LINE:
//     DebugLineData const& lineData = std::get<DebugLineData>(cmd.data);
//     DebugPrimitive primitive;
//     primitive.primitiveId = cmd.entityId;
//     primitive.type = DebugPrimitiveType::LINE;
//     primitive.startPos = lineData.start;
//     primitive.endPos = lineData.end;
//     primitive.startColor = lineData.color;
//     primitive.endColor = lineData.color;
//     primitive.radius = lineData.radius;
//     primitive.duration = lineData.duration;
//     primitive.timeRemaining = lineData.duration;
//
//     auto* backBuffer = m_debugRenderStateBuffer->GetBackBuffer();
//     (*backBuffer)[cmd.entityId] = primitive;
//     m_debugRenderStateBuffer->MarkDirty(cmd.entityId);
//     break;
//
// case RenderCommandType::DEBUG_ADD_WORLD_TEXT:
//     DebugWorldTextData const& textData = std::get<DebugWorldTextData>(cmd.data);
//     DebugPrimitive textPrimitive;
//     textPrimitive.primitiveId = cmd.entityId;
//     textPrimitive.type = DebugPrimitiveType::TEXT_3D;
//     textPrimitive.text = textData.text;
//     textPrimitive.fontSize = textData.fontSize;
//     textPrimitive.textAlignment = textData.alignment;
//     textPrimitive.textTransform = textData.transform;
//     textPrimitive.startColor = textData.color;
//     textPrimitive.duration = textData.duration;
//     textPrimitive.timeRemaining = textData.duration;
//
//     auto* backBuffer = m_debugRenderStateBuffer->GetBackBuffer();
//     (*backBuffer)[cmd.entityId] = textPrimitive;
//     m_debugRenderStateBuffer->MarkDirty(cmd.entityId);
//     break;
//
// // After processing all commands:
// m_debugRenderStateBuffer->SwapBuffers();
//
// // In App::RenderDebugPrimitives():
// DebugPrimitiveMap const* frontBuffer = m_debugRenderStateBuffer->GetFrontBuffer();
// for (auto const& [primitiveId, primitive] : *frontBuffer) {
//     if (!primitive.isActive) continue;
//
//     switch (primitive.type) {
//         case DebugPrimitiveType::LINE:
//             DebugAddWorldLine(primitive.startPos, primitive.endPos,
//                              primitive.radius, 0.0f,  // duration=0 (render once)
//                              primitive.startColor, primitive.endColor);
//             break;
//
//         case DebugPrimitiveType::TEXT_3D:
//             DebugAddWorldText(primitive.text.c_str(), primitive.textTransform,
//                              primitive.fontSize, primitive.textAlignment,
//                              0.0f, primitive.startColor);  // duration=0 (render once)
//             break;
//
//         case DebugPrimitiveType::TEXT_2D:
//             DebugAddScreenText(primitive.text.c_str(),
//                               Vec2(primitive.startPos.x, primitive.startPos.y),
//                               primitive.fontSize, primitive.textAlignment,
//                               0.0f, primitive.startColor);  // duration=0 (render once)
//             break;
//     }
// }
//----------------------------------------------------------------------------------------------------
