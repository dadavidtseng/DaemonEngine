//----------------------------------------------------------------------------------------------------
// Sphere3.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Math/Sphere3.hpp"

#include "Engine/Math/MathUtils.hpp"

//----------------------------------------------------------------------------------------------------
Sphere3::Sphere3(Vec3 const& centerPosition,
                 float const radius)
    : m_centerPosition(centerPosition),
      m_radius(radius)
{
}

//----------------------------------------------------------------------------------------------------
bool Sphere3::IsPointInside(Vec3 const& point) const
{
    return GetDistanceSquared3D(point, m_centerPosition) < m_radius * m_radius;
}

//----------------------------------------------------------------------------------------------------
Vec3 Sphere3::GetNearestPoint(Vec3 const& point) const
{
    if (IsPointInside(point) == true)
    {
        return point;
    }

    Vec3 const centerToPoint = point - m_centerPosition;

    return m_centerPosition + centerToPoint.GetClamped(m_radius);
}
