//----------------------------------------------------------------------------------------------------
// Vertex_Font.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"

//----------------------------------------------------------------------------------------------------
struct Vertex_Font
{
    Vec3  m_position       = Vec3::ZERO;     // 12 bytes
    Rgba8 m_color          = Rgba8::WHITE;   // 4 bytes
    Vec2  m_uvTexCoords    = Vec2::ZERO;     // 8 bytes
    Vec2  m_glyphPosition  = Vec2::ZERO;     // 8 bytes — [0,1] within glyph quad
    Vec2  m_textPosition   = Vec2::ZERO;     // 8 bytes — normalized within text block
    int   m_characterIndex = 0;              // 4 bytes — 0-based char index in string
    float m_effectParam    = 0.f;            // 4 bytes — generic effect parameter
    // Total: 48 bytes per vertex

    Vertex_Font() = default;

    Vertex_Font(Vec3 const& position, Rgba8 const& color, Vec2 const& uvTexCoords,
                Vec2 const& glyphPosition, Vec2 const& textPosition,
                int characterIndex, float effectParam = 0.f);
};
