//----------------------------------------------------------------------------------------------------
// Cylinder.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"

struct FloatRange;

//----------------------------------------------------------------------------------------------------
struct Cylinder3
{
    Vec3  m_startPosition = Vec3::ZERO;
    Vec3  m_endPosition   = Vec3::ZERO;
    float m_radius        = 0.f;

    Cylinder3()                          = default;
    ~Cylinder3()                         = default;
    Cylinder3(Cylinder3 const& copyFrom) = default;
    explicit Cylinder3(Vec3 const& startPosition, Vec3 const& endPosition, float radius);

    Vec3 GetCenterPosition() const;
    Vec2 GetCenterPositionXY()const;
    FloatRange GetFloatRange() const;
    bool IsPointInside(Vec3 const& point) const;
    Vec3 GetNearestPoint(Vec3 const& point) const;
};
