//----------------------------------------------------------------------------------------------------
// RaycastUtils.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include "Engine/Math/Vec2.hpp"

struct Ray2
{
    Vec2  m_origin;        // Ray's origin point
    Vec2  m_direction;     // Direction vector of the ray
    float m_maxDist;      // Maximum distance the ray will travel
    // TODO: move to .cpp
    // Constructor to initialize the ray's properties
    Ray2(const Vec2& origin, const Vec2& direction, const float maxDist)
        : m_origin(origin), m_direction(direction), m_maxDist(maxDist)
    {
    }
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

RaycastResult2D RaycastVsDisc2D(const Vec2& startPos, const Vec2& fwdNormal, float maxDist, const Vec2& discCenter, float discRadius);
