//-----------------------------------------------------------------------------------------------
// Texture.cpp
//

//-----------------------------------------------------------------------------------------------
#include "Engine/Renderer/BitmapFont.hpp"

//----------------------------------------------------------------------------------------------------
Texture& BitmapFont::GetTexture()
{
}

//----------------------------------------------------------------------------------------------------
void BitmapFont::AddVertsForText2D(VertexList& vertexArray, Vec2 const& textMins, float cellHeight, String const& text, Rgba8 const& tint, float cellAspectScale)
{
}

//----------------------------------------------------------------------------------------------------
float BitmapFont::GetTextWidth(float cellHeight, String const& text, float cellAspectScale)
{
}

//----------------------------------------------------------------------------------------------------
BitmapFont::BitmapFont(char const* fontFilePathNameWithNoExtension, Texture& fontTexture)
{
}

//----------------------------------------------------------------------------------------------------
float BitmapFont::GetGlyphAspect(int glyphUnicode) const
{
}
