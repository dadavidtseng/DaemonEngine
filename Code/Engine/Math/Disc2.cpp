//----------------------------------------------------------------------------------------------------
// Disc2.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Math/Disc2.hpp"
//----------------------------------------------------------------------------------------------------
#include "MathUtils.hpp"

//----------------------------------------------------------------------------------------------------
Disc2::Disc2(Vec2 const& position, float const radius)
    : m_position(position),
      m_radius(radius)
{
}

//----------------------------------------------------------------------------------------------------
bool Disc2::IsPointInside(Vec2 const& point) const
{
    float const distanceSquared = GetDistanceSquared2D(point, m_position);
    float const radiusSquared = m_radius * m_radius;

    return distanceSquared <= radiusSquared;
}

//----------------------------------------------------------------------------------------------------
Vec2 Disc2::GetNearestPoint(Vec2 const& point) const
{
    // 1. If the point is inside the disc, return the point itself.
    if (IsPointInside(point) == true)
    {
        return point;
    }

    // 2. Calculate the nearest point on the disc.
    Vec2 const centerToPoint       = point - m_position;
    Vec2 const centerToPointNormal = centerToPoint.GetNormalized();

    return m_position + centerToPointNormal * m_radius;
}

//----------------------------------------------------------------------------------------------------
Vec2 Disc2::GetPosition() const
{
    return m_position;
}

//----------------------------------------------------------------------------------------------------
float Disc2::GetRadius() const
{
    return m_radius;
}

//----------------------------------------------------------------------------------------------------
void Disc2::SetPosition(Vec2 const& newCenter)
{
    m_position = newCenter;
}

//----------------------------------------------------------------------------------------------------
void Disc2::SetRadius(const float newRadius)
{
    m_radius = newRadius;
}
