//----------------------------------------------------------------------------------------------------
// Plane2.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Math/Plane2.hpp"
//----------------------------------------------------------------------------------------------------
#include "Engine/Math/MathUtils.hpp"

//----------------------------------------------------------------------------------------------------
Plane2::Plane2()
{
}

Plane2::Plane2(Vec2 const& normal, Vec2 const& refPosOnPlane)
    : m_normal(normal)
      , m_distanceFromOrigin(DotProduct2D(normal, refPosOnPlane))
{
}

Vec2 Plane2::GetOriginPoint() const
{
    return m_normal * m_distanceFromOrigin;
}

float Plane2::GetAltitudeOfPoint(Vec2 const& refPoint) const
{
    return DotProduct2D(m_normal, refPoint) - m_distanceFromOrigin;
}

Vec2 Plane2::GetNearestPoint(Vec2 const& refPoint) const
{
    return refPoint - m_normal * GetAltitudeOfPoint(refPoint);
}
