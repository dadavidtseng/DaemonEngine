//----------------------------------------------------------------------------------------------------
// BitmapFont.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"

//-Forward-Declaration--------------------------------------------------------------------------------
class Texture;

//----------------------------------------------------------------------------------------------------
enum eTextBoxMode
{
    SHRINK_TO_FIT,
    OVERRUN
};

//----------------------------------------------------------------------------------------------------
class BitmapFont
{
    friend class Renderer;  // Only the Renderer can create new BitmapFont objects!
    friend class RendererEx;

public:
    Texture const& GetTexture() const;
    void           AddVertsForText2D(VertexList_PCU& verts, String const& text, Vec2 const& textMins, float cellHeight, Rgba8 const& tint = Rgba8::WHITE, float cellAspectRatio = 1.f) const;
    void           AddVertsForTextInBox2D(VertexList_PCU& verts, String const& text, AABB2 const& box, float cellHeight, Rgba8 const& tint = Rgba8::WHITE, float cellAspectRatio = 1.f, Vec2 const& alignment = Vec2::ZERO, eTextBoxMode mode = SHRINK_TO_FIT, int maxGlyphsToDraw = INT_MAX) const;
    void           AddVertsForText3DAtOriginXForward(VertexList_PCU& verts, String const& text, float cellHeight, Rgba8 const& tint = Rgba8::WHITE, float cellAspectRatio = 1.f, Vec2 const& alignment = Vec2(0.5f, 0.5f), int maxGlyphsToDraw = INT_MAX) const;

    float GetTextWidth(float cellHeight, String const& text, float cellAspectRatio = 1.f) const;

private:
    BitmapFont(char const* fontFilePathNameWithNoExtension, Texture const& fontTexture, IntVec2 const& spriteCoords);

protected:
    float GetGlyphAspect(int glyphUnicode) const;   // For now this will always return m_fontDefaultAspect

    String      m_fontFilePathNameWithNoExtension;
    SpriteSheet m_fontGlyphsSpriteSheet;
    float       m_fontDefaultAspect = 1.f;  // For basic (tier 1) fonts, set this to the aspect of the sprite sheet texture
};
