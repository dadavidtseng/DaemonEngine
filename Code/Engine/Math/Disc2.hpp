//----------------------------------------------------------------------------------------------------
// Disc2.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once

#include "Engine/Math/Vec2.hpp"

//----------------------------------------------------------------------------------------------------
struct Disc2
{
    Vec2  m_position = Vec2::ZERO;
    float m_radius   = 0.f;

    // Construction / Destruction
    Disc2()                      = default;
    ~Disc2()                     = default;
    Disc2(Disc2 const& copyFrom) = default;
    explicit Disc2(Vec2 const& position, float radius);

    // Accessors (const methods)
    bool  IsPointInside(Vec2 const& point) const;
    Vec2  GetNearestPoint(Vec2 const& point) const;
    Vec2  GetPosition() const;
    float GetRadius() const;

    // Mutators (non-const methods)
    void SetPosition(Vec2 const& newCenter);
    void SetRadius(float newRadius);
};
