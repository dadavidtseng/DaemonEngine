//----------------------------------------------------------------------------------------------------
// LineSegment2.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include "Engine/Math/Vec2.hpp"

//----------------------------------------------------------------------------------------------------
struct LineSegment2
{
    Vec2  m_startPosition = Vec2::ZERO;
    Vec2  m_endPosition   = Vec2::ZERO;
    float m_thickness     = 0.f;
    bool  m_isInfinite    = false;

    LineSegment2() = default;
    explicit LineSegment2(Vec2 const& startPosition, Vec2 const& endPosition, float thickness, bool isInfinite);

    // Accessors (const methods)
    float GetLength() const;
    Vec2  GetCenter() const;
    Vec2  GetNearestPoint(Vec2 const& point) const;
};
