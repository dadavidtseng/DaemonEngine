//----------------------------------------------------------------------------------------------------
// RaycastUtils.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include <cstdint>

#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"

struct Plane3;
struct OBB3;
//-Forward-Declaration--------------------------------------------------------------------------------
struct FloatRange;

//----------------------------------------------------------------------------------------------------
enum class eAABB2HitType : int8_t
{
    NONE = -1,
    TOP,
    BOTTOM,
    LEFT,
    RIGHT
};

//----------------------------------------------------------------------------------------------------
enum class eAABB3HitType : int8_t
{
    NONE = -1,
    TOP,
    BOTTOM,
    LEFT,
    RIGHT,
    FRONT,
    BACK
};

//----------------------------------------------------------------------------------------------------
struct Ray2
{
    Ray2(Vec2 const& startPosition, Vec2 const& forwardNormal, float maxLength);
    Ray2(Vec2 const& startPosition, Vec2 const& endPosition);
    Ray2(Vec2 const& startPosition, float orientationDegrees, float maxLength);

    Vec2  m_startPosition = Vec2::ZERO;
    Vec2  m_forwardNormal = Vec2::ZERO;
    float m_maxLength     = 0.f;
};

//----------------------------------------------------------------------------------------------------
struct Ray3
{
    Ray3(Vec3 const& startPosition, Vec3 const& forwardNormal, float maxLength);
    Ray3(Vec3 const& startPosition, Vec3 const& endPosition);

    Vec3  m_startPosition = Vec3::ZERO;
    Vec3  m_forwardNormal = Vec3::ZERO;
    float m_maxLength     = 0.f;
};

//----------------------------------------------------------------------------------------------------
struct RaycastResult2D
{
    // Basic raycast result information (required)
    bool  m_didImpact      = false;
    Vec2  m_impactPosition = Vec2::ZERO;
    Vec2  m_impactNormal   = Vec2::ZERO;
    float m_impactLength   = 0.f;

    // Original raycast information (optional)
    Vec2  m_rayStartPosition = Vec2::ZERO;
    Vec2  m_rayForwardNormal = Vec2::ZERO;
    float m_rayMaxLength     = 0.f;
};

//----------------------------------------------------------------------------------------------------
struct RaycastResult3D
{
    // Basic raycast result information (required)
    bool  m_didImpact      = false;
    Vec3  m_impactPosition = Vec3::ZERO;
    Vec3  m_impactNormal   = Vec3::ZERO;
    float m_impactLength   = 0.f;

    // Original raycast information (optional)
    Vec3  m_rayStartPosition = Vec3::ZERO;
    Vec3  m_rayForwardNormal = Vec3::ZERO;
    float m_rayMaxLength     = 0.f;
};

RaycastResult2D RaycastVsDisc2D(Vec2 const& rayStartPosition, Vec2 const& rayForwardNormal, float maxLength, Vec2 const& discCenter, float discRadius);
RaycastResult2D RaycastVsLineSegment2D(Vec2 const& rayStartPosition, Vec2 const& rayForwardNormal, float maxLength, Vec2 const& lineStartPosition, Vec2 const& lineEndPosition);
RaycastResult2D RaycastVsAABB2D(Vec2 const& rayStartPosition, Vec2 const& rayForwardNormal, float maxLength, Vec2 const& aabb2Mins, Vec2 const& aabb2Maxs);

RaycastResult3D RaycastVsSphere3D(Vec3 const& rayStartPosition, Vec3 const& rayForwardNormal, float maxLength, Vec3 const& sphereCenter, float sphereRadius);
RaycastResult3D RaycastVsAABB3D(Vec3 const& rayStartPosition, Vec3 const& rayForwardNormal, float maxLength, Vec3 const& aabb3Mins, Vec3 const& aabb3Maxs);
RaycastResult3D RaycastVsCylinderZ3D(Vec3 const& rayStartPosition, Vec3 const& rayForwardNormal, float maxLength, Vec2 const& cylinderCenterXY, FloatRange const& cylinderMinMaxZ, float cylinderRadius);
RaycastResult3D RaycastVsOBB3D(Vec3 const& rayStartPosition, Vec3 const& rayForwardNormal, float maxLength, OBB3 const& obb3);
RaycastResult3D RaycastVsPlane3D(Vec3 const& rayStartPosition, Vec3 const& rayForwardNormal, float maxLength, Plane3 const& plane3);
