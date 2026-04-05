//----------------------------------------------------------------------------------------------------
// Vertex_Font.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Renderer/Vertex_Font.hpp"

//----------------------------------------------------------------------------------------------------
Vertex_Font::Vertex_Font(Vec3 const& position, Rgba8 const& color, Vec2 const& uvTexCoords,
                         Vec2 const& glyphPosition, Vec2 const& textPosition,
                         int const characterIndex, float const effectParam)
    : m_position(position)
    , m_color(color)
    , m_uvTexCoords(uvTexCoords)
    , m_glyphPosition(glyphPosition)
    , m_textPosition(textPosition)
    , m_characterIndex(characterIndex)
    , m_effectParam(effectParam)
{
}
