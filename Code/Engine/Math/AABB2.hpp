//----------------------------------------------------------------------------------------------------
// AABB2.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
//----------------------------------------------------------------------------------------------------
#include "Engine/Math/Vec2.hpp"

//----------------------------------------------------------------------------------------------------
struct IntVec2;

//----------------------------------------------------------------------------------------------------
struct AABB2
{
    Vec2 m_mins = Vec2::ZERO;
    Vec2 m_maxs = Vec2::ZERO;

    static AABB2 ZERO_TO_ONE;
    static AABB2 NEG_HALF_TO_HALF;

    // Construction / Destruction
    AABB2()                      = default;
    ~AABB2()                     = default;
    AABB2(AABB2 const& copyFrom) = default;
    explicit AABB2(int minX, int minY, int maxX, int maxY);
    explicit AABB2(float minX, float minY, float maxX, float maxY);
    explicit AABB2(IntVec2 const& mins, IntVec2 const& maxs);
    explicit AABB2(Vec2 const& mins, Vec2 const& maxs);

    // Accessors (const methods)
    bool  IsPointInside(Vec2 const& point) const;
    Vec2  GetNearestPoint(Vec2 const& point) const;
    Vec2  GetCenter() const;
    Vec2  GetDimensions() const;
    Vec2  GetPointAtUV(Vec2 const& uv) const;        // uv=(0,0) is at mins; uv=(1,1) is at maxs
    Vec2  GetUVForPoint(Vec2 const& pointPos) const; // uv=(.5,.5) is at center; u or v outside [0,1] extrapolated
    AABB2 GetBoxAtUVs(Vec2 const& uvMins, Vec2 const& uvMaxs) const;
    float GetWidthOverHeightRatios() const;

    // Mutators (non-const methods)
    void Translate(Vec2 const& translationToApply);
    void SetCenter(Vec2 const& newCenter);
    void SetDimensions(Vec2 const& newDimensions);
    void StretchToIncludePoint(Vec2 const& targetPointPos);
    void ReduceToAspectRatio(float newAspectRatio);
    void EnlargeToAspectRatio(float newAspectRatio);
    void AddPadding(float xToAddOnBothSides, float yToAddToTopAndBottom);
    void ClampWithin(AABB2 const* containingBox);
    void ChopOffTop(float heightOfChoppedPiece);

    // Operators (const)
    bool operator==(AABB2 const& aabb2) const;
};
