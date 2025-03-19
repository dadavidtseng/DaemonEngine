//----------------------------------------------------------------------------------------------------
// Cylinder.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Math/Cylinder3.hpp"

#include "FloatRange.hpp"
#include "Engine/Math/MathUtils.hpp"

//----------------------------------------------------------------------------------------------------
Cylinder3::Cylinder3(Vec3 const& startPosition,
                     Vec3 const& endPosition,
                     float const radius)
    : m_startPosition(startPosition),
      m_endPosition(endPosition),
      m_radius(radius)
{
}

//----------------------------------------------------------------------------------------------------
Vec3 Cylinder3::GetCenterPosition() const
{
    return (m_startPosition + m_endPosition) / 2.f;
}

//----------------------------------------------------------------------------------------------------
Vec2 Cylinder3::GetCenterPositionXY() const
{
    Vec3 const centerPosition = GetCenterPosition();

    return Vec2(centerPosition.x, centerPosition.y);
}

//----------------------------------------------------------------------------------------------------
FloatRange Cylinder3::GetFloatRange() const
{
    return FloatRange(m_startPosition.z, m_endPosition.z);
}

//----------------------------------------------------------------------------------------------------
bool Cylinder3::IsPointInside(Vec3 const& point) const
{
    Vec3 const cylinderCenterPosition   = (m_startPosition + m_endPosition) * 0.5f;
    Vec2 const cylinderCenterPositionXY = Vec2(cylinderCenterPosition.x, cylinderCenterPosition.y);
    Vec2 const pointXY                  = Vec2(point.x, point.y);

    return
        IsPointInsideDisc2D(pointXY, cylinderCenterPositionXY, m_radius) &&
        point.z < m_endPosition.z &&
        point.z > m_startPosition.z;
}

//----------------------------------------------------------------------------------------------------
Vec3 Cylinder3::GetNearestPoint(Vec3 const& point) const
{
    Vec3 const cylinderCenterPosition   = (m_startPosition + m_endPosition) * 0.5f;
    Vec2 const cylinderCenterPositionXY = Vec2(cylinderCenterPosition.x, cylinderCenterPosition.y);
    Vec2 const pointXY                  = Vec2(point.x, point.y);

    if (IsPointInside(point) == true)
    {
        return point;
    }

    Vec2 const nearestPointOnDisc = GetNearestPointOnDisc2D(pointXY, cylinderCenterPositionXY, m_radius);
    Vec3       nearestPoint       = Vec3(nearestPointOnDisc.x, nearestPointOnDisc.y, 0.f);

    nearestPoint.z = GetClamped(point.z, m_startPosition.z, m_endPosition.z);

    return nearestPoint;
}
