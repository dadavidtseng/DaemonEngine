//----------------------------------------------------------------------------------------------------
// Vertex_PCU.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"

//----------------------------------------------------------------------------------------------------
struct Vertex_PCU
{
    Vec3  m_position    = Vec3::ZERO;
    Rgba8 m_color       = Rgba8::WHITE;
    Vec2  m_uvTexCoords = Vec2::ZERO;

    Vertex_PCU() = default;
    Vertex_PCU(Vec3 const& position, Rgba8 const& color, Vec2 const& uvTexCoords = Vec2::ZERO);
    //TODO: add a bunch of explicit constructor
};
