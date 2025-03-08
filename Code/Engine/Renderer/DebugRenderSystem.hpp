//----------------------------------------------------------------------------------------------------
// DebugRenderSystem.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include <string>

#include "Engine/Core/EventSystem.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/Mat44.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"

//-Forward-Declaration--------------------------------------------------------------------------------
class Camera;
class Renderer;

//----------------------------------------------------------------------------------------------------
enum class DebugRenderMode
{
    ALWAYS,
    USE_DEPTH,
    X_RAY
};

//----------------------------------------------------------------------------------------------------
struct DebugRenderConfig
{
    Renderer* m_renderer = nullptr;
    String    m_fontName = "SquirrelFixedFont";
};

//----------------------------------------------------------------------------------------------------
// Setup
//
void DebugRenderSystemStartup(DebugRenderConfig const& config);
void DebugRenderSystemShutdown();

//----------------------------------------------------------------------------------------------------
// Control
//
void DebugRenderSetVisible();
void DebugRenderSetHidden();
void DebugRenderClear();

//----------------------------------------------------------------------------------------------------
// Output
//
void DebugRenderBeginFrame();
void DebugRenderWorld(Camera const& camera);
void DebugRenderScreen(Camera const& camera);
void DebugRenderEndFrame();

//----------------------------------------------------------------------------------------------------
// Geometry
//
void DebugAddWorldPoint(Vec3 const& pos, float radius, float duration, Rgba8 const& startColor = Rgba8::WHITE, Rgba8 const& endColor = Rgba8::WHITE, DebugRenderMode mode = DebugRenderMode::USE_DEPTH);
void DebugAddWorldLine(Vec3 const& startPosition, Vec3 const& endPosition, float radius, float duration, Rgba8 const& startColor = Rgba8::WHITE, Rgba8 const& endColor = Rgba8::WHITE, DebugRenderMode mode = DebugRenderMode::USE_DEPTH);
void DebugAddWorldWireCylinder(Vec3 const& base, Vec3 const& top, float radius, float duration, Rgba8 const& startColor = Rgba8::WHITE, Rgba8 const& endColor = Rgba8::WHITE, DebugRenderMode mode = DebugRenderMode::USE_DEPTH);
void DebugAddWorldWireSphere(Vec3 const& center, float radius, float duration, Rgba8 const& startColor = Rgba8::WHITE, Rgba8 const& endColor = Rgba8::WHITE, DebugRenderMode mode = DebugRenderMode::USE_DEPTH);
void DebugAddWorldArrow(Vec3 const& start, Vec3 const& end, float radius, float duration, Rgba8 const& startColor = Rgba8::WHITE, Rgba8 const& endColor = Rgba8::WHITE, DebugRenderMode mode = DebugRenderMode::USE_DEPTH);
void DebugAddWorldText(String const& text, Mat44 const& transform, float textHeight, Vec2 const& alignment, float duration, Rgba8 const& startColor = Rgba8::WHITE, Rgba8 const& endColor = Rgba8::WHITE, DebugRenderMode mode = DebugRenderMode::USE_DEPTH);
void DebugAddBillboardText(String const& text, Vec3 const& origin, float textHeight, Vec2 const& alignment, float duration, Rgba8 const& startColor = Rgba8::WHITE, Rgba8 const& endColor = Rgba8::WHITE, DebugRenderMode mode = DebugRenderMode::USE_DEPTH);
void DebugAddWorldBasis(Mat44 const& transform, float duration, DebugRenderMode mode = DebugRenderMode::USE_DEPTH);
void DebugAddScreenText(String const& text, Vec2 const& position, float size, Vec2 const& alignment, float duration, Rgba8 const& startColor = Rgba8::WHITE, Rgba8 const& endColor = Rgba8::WHITE, DebugRenderMode mode = DebugRenderMode::USE_DEPTH);
void DebugAddMessage(String const& text, float duration, Rgba8 const& startColor = Rgba8::WHITE, Rgba8 const& endColor = Rgba8::WHITE);

//----------------------------------------------------------------------------------------------------
// Console commands
//
bool OnDebugRenderClear(EventArgs& args);
bool OnDebugRenderToggle(EventArgs& args);
