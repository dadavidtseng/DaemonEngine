//----------------------------------------------------------------------------------------------------
// Capsule2.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
//----------------------------------------------------------------------------------------------------
#include "Engine/Math/Vec2.hpp"

//----------------------------------------------------------------------------------------------------
struct Capsule2
{
    Vec2  m_startPosition = Vec2::ZERO;
    Vec2  m_endPosition   = Vec2::ZERO;
    float m_radius        = 0.f;

    // Construction / Destruction
    Capsule2() = default;
    explicit Capsule2(Vec2 const& startPosition, Vec2 const& endPosition, float radius);

    // Accessors (const methods)
    bool       IsPointInside(Vec2 const& point) const;
    Vec2       GetNearestPoint(Vec2 const& point) const;
};
