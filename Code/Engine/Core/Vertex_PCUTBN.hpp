//----------------------------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"

//----------------------------------------------------------------------------------------------------
struct Vertex_PCUTBN
{
    Vec3  m_position    = Vec3::ZERO;
    Rgba8 m_color       = Rgba8::MAGENTA;
    Vec2  m_uvTexCoords = Vec2::ZERO;
    Vec3  m_tangent     = Vec3::ZERO;
    Vec3  m_bitangent   = Vec3::ZERO;
    Vec3  m_normal      = Vec3::ZERO;

    Vertex_PCUTBN() = default;
    Vertex_PCUTBN(Vec3 const& position, Rgba8 const& color, Vec2 const& uvTexCoords, Vec3 const& tangent, Vec3 const& bitangent, Vec3 const& normal);
    Vertex_PCUTBN(float px, float py, float pz, unsigned char r, unsigned char g, unsigned char b, unsigned char a, float u, float v, float tx, float ty, float tz, float bx, float by, float bz, float nx, float ny, float nz);
};
