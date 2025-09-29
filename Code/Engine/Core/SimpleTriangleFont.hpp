//----------------------------------------------------------------------------------------------------
// SimpleTriangleFont.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once

#include "Engine/Core/StringUtils.hpp"
#include "Engine/Renderer/VertexUtils.hpp"

//----------------------------------------------------------------------------------------------------
void AddVertsForTextTriangles2D(VertexList_PCU& verts,
                                String const&   text,
                                Vec2 const&     startMins,
                                float           cellHeight,
                                Rgba8 const&    color,
                                float           cellAspect      = 0.56f,
                                bool            isFlipped       = false,
                                float           spacingFraction = 0.2f);

//----------------------------------------------------------------------------------------------------
float GetSimpleTriangleStringWidth(String const& text,
                                   float         cellHeight,
                                   float         cellAspect      = 0.56f,
                                   float         spacingFraction = 0.2f);
