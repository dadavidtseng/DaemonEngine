//----------------------------------------------------------------------------------------------------
// Capsule2.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Math/Capsule2.hpp"

#include "Engine/Math/MathUtils.hpp"

//----------------------------------------------------------------------------------------------------
Capsule2::Capsule2(Vec2 const& startPosition,
                   Vec2 const& endPosition,
                   float const radius)
    : m_startPosition(startPosition),
      m_endPosition(endPosition),
      m_radius(radius)
{
}

//----------------------------------------------------------------------------------------------------
bool Capsule2::IsPointInside(Vec2 const& point) const
{
    // Calculate the capsule's direction vector
    Vec2 const startToEnd = m_endPosition - m_startPosition;
    float const startToEndLengthSquared = startToEnd.GetLengthSquared();

    if (startToEndLengthSquared == 0.f)
    {
        float const distanceSquared = GetDistanceSquared2D(point, m_startPosition);
        float const radiusSquared   = m_radius * m_radius;

        return distanceSquared <= radiusSquared;
    }

    float const capsuleLength    = startToEnd.GetLength();
    Vec2 const  startToEndNormal = startToEnd.GetNormalized();

    // Calculate the projection of the point onto the capsule's direction
    float const projectionLength        = GetProjectedLength2D(point - m_startPosition, startToEndNormal);
    float const clampedProjectionLength = GetClamped(projectionLength, 0.f, capsuleLength);

    // Find the nearest point on the capsule segment
    Vec2 const nearestPointOnSegment = m_startPosition + startToEndNormal * clampedProjectionLength;

    // Calculate the distance from the point to the nearest point on the segment
    float const distanceToSegment = (point - nearestPointOnSegment).GetLength();

    // Check if the point is within the radius of the capsule
    return
        distanceToSegment <= m_radius ||
        (point - m_startPosition).GetLengthSquared() <= m_radius * m_radius ||
        (point - m_endPosition).GetLengthSquared() <= m_radius * m_radius;
}

//----------------------------------------------------------------------------------------------------
Vec2 Capsule2::GetNearestPoint(Vec2 const& point) const
{
    if (IsPointInside(point))
    {
        return point;
    }

    // Calculate the capsule's direction vector
    Vec2 const startToEnd = m_endPosition - m_startPosition;
    float const startToEndLengthSquared = startToEnd.GetLengthSquared();

    if (startToEndLengthSquared == 0.f)
    {
        Vec2 const startToPoint        = point - m_startPosition;
        Vec2 const centerToPointNormal = startToPoint.GetNormalized();

        return m_startPosition + centerToPointNormal * m_radius;
    }

    float const capsuleLength    = startToEnd.GetLength();
    Vec2 const  startToEndNormal = startToEnd.GetNormalized();

    // Calculate the projection of the point onto the capsule's direction
    float const projectionLength        = GetProjectedLength2D(point - m_startPosition, startToEndNormal);
    float const clampedProjectionLength = GetClamped(projectionLength, 0.f, capsuleLength);

    // Find the nearest point on the capsule segment
    Vec2 const nearestPointOnSegment              = m_startPosition + startToEndNormal * clampedProjectionLength;
    Vec2 const nearestPointOnSegmentToPointNormal = (point - nearestPointOnSegment).GetNormalized();

    return
        nearestPointOnSegment + nearestPointOnSegmentToPointNormal * m_radius;
}
