//----------------------------------------------------------------------------------------------------
// Sphere3.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
//----------------------------------------------------------------------------------------------------
#include "Engine/Math/Vec3.hpp"

//----------------------------------------------------------------------------------------------------
struct Sphere3
{
    Vec3  m_centerPosition = Vec3::ZERO;
    float m_radius         = 0.f;

    Sphere3()                        = default;
    ~Sphere3()                       = default;
    Sphere3(Sphere3 const& copyFrom) = default;

    explicit Sphere3(Vec3 const& centerPosition, float radius);

    bool       IsPointInside(Vec3 const& point) const;
    Vec3       GetNearestPoint(Vec3 const& point) const;
};
