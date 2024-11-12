//----------------------------------------------------------------------------------------------------
// Disc2.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once

#include "Engine/Math/Vec2.hpp"

//----------------------------------------------------------------------------------------------------
struct Disc2
{
    Vec2  m_position = Vec2::ZERO; // Center position
    float m_radius   = 0.f;   // Radius

    // Construction/Destruction
    Disc2()                      = default;                 // Default constructor
    ~Disc2()                     = default;                 // Destructor
    Disc2(Disc2 const& copyFrom) = default;					// Copy constructor
    explicit Disc2(const Vec2& position, float radius);		// Explicit constructor

    // Accessors (const methods)
    bool  IsPointInside(Vec2 const& point) const; // Check if a point is inside the circle
    float GetRadius() const;
    Vec2  GetCenter() const;
    Vec2  GetNearestPoint(Vec2 const& referencePosition) const; // Get the nearest point

    // Mutators (non-const methods)
    void Translate(Vec2 const& translationToApply);
    void SetCenter(Vec2 const& newCenter);
    void SetRadius(float newRadius);
    void StretchToIncludePoint(Vec2 const& targetPointPos);
};
