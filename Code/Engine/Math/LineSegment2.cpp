//----------------------------------------------------------------------------------------------------
// LineSegment2.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Math/LineSegment2.hpp"

#include "Engine/Math/MathUtils.hpp"

//----------------------------------------------------------------------------------------------------
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

//----------------------------------------------------------------------------------------------------
Vec2 LineSegment2::GetNearestPoint(Vec2 const& point) const
{
    // 1. Calculate startToEnd direction on the line and its lengthSquared.
    Vec2 const  startToEnd              = m_endPosition - m_startPosition;
    float const startToEndLengthSquared = startToEnd.GetLengthSquared();

    // 2. If the line's lengthSquared is zero, return the startPosition of the line.
    if (startToEndLengthSquared == 0.f)
    {
        return m_startPosition;
    }

    // 3. Project the point onto the infinite line defined by startToEnd and calculate its proportion t.
    Vec2 const  startToPoint = point - m_startPosition;
    float const t            = DotProduct2D(startToPoint, startToEnd) / startToEndLengthSquared;

    // 4. If the line is infinite, return the nearest point on the infinite line.
    if (m_isInfinite == true)
    {
        return m_startPosition + t * startToEnd;
    }

    // 5. If the line is not infinite, clamp t to the range [0, 1] to stay within the line segment,
    // and return the nearest point on the line segment.
    float const clampedT = GetClampedZeroToOne(t);

    return m_startPosition + clampedT * startToEnd;
}
