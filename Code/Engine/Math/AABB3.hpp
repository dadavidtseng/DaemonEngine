//----------------------------------------------------------------------------------------------------
// AABB3.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include "Engine/Math/Vec3.hpp"

//----------------------------------------------------------------------------------------------------
struct AABB3
{
    Vec3 m_mins;
    Vec3 m_maxs;

    static AABB3 ZERO_TO_ONE;
    static AABB3 NEG_HALF_TO_HALF;

    AABB3()                      = default;
    ~AABB3()                     = default;
    AABB3(AABB3 const& copyFrom) = default;
    explicit AABB3(float minX, float minY, float minZ, float maxX, float maxY, float maxZ);
    explicit AABB3(Vec3 const& mins, Vec3 const& maxs);

    bool       IsPointInside(Vec3 const& referencePoint) const;
    Vec3       GetNearestPoint(Vec3 const& referencePoint) const;
};
