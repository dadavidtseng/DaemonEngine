//----------------------------------------------------------------------------------------------------
// Triangle2.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Math/Triangle2.hpp"
//----------------------------------------------------------------------------------------------------
#include "Engine/Math/MathUtils.hpp"

//----------------------------------------------------------------------------------------------------
Triangle2::Triangle2(Vec2 const& ccw1, Vec2 const& ccw2, Vec2 const& ccw3)
    : m_positionCounterClockwise{ccw1, ccw2, ccw3}
{
}

//----------------------------------------------------------------------------------------------------
Triangle2::Triangle2(Vec2 const points[3])
    : m_positionCounterClockwise{points[0], points[1], points[2]}
{
}

//----------------------------------------------------------------------------------------------------
bool Triangle2::IsPointInside(Vec2 const& point) const
{
    // Implement the barycentric method to check if the point is inside the triangle
    Vec2 const v0 = m_positionCounterClockwise[1] - m_positionCounterClockwise[0];
    Vec2 const v1 = m_positionCounterClockwise[2] - m_positionCounterClockwise[0];
    Vec2 const v2 = point - m_positionCounterClockwise[0];

    float const dot00 = DotProduct2D(v0, v0);
    float const dot01 = DotProduct2D(v0, v1);
    float const dot02 = DotProduct2D(v0, v2);
    float const dot11 = DotProduct2D(v1, v1);
    float const dot12 = DotProduct2D(v1, v2);

    // Barycentric coordinates
    float invDenom = 1.0f / (dot00 * dot11 - dot01 * dot01);
    float u        = (dot11 * dot02 - dot01 * dot12) * invDenom;
    float v        = (dot00 * dot12 - dot01 * dot02) * invDenom;

    return (u >= 0) && (v >= 0) && (u + v <= 1);
}

//----------------------------------------------------------------------------------------------------
Vec2 Triangle2::GetNearestPoint(Vec2 const& point) const
{
    // 1. If the point is inside the triangle, return the point itself.
    if (IsPointInside(point))
    {
        return point;
    }

    // 2. Set the nearestPoint and minLengthSquared on one of the triangle's corner.
    Vec2  nearestPoint     = m_positionCounterClockwise[0];
    float minLengthSquared = (point - nearestPoint).GetLengthSquared();

    for (int edgeIndex = 0; edgeIndex < 3; ++edgeIndex)
    {
        // 3. Define the edgeStart and edgeEnd.
        Vec2 edgeStartPosition = m_positionCounterClockwise[edgeIndex];
        Vec2 edgeEndPosition   = m_positionCounterClockwise[(edgeIndex + 1) % 3];

        // 4. Calculate startToEnd direction on the edge and its lengthSquared.
        Vec2        edgeStartToEnd    = edgeEndPosition - edgeStartPosition;
        float const edgeLengthSquared = edgeStartToEnd.GetLengthSquared();

        // 5. If the edge's lengthSquared is zero, continue to the next edge.
        if (edgeLengthSquared == 0.f)
        {
            return point;
        }

        // 6. Project the point onto the infinite line defined by startToEnd, calculate its proportion t,
        // clamp t to the range [0, 1] to stay within the line segment.
        Vec2  startToPoint = point - edgeStartPosition;
        float t            = DotProduct2D(startToPoint, edgeStartToEnd) / edgeLengthSquared;
        t                  = GetClampedZeroToOne(t);

        // 7. Calculate the nearest point on the line segment.
        Vec2        closestPointOnEdge = edgeStartPosition + edgeStartToEnd * t;
        float const distanceSquared    = (point - closestPointOnEdge).GetLengthSquared();

        // 8. Update the nearest point if this edge is closer.
        if (distanceSquared < minLengthSquared)
        {
            minLengthSquared = distanceSquared;
            nearestPoint     = closestPointOnEdge;
        }
    }

    return nearestPoint;
}
