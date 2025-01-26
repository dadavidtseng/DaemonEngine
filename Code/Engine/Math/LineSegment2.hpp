//----------------------------------------------------------------------------------------------------
// LineSegment2.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include "Engine/Math/Vec2.hpp"

//----------------------------------------------------------------------------------------------------
struct LineSegment2
{
    Vec2  m_start      = Vec2::ZERO;
    Vec2  m_end        = Vec2::ZERO;
    float m_thickness  = 0.f;
    bool  m_isInfinite = false;

    LineSegment2() = default;
    explicit LineSegment2(Vec2 const& start, Vec2 const& end, float thickness, bool isInfinite);

    // Accessors (const methods)
    float GetLength() const;
    Vec2  GetCenter() const;
    Vec2  GetNearestPoint(Vec2 const& referencePosition) const;

    // Mutators (non-const methods)
    void Translate(Vec2 const& translation);
    void SetCenter(Vec2 const& newCenter);
    void RotateAboutCenter(float rotationDeltaDegrees);
};
