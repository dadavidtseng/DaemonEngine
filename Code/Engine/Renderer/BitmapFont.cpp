//-----------------------------------------------------------------------------------------------
// Texture.cpp
//

//-----------------------------------------------------------------------------------------------
#include "Engine/Renderer/BitmapFont.hpp"

#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Math/AABB2.hpp"

//----------------------------------------------------------------------------------------------------
Texture const& BitmapFont::GetTexture() const
{
    return m_fontGlyphsSpriteSheet.GetTexture();
}

//----------------------------------------------------------------------------------------------------
void BitmapFont::AddVertsForText2D(std::vector<Vertex_PCU>& vertexArray, Vec2 const& textMins, float cellHeight, std::string const& text, Rgba8 const& tint, float cellAspectScale) const
{
    Vec2 currentPosition = textMins; // Create a local copy to modify

    for (char const& c : text)
    {
        int const   glyphIndex  = static_cast<unsigned char>(c);
        AABB2       uvs         = m_fontGlyphsSpriteSheet.GetSpriteUVs(glyphIndex);
        float const glyphAspect = GetGlyphAspect(glyphIndex);
        Vec2 const  glyphSize(cellHeight * glyphAspect * cellAspectScale, cellHeight);

        AddVertsForAABB2D(vertexArray, AABB2(currentPosition, currentPosition + glyphSize), tint, uvs.m_mins, uvs.m_maxs);

        currentPosition.x += glyphSize.x;
    }
}

//----------------------------------------------------------------------------------------------------
float BitmapFont::GetTextWidth(float const cellHeight, String const& text, float const cellAspectScale) const
{
    float totalWidth = 0.f;

    for (char const& c : text)
    {
        int const   glyphIndex  = static_cast<unsigned char>(c);
        float const glyphAspect = GetGlyphAspect(glyphIndex);
        totalWidth += (cellHeight * glyphAspect * cellAspectScale);
    }

    return totalWidth;
}

//----------------------------------------------------------------------------------------------------
BitmapFont::BitmapFont(char const* fontFilePathNameWithNoExtension, Texture const& fontTexture, IntVec2 const& spriteCoords)
    : m_fontFilePathNameWithNoExtension(fontFilePathNameWithNoExtension)
    , m_fontGlyphsSpriteSheet(fontTexture, spriteCoords)
{
}

//----------------------------------------------------------------------------------------------------
float BitmapFont::GetGlyphAspect(int const glyphUnicode) const
{
    UNUSED(glyphUnicode)
    return m_fontDefaultAspect;
}
