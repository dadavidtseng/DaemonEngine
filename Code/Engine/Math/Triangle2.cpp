//----------------------------------------------------------------------------------------------------
// Triangle2.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Math/Triangle2.hpp"

#include <cmath>

#include "Engine/Math/MathUtils.hpp" // Assuming this includes necessary math functions

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

// Get the nearest point on the triangle to a reference position
Vec2 Triangle2::GetNearestPoint(Vec2 const& point) const
{
    if (IsPointInside(point))
    {
        return point;
    }

    // Start by finding the closest point on each edge of the triangle
    Vec2  nearestPoint       = m_positionCounterClockwise[0];
    float minDistanceSquared = (point - nearestPoint).GetLengthSquared();

    for (int i = 0; i < 3; ++i)
    {
        // Define the endpoints of the current edge
        Vec2 edgeStart = m_positionCounterClockwise[i];
        Vec2 edgeEnd   = m_positionCounterClockwise[(i + 1) % 3];

        // Find the nearest point on the edge segment to referencePosition
        Vec2  edgeDirection     = edgeEnd - edgeStart;
        float edgeLengthSquared = edgeDirection.GetLengthSquared();

        if (edgeLengthSquared > 0.0f)
        {
            // Project referencePosition onto the edge (using dot product) and clamp to segment
            Vec2  startToPoint = point - edgeStart;
            float t            = DotProduct2D(startToPoint, edgeDirection) / edgeLengthSquared;
            t                  = GetClamped(t, 0.0f, 1.0f); // Clamp t to the range [0, 1] to stay within the segment

            Vec2  closestPointOnEdge = edgeStart + edgeDirection * t;
            float distanceSquared    = (point - closestPointOnEdge).GetLengthSquared();

            // Update the nearest point if this edge is closer
            if (distanceSquared < minDistanceSquared)
            {
                minDistanceSquared = distanceSquared;
                nearestPoint       = closestPointOnEdge;
            }
        }
    }

    return nearestPoint;
}






