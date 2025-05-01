//----------------------------------------------------------------------------------------------------
// Plane2.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include "Engine/Math/Vec2.hpp"

//----------------------------------------------------------------------------------------------------
struct Plane2
{
    Plane2();
    Plane2(Vec2 const& normal, Vec2 const& refPosOnPlane);
    /// Get the point on plane nearest to origin(0,0,0)
    Vec2 GetOriginPoint() const;
    ///
    float GetAltitudeOfPoint(Vec2 const& refPoint) const;
    ///
    Vec2  GetNearestPoint(Vec2 const& refPoint) const;
    Vec2  m_normal             = Vec2::ZERO;
    float m_distanceFromOrigin = 0.f;
};
