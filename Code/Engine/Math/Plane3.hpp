//----------------------------------------------------------------------------------------------------
// Disc2.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once

#include "Engine/Math/Vec3.hpp"

//----------------------------------------------------------------------------------------------------
struct Plane3
{
    Plane3() = default;
    Plane3(Vec3 const& normal, float distanceFromOrigin);

    /// Get the point on plane nearest to origin(0,0,0)
    Vec3  GetOriginPoint() const;
    float GetAltitudeOfPoint(Vec3 const& point) const;
    Vec3  GetNearestPoint(Vec3 const& point) const;
    void  Translate(Vec3 const& translationToApply);

    Vec3  m_normal             = Vec3::Z_BASIS;
    float m_distanceFromOrigin = 0.f;
};
