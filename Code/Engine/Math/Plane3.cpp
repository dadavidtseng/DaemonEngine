//----------------------------------------------------------------------------------------------------
// Plane3.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Math/Plane3.hpp"

#include "MathUtils.hpp"

//----------------------------------------------------------------------------------------------------
Plane3::Plane3(Vec3 const& normal,
               float const distanceFromOrigin)
    : m_normal(normal),
      m_distanceFromOrigin(distanceFromOrigin)
{
}

Vec3 Plane3::GetOriginPoint() const
{
    return m_normal * m_distanceFromOrigin;
}

float Plane3::GetAltitudeOfPoint(Vec3 const& refPoint) const
{
    return DotProduct3D(m_normal, refPoint) - m_distanceFromOrigin;
}

Vec3 Plane3::GetNearestPoint(Vec3 const& refPoint) const
{
    return refPoint - m_normal * GetAltitudeOfPoint(refPoint);
}

void Plane3::Translate(Vec3 const& translationToApply)
{
    // translate do not change the normal and all points on the plane translates
    Vec3 translatedOrigin = GetOriginPoint() + translationToApply;
    m_distanceFromOrigin  = DotProduct3D(translatedOrigin, m_normal);
}
