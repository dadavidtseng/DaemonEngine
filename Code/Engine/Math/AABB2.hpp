//----------------------------------------------------------------------------------------------------
// AABB2.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include "Engine/Math/Vec2.hpp"

//----------------------------------------------------------------------------------------------------
struct IntVec2;

//----------------------------------------------------------------------------------------------------
struct AABB2
{
    Vec2         m_mins;
    Vec2         m_maxs;
    static AABB2 ZERO_TO_ONE;
    static AABB2 NEG_HALF_TO_HALF;

    // Construction/Destruction
    AABB2()                      = default;     // default constructor (do nothing)
    ~AABB2()                     = default;     // destructor (do nothing)
    AABB2(AABB2 const& copyFrom) = default;     // copy constructor (from another vec2)
    explicit AABB2(float minX, float minY, float maxX, float maxY); // explicit constructor (from x1, y1, x2, y2)
    explicit AABB2(Vec2 const& mins, Vec2 const& maxs);             // explicit constructor (from mins, maxs)
    explicit AABB2(IntVec2 const& mins, IntVec2 const& maxs);

    // Accessors (const methods)
    bool IsPointInside(Vec2 const& point) const;
    Vec2 GetCenter() const;
    Vec2 GetDimensions() const;
    Vec2 GetNearestPoint(Vec2 const& referencePoint) const;
    Vec2 GetPointAtUV(Vec2 const& uv) const;        // uv=(0,0) is at mins; uv=(1,1) is at maxs
    Vec2 GetUVForPoint(Vec2 const& pointPos) const; // uv=(.5,.5) is at center; u or v outside [0,1] extrapolated

    // Mutators (non-const methods)
    void Translate(Vec2 const& translationToApply);
    void SetCenter(Vec2 const& newCenter);
    void SetDimensions(Vec2 const& newDimensions);
    void StretchToIncludePoint(Vec2 const& targetPointPos);

    bool operator==(const AABB2& aabb2) const;
};
