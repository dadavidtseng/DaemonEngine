//----------------------------------------------------------------------------------------------------
// BitmapFont.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include "SpriteSheet.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/VertexUtils.hpp"

class Texture;
//----------------------------------------------------------------------------------------------------
class BitmapFont
{
    friend class Renderer; // Only the Renderer can create new BitmapFont objects!

public:
    Texture& GetTexture();
    void     AddVertsForText2D(VertexList& vertexArray, Vec2 const& textMins, float cellHeight, String const& text, Rgba8 const& tint = Rgba8::WHITE, float cellAspectScale = 1.f);
    float    GetTextWidth(float cellHeight, String const& text, float cellAspectScale = 1.f);

private:
    BitmapFont(char const* fontFilePathNameWithNoExtension, Texture& fontTexture);

protected:
    float GetGlyphAspect(int glyphUnicode) const; // For now this will always return m_fontDefaultAspect

    String      m_fontFilePathNameWithNoExtension;
    SpriteSheet m_fontGlyphsSpriteSheet;
    float       m_fontDefaultAspect = 1.0f; // For basic (tier 1) fonts, set this to the aspect of the sprite sheet texture
};
