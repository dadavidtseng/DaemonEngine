//----------------------------------------------------------------------------------------------------
// RaycastUtils.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"

struct Disc2;
struct FloatRange;
struct AABB3;
struct AABB2;

//----------------------------------------------------------------------------------------------------
struct Ray2
{
    Ray2(Vec2 const& startPosition, Vec2 const& normalDirection, float maxLength);
    Ray2(Vec2 const& startPosition, Vec2 const& endPosition);
    Ray2(Vec2 const& startPosition, float orientationDegrees, float maxLength);

    Vec2  m_startPosition   = Vec2::ZERO;
    Vec2  m_normalDirection = Vec2::ZERO;
    float m_maxLength       = 0.f;
};

//----------------------------------------------------------------------------------------------------
struct Ray3
{
    Ray3(Vec3 const& startPos, Vec3 const& forwardNormal, float maxDist);
    Ray3(Vec3 const& startPos, Vec3 const& endPos);

    Vec3  m_startPos;
    float m_maxDist;
    Vec3  m_forwardNormal;
};

//----------------------------------------------------------------------------------------------------
struct RaycastResult2D
{
    // Basic raycast result information (required)
    bool  m_didImpact             = false;
    float m_impactDistance        = 0.f;
    Vec2  m_impactPosition        = Vec2::ZERO;
    Vec2  m_impactNormalDirection = Vec2::ZERO;

    // Original raycast information (optional)
    Vec2  m_rayForwardNormal = Vec2::ZERO;
    Vec2  m_rayStartPosition = Vec2::ZERO;
    float m_rayMaxLength     = 1.f;
};

struct RaycastResult3D
{
    bool  m_didImpact  = false;
    float m_impactDist = 0.f;
    Vec3  m_impactPos;
    Vec3  m_impactNormal;
    Vec3  m_rayStartPos;
    Vec3  m_rayForwardNormal;
    float m_rayMaxLength = 1.f;
};

RaycastResult2D RaycastVsDisc2D(Vec2 const& rayStartPosition, Vec2 const& rayForwardNormal, float maxLength, Vec2 const& discCenter, float discRadius);
RaycastResult2D RaycastVsDisc2D(Ray2 const& ray, Vec2 const& discCenter, float discRadius);
RaycastResult2D RaycastVsDisc2D(Vec2 const& rayStartPosition, Vec2 const& rayForwardNormal, float maxLength, Disc2 const& disc);
RaycastResult2D RaycastVsDisc2D(Ray2 const& ray, Disc2 const& disc);
RaycastResult2D RayCastVsLineSegment2D(Vec2 const& rayStartPosition, Vec2 const& rayForwardNormal, float maxDist, Vec2 const& lineStartPos, Vec2 const& lineEndPos);
RaycastResult2D RayCastVsAABB2D(Vec2 const& rayStartPosition, Vec2 const& rayForwardNormal, float maxDist, AABB2 const& aabb2);
RaycastResult3D RaycastVsAABB3D(Vec3 rayStartPosition, Vec3 rayForwardNormal, float rayLength, AABB3 box);
RaycastResult3D RaycastVsSphere3D(Vec3 rayStartPosition, Vec3 rayForwardNormal, float rayLength, Vec3 sphereCenter, float sphereRadius);
RaycastResult3D RaycastVsCylinderZ3D(Vec3 rayStartPosition, Vec3 rayForwardNormal, float rayLength, Vec3 const& centerXY, FloatRange const& minMaxZ, float radiusXY);
