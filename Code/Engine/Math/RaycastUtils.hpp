//----------------------------------------------------------------------------------------------------
// RaycastUtils.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include "Engine/Math/Vec2.hpp"

//----------------------------------------------------------------------------------------------------
struct Ray2
{
    Ray2(Vec2 const& origin, Vec2 const& direction, float maxDist);

    Vec2  m_origin;
    Vec2  m_direction;
    float m_maxDist;
};

//----------------------------------------------------------------------------------------------------
struct RaycastResult2D
{
    // Basic raycast result information (required)
    bool  m_didImpact  = false;
    float m_impactDist = 0.f;
    Vec2  m_impactPos;
    Vec2  m_impactNormal;

    // Original raycast information (optional)
    Vec2  m_rayFwdNormal;
    Vec2  m_rayStartPos;
    float m_rayMaxLength = 1.f;
};

RaycastResult2D RaycastVsDisc2D(Vec2 const& startPos, Vec2 const& fwdNormal, float maxDist, Vec2 const& discCenter, float discRadius);
RaycastResult2D RaycastVsLineSegment2D();
RaycastResult2D RaycastVsAABB2D();
