//-----------------------------------------------------------------------------------------------
// LineSegment2.cpp
//-----------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------
#include "Engine/Math/LineSegment2.hpp"
#include "Engine/Math/MathUtils.hpp"

//-----------------------------------------------------------------------------------------------
LineSegment2::LineSegment2(Vec2 const& startPosition,
                           Vec2 const& endPosition,
                           float const thickness,
                           bool const  isInfinite)
    : m_startPosition(startPosition),
      m_endPosition(endPosition),
      m_thickness(thickness),
      m_isInfinite(isInfinite)
{
}

//----------------------------------------------------------------------------------------------------
float LineSegment2::GetLength() const
{
    return (m_startPosition - m_endPosition).GetLength();
}

//----------------------------------------------------------------------------------------------------
Vec2 LineSegment2::GetCenter() const
{
    return (m_startPosition + m_endPosition) * 0.5f;
}

Vec2 LineSegment2::GetNearestPoint(Vec2 const& point) const
{
    Vec2 const  startToEnd           = m_endPosition - m_startPosition;
    float const startToEndLengthSquared = startToEnd.GetLengthSquared();


    if (startToEndLengthSquared == 0.f)
    {
        return m_startPosition; // Return the start point as the nearest point
    }

    // Project the reference position onto the infinite line defined by m_start and m_end
    float const t = DotProduct2D((point - m_startPosition), startToEnd) / startToEndLengthSquared;

    if (m_isInfinite)
    {
        // Return the nearest point on the infinite line
        return m_startPosition + t * startToEnd;
    }

    // Clamp t to the range [0, 1] for the finite line segment
    float const clampedT = GetClampedZeroToOne(t);
    return m_startPosition + clampedT * startToEnd; // Return the nearest point on the line segment
}
