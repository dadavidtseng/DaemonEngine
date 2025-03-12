//----------------------------------------------------------------------------------------------------
// Cylinder.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Math/Cylinder3.hpp"

#include "Disc2.hpp"
#include "MathUtils.hpp"

//----------------------------------------------------------------------------------------------------
Cylinder3::Cylinder3(Vec3 const& startPosition, Vec3 const& endPosition, float const radius)
    : m_startPosition(startPosition),
      m_endPosition(endPosition),
      m_radius(radius)
{
}

bool Cylinder3::IsPointInside(Vec3 const& referencePoint) const
{
    return false;
}

Vec3 Cylinder3::GetNearestPoint(Vec3 const& referencePoint) const
{
    Vec3 nearestPoint;

    Vec3  cylinderCenterPosition   = (m_startPosition + m_endPosition) * 0.5f;
    Vec2  cylinderCenterPositionXY = Vec2(cylinderCenterPosition.x, cylinderCenterPosition.y);
    Disc2 disc                     = Disc2(cylinderCenterPositionXY, m_radius);
    Vec2  referencePointXY         = Vec2(referencePoint.x, referencePoint.y);

    if (disc.IsPointInside(referencePointXY) && referencePoint.z < m_endPosition.z && referencePoint.z > m_startPosition.z)
    {
        return referencePoint;
    }

    nearestPoint.x = disc.GetNearestPoint(referencePointXY).x;
    nearestPoint.y = disc.GetNearestPoint(referencePointXY).y;
    nearestPoint.z = GetClamped(referencePoint.z, m_startPosition.z, m_endPosition.z);


    return nearestPoint;
}
