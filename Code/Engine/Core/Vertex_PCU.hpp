//----------------------------------------------------------------------------------------------------
// Vertex_PCU.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"

//-----------------------------------------------------------------------------------------------
struct Vertex_PCU
{
	Vec3  m_position;
	Rgba8 m_color;
	Vec2  m_uvTexCoords;

	Vertex_PCU();
	Vertex_PCU(Vec3 const& position, Rgba8 const& color, Vec2 const& uvTexCoords = Vec2(0.f, 0.f));
	//TODO: add a bunch of explicit constructor
};
