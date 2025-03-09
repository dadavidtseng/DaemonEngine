//-----------------------------------------------------------------------------------------------
// Triangle2.cpp
//

//-----------------------------------------------------------------------------------------------
#include "Engine/Math/Triangle2.hpp"

#include <cmath>

#include "Engine/Math/MathUtils.hpp" // Assuming this includes necessary math functions

// Destructor
Triangle2::~Triangle2() = default;

// Default constructor
Triangle2::Triangle2() = default;

// Copy constructor
Triangle2::Triangle2(Triangle2 const& copyFrom)
    : m_positionCounterClockwise{
        copyFrom.m_positionCounterClockwise[0],
        copyFrom.m_positionCounterClockwise[1],
        copyFrom.m_positionCounterClockwise[2]
    }
{
}

// Constructor from three points
Triangle2::Triangle2(Vec2 const& p1, Vec2 const& p2, Vec2 const& p3)
{
    m_positionCounterClockwise[0] = p1;
    m_positionCounterClockwise[1] = p2;
    m_positionCounterClockwise[2] = p3;
}

// Constructor from an array of points
Triangle2::Triangle2(Vec2 const points[3])
{
    m_positionCounterClockwise[0] = points[0];
    m_positionCounterClockwise[1] = points[1];
    m_positionCounterClockwise[2] = points[2];
}

// Check if a point is inside the triangle
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

// Get the center of the triangle
Vec2 Triangle2::GetCenter() const
{
    return (m_positionCounterClockwise[0] + m_positionCounterClockwise[1] + m_positionCounterClockwise[2]) / 3.0f;
}

// Get the nearest point on the triangle to a reference position
Vec2 Triangle2::GetNearestPoint(Vec2 const& referencePosition) const
{
    if (IsPointInside(referencePosition))
    {
        return  referencePosition;
    }
    
    // Start by finding the closest point on each edge of the triangle
    Vec2  nearestPoint       = m_positionCounterClockwise[0];
    float minDistanceSquared = (referencePosition - nearestPoint).GetLengthSquared();

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
            Vec2  startToPoint = referencePosition - edgeStart;
            float t            = DotProduct2D(startToPoint, edgeDirection) / edgeLengthSquared;
            t                  = GetClamped(t, 0.0f, 1.0f); // Clamp t to the range [0, 1] to stay within the segment

            Vec2  closestPointOnEdge = edgeStart + edgeDirection * t;
            float distanceSquared    = (referencePosition - closestPointOnEdge).GetLengthSquared();

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

// Translate the triangle by a given vector
void Triangle2::Translate(Vec2 const& translation)
{
    for (int i = 0; i < 3; ++i)
    {
        m_positionCounterClockwise[i] += translation;
    }
}

void Triangle2::SetCenter(Vec2 const& newCenter)
{
    Vec2 currentCenter = GetCenter();
    Vec2 translation   = newCenter - currentCenter;
    Translate(translation);
}

// Stretch the triangle to include a given point
void Triangle2::StretchToIncludePoint(Vec2 const& targetPointPos)
{
    if (IsPointInside(targetPointPos))
        return;

    // Update the triangle vertices if the point is outside
    for (int i = 0; i < 3; ++i)
    {
        if (targetPointPos.x < m_positionCounterClockwise[i].x)
            m_positionCounterClockwise[i].x = targetPointPos.x;

        if (targetPointPos.x > m_positionCounterClockwise[i].x)
            m_positionCounterClockwise[i].x = targetPointPos.x;

        if (targetPointPos.y < m_positionCounterClockwise[i].y)
            m_positionCounterClockwise[i].y = targetPointPos.y;

        if (targetPointPos.y > m_positionCounterClockwise[i].y)
            m_positionCounterClockwise[i].y = targetPointPos.y;
    }
}

void Triangle2::RotateAboutCenter(float degrees)
{
    Vec2 center = GetCenter();

    float radians = degrees * (PI / 180.f);

    for (int i = 0; i < 3; ++i)
    {
        Vec2 translatedPoint = m_positionCounterClockwise[i] - center;

        float rotatedX = translatedPoint.x * cos(radians) - translatedPoint.y * sin(radians);
        float rotatedY = translatedPoint.x * sin(radians) + translatedPoint.y * cos(radians);

        m_positionCounterClockwise[i] = Vec2(rotatedX, rotatedY) + center;
    }
}
