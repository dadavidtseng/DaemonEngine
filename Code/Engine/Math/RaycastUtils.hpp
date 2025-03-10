//----------------------------------------------------------------------------------------------------
// RaycastUtils.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include "Vec3.hpp"
#include "Engine/Math/Vec2.hpp"

struct FloatRange;
struct AABB3;
struct AABB2;

//----------------------------------------------------------------------------------------------------
struct Ray2
{
    Ray2(Vec2 const& origin, Vec2 const& direction, float maxDist);

    Vec2  m_origin;
    Vec2  m_direction;
    float m_maxDist;
};

// struct Ray2D {
// public:
//     Ray2D( Vec2 const& startPos, Vec2 const& forwardNormal, float maxDist );
//     Ray2D( Vec2 const& startPos, Vec2 const& endPos );
//     Ray2D( Vec2 const& startPos, float orientationDegrees, float maxDist );
// public:
//     Vec2 m_startPos;
//     float m_maxDist;
//     Vec2 m_forwardNormal;
// };
//
// struct Ray3D {
// public:
//     Ray3D( Vec3 const& startPos, Vec3 const& forwardNormal, float maxDist );
//     Ray3D( Vec3 const& startPos, Vec3 const& endPos );
// public:
//     Vec3 m_startPos;
//     float m_maxDist;
//     Vec3 m_forwardNormal;
// };

//----------------------------------------------------------------------------------------------------
struct RaycastResult2D
{
    // Basic raycast result information (required)
    bool  m_didImpact  = false;
    float m_impactDist = 0.f;
    Vec2  m_impactPos;
    Vec2  m_impactNormal;

    // Original raycast information (optional)
    Vec2  m_rayForwardNormal;
    Vec2  m_rayStartPos;
    float m_rayMaxLength = 1.f;
};

struct RaycastResult3D
{
    bool m_didImpact = false;
    float m_impactDist = 0.f;
    Vec3 m_impactPos;
    Vec3 m_impactNormal;
    Vec3 m_rayStartPos;
    Vec3 m_rayForwardNormal;
    float m_rayMaxLength = 1.f;
};

RaycastResult2D RaycastVsDisc2D(Vec2 const& startPos, Vec2 const& forwardNormal, float maxDist, Vec2 const& discCenter, float discRadius);
RaycastResult2D RayCastVsLineSegment2D(Vec2 const& startPos, Vec2 const& forwardNormal, float maxDist, Vec2 const& lineStartPos, Vec2 const& lineEndPos);
RaycastResult2D RayCastVsAABB2D(Vec2 const& startPos, Vec2 const& forwardNormal, float maxDist, AABB2 const& aabb2);
RaycastResult3D RaycastVsAABB3D(Vec3 rayStart, Vec3 rayForwardNormal, float rayLength, AABB3 box);
RaycastResult3D RaycastVsSphere3D(Vec3 rayStart, Vec3 rayForwardNormal, float rayLength, Vec3 sphereCenter, float sphereRadius);
RaycastResult3D RaycastVsCylinderZ3D(Vec3 rayStart, Vec3 rayForwardNormal, float rayLength, Vec3 const& centerXY, FloatRange const& minMaxZ, float radiusXY);
