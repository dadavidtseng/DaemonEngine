// Capsule2.hpp
#pragma once
#include "Engine/Math/Vec2.hpp"

//-----------------------------------------------------------------------------------------------
struct Capsule2
{
    Vec2  m_start  = Vec2::ZERO;
    Vec2  m_end    = Vec2::ZERO;
    float m_radius = 0.f;

    Capsule2();
    explicit Capsule2(Vec2 const& start, Vec2 const& end, float radius);

    // Mutators (non-const methods)
    void Translate(Vec2 const& translation);
    void SetCenter(Vec2 const& newCenter);
    void RotateAboutCenter(Vec2 const& rotationDeltaDegrees);

    // Accessors (const methods)
    bool       IsPointInside(Vec2 const& point) const;
    Vec2       GetNearestPoint(Vec2 const& point) const;
    Vec2 const GetCenter() const;
};
