//----------------------------------------------------------------------------------------------------
// Triangle2.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include "Engine/Math/Vec2.hpp"

//----------------------------------------------------------------------------------------------------
struct Triangle2
{
    Vec2 m_positionCounterClockwise[3];

    // Construction / Destruction
    Triangle2()                          = default;
    ~Triangle2()                         = default;
    Triangle2(Triangle2 const& copyFrom) = default;
    explicit Triangle2(Vec2 const& ccw1, Vec2 const& ccw2, Vec2 const& ccw3);
    explicit Triangle2(Vec2 const points[3]);

    // Accessors (const methods)
    bool IsPointInside(Vec2 const& point) const;
    Vec2 GetNearestPoint(Vec2 const& point) const;
};
