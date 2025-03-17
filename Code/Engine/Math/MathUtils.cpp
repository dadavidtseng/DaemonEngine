//----------------------------------------------------------------------------------------------------
// MathUtils.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Math/MathUtils.hpp"

#include <cmath>

#include "Engine/Math/Capsule2.hpp"
// #include "LineSegment2.hpp"
#include "Cylinder3.hpp"
#include "FloatRange.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Math/Mat44.hpp"
#include "Engine/Math/Disc2.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/OBB2.hpp"
#include "Engine/Math/Triangle2.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/Vec4.hpp"
#include "Engine/Math/Sphere3.hpp"

//-Start-of-Clamp-and-Lerp----------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
float GetClampedZeroToOne(float const value)
{
    return GetClamped(value, 0.f, 1.f);
}

//----------------------------------------------------------------------------------------------------
float Interpolate(float const start,
                  float const end,
                  float const fractionTowardEnd)
{
    float const range = end - start;

    return start + range * fractionTowardEnd;
}

//----------------------------------------------------------------------------------------------------
float GetFractionWithinRange(float const value,
                             float const rangeStart,
                             float const rangeEnd)
{
    float const range = rangeEnd - rangeStart;

    return range == 0.f ? 0.f : (value - rangeStart) / range;
}

//----------------------------------------------------------------------------------------------------
float RangeMap(float const inValue,
               float const inStart,
               float const inEnd,
               float const outStart,
               float const outEnd)
{
    float const fractionTowardEnd = GetFractionWithinRange(inValue, inStart, inEnd);

    return Interpolate(outStart, outEnd, fractionTowardEnd);
}

//----------------------------------------------------------------------------------------------------
float RangeMapClamped(float const inValue,
                      float const inStart,
                      float const inEnd,
                      float const outStart,
                      float const outEnd)
{
    float const clampedInValue = GetClamped(inValue, inStart, inEnd);

    return RangeMap(clampedInValue, inStart, inEnd, outStart, outEnd);
}

//----------------------------------------------------------------------------------------------------
int RoundDownToInt(float const value)
{
    return static_cast<int>(floorf(value));
}

//-End-of-Clamp-and-Lerp------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
//-Start-of-Angle-Utilities---------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
float ConvertDegreesToRadians(float const degrees)
{
    return degrees * (PI / 180.f);
}

//----------------------------------------------------------------------------------------------------
float ConvertRadiansToDegrees(float const radians)
{
    return radians * (180.f / PI);
}

//----------------------------------------------------------------------------------------------------
float CosDegrees(float const degrees)
{
    return cosf(ConvertDegreesToRadians(degrees));
}

//----------------------------------------------------------------------------------------------------
float SinDegrees(float const degrees)
{
    return sinf(ConvertDegreesToRadians(degrees));
}

//----------------------------------------------------------------------------------------------------
float Atan2Degrees(float const y,
                   float const x)
{
    return ConvertRadiansToDegrees(atan2(y, x));
}

//----------------------------------------------------------------------------------------------------
float GetShortestAngularDispDegrees(float const startDegrees,
                                    float const endDegrees)
{
    float disp = endDegrees - startDegrees;

    while (disp > 180.f)
    {
        disp -= 360.f;
    }

    while (disp < -180.f)
    {
        disp += 360.f;
    }

    return disp;
}

//----------------------------------------------------------------------------------------------------
float GetTurnedTowardDegrees(float const currentDegrees,
                             float const goalDegrees,
                             float const maxDeltaDegrees)
{
    float const angDispDeg = GetShortestAngularDispDegrees(currentDegrees, goalDegrees);

    if (fabsf(angDispDeg) < maxDeltaDegrees)
    {
        return goalDegrees;
    }

    if (angDispDeg > 0.f)
    {
        return currentDegrees + maxDeltaDegrees;
    }

    return currentDegrees - maxDeltaDegrees;
}

//----------------------------------------------------------------------------------------------------
float GetAngleDegreesBetweenVectors2D(Vec2 const& a,
                                      Vec2 const& b)
{
    float const dotProduct = DotProduct2D(a, b);
    float const magnitudeA = a.GetLength();
    float const magnitudeB = b.GetLength();

    // If one of the vectors is zero, the angle is undefined, so return 0
    if (magnitudeA == 0.f || magnitudeB == 0.f)
    {
        return 0.f;
    }

    float cosTheta = dotProduct / (magnitudeA * magnitudeB);

    // Clamp the value to the valid range of acos (to avoid numerical issues)
    cosTheta = GetClamped(cosTheta, -1.0f, 1.0f);

    // Compute the angle in radians and convert to degrees
    float const angleRadians = acosf(cosTheta);
    float const angleDegrees = ConvertRadiansToDegrees(angleRadians);

    return angleDegrees;
}

//-End-of-Angle-Utilities-----------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
//-Start-of-Dot-and-Cross-----------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
float DotProduct2D(Vec2 const& a,
                   Vec2 const& b)
{
    return
        a.x * b.x +
        a.y * b.y;
}

//----------------------------------------------------------------------------------------------------
float DotProduct3D(Vec3 const& a, Vec3 const& b)
{
    return
        a.x * b.x +
        a.y * b.y +
        a.z * b.z;
}

//----------------------------------------------------------------------------------------------------
float DotProduct4D(Vec4 const& a, Vec4 const& b)
{
    return
        a.x * b.x +
        a.y * b.y +
        a.z * b.z +
        a.w * b.w;
}

//----------------------------------------------------------------------------------------------------
float CrossProduct2D(Vec2 const& a, Vec2 const& b)
{
    return
        a.x * b.y -
        a.y * b.x;
}

//----------------------------------------------------------------------------------------------------
Vec3 CrossProduct3D(Vec3 const& a, Vec3 const& b)
{
    return
        Vec3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}

//-End-of-Dot-and-Cross-------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
//-Start-of-Distance-&-Projections-Utilities----------------------------------------------------------

//----------------------------------------------------------------------------------------------------
float GetDistance2D(Vec2 const& positionA,
                    Vec2 const& positionB)
{
    float const BxToAx = positionA.x - positionB.x;
    float const ByToAy = positionA.y - positionB.y;

    return
        sqrtf(BxToAx * BxToAx + ByToAy * ByToAy);
}

//----------------------------------------------------------------------------------------------------
float GetDistanceSquared2D(Vec2 const& positionA,
                           Vec2 const& positionB)
{
    float const BxToAx = positionA.x - positionB.x;
    float const ByToAy = positionA.y - positionB.y;

    return
        BxToAx * BxToAx +
        ByToAy * ByToAy;
}

//----------------------------------------------------------------------------------------------------
float GetDistance3D(Vec3 const& positionA,
                    Vec3 const& positionB)
{
    float const BxToAx = positionA.x - positionB.x;
    float const ByToAy = positionA.y - positionB.y;
    float const BzToAz = positionA.z - positionB.z;

    return
        sqrtf(BxToAx * BxToAx + ByToAy * ByToAy + BzToAz * BzToAz);
}

//----------------------------------------------------------------------------------------------------
float GetDistanceSquared3D(Vec3 const& positionA,
                           Vec3 const& positionB)
{
    float const BxToAx = positionA.x - positionB.x;
    float const ByToAy = positionA.y - positionB.y;
    float const BzToAz = positionA.z - positionB.z;

    return
        BxToAx * BxToAx +
        ByToAy * ByToAy +
        BzToAz * BzToAz;
}

//----------------------------------------------------------------------------------------------------
float GetDistanceXY3D(Vec3 const& positionA,
                      Vec3 const& positionB)
{
    float const BxToAx = positionA.x - positionB.x;
    float const ByToAy = positionA.y - positionB.y;

    return
        sqrtf(BxToAx * BxToAx + ByToAy * ByToAy);
}

//----------------------------------------------------------------------------------------------------
float GetDistanceXYSquared3D(Vec3 const& positionA,
                             Vec3 const& positionB)
{
    float const BxToAx = positionA.x - positionB.x;
    float const ByToAy = positionA.y - positionB.y;

    return
        BxToAx * BxToAx +
        ByToAy * ByToAy;
}

//----------------------------------------------------------------------------------------------------
int GetTaxicabDistance2D(IntVec2 const& pointA,
                         IntVec2 const& pointB)
{
    int const deltaX = abs(pointA.x - pointB.x);
    int const deltaY = abs(pointA.y - pointB.y);

    return
        deltaX + deltaY;
}

//----------------------------------------------------------------------------------------------------
float GetProjectedLength2D(Vec2 const& vectorToProject,
                           Vec2 const& vectorToProjectOnto)
{
    float const vectorToProjectOntoLength = vectorToProjectOnto.GetLength();

    if (vectorToProjectOntoLength == 0.f)
    {
        return 0.f;
    }

    float const dotProduct      = DotProduct2D(vectorToProject, vectorToProjectOnto);
    float const projectedLength = dotProduct / vectorToProjectOntoLength;

    return projectedLength;
}

//----------------------------------------------------------------------------------------------------
Vec2 GetProjectedOnto2D(Vec2 const& vectorToProject,
                        Vec2 const& vectorToProjectOnto)
{
    float const vectorToProjectOntoLengthSquared = vectorToProjectOnto.GetLengthSquared();

    if (vectorToProjectOntoLengthSquared == 0.f)
    {
        return Vec2::ZERO;
    }

    float const dotProduct      = DotProduct2D(vectorToProject, vectorToProjectOnto);
    float const projectionScale = dotProduct / vectorToProjectOntoLengthSquared;

    return vectorToProjectOnto * projectionScale;
}

//-End-of-Distance-&-Projections-Utilities------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
//-Start-of-Geometry-Query-Utilities------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
bool DoDiscsOverlap(Vec2 const& centerA,
                    const float radiusA,
                    Vec2 const& centerB,
                    const float radiusB)
{
    return
        GetDistance2D(centerA, centerB) < (radiusA + radiusB);
}

//----------------------------------------------------------------------------------------------------
bool DoSpheresOverlap(Vec3 const& centerA,
                      float const radiusA,
                      Vec3 const& centerB,
                      float const radiusB)
{
    return
        GetDistance3D(centerA, centerB) < (radiusA + radiusB);
}

//----------------------------------------------------------------------------------------------------
bool DoAABB2sOverlap2D(AABB2 const& boxA, AABB2 const& boxB)
{
    float aMinX = boxA.m_mins.x;
    float aMaxX = boxA.m_maxs.x;
    float aMinY = boxA.m_mins.y;
    float aMaxY = boxA.m_maxs.y;
    float bMinX = boxB.m_mins.x;
    float bMaxX = boxB.m_maxs.x;
    float bMinY = boxB.m_mins.y;
    float bMaxY = boxB.m_maxs.y;

    if (aMaxX > bMinX && bMaxX > aMinX && aMaxY > bMinY && bMaxY > aMinY)
    {
        return true;
    }
    return false;
}

bool DoAABB3sOverlap3D(AABB3 const& first, AABB3 const& second)
{
    float aMinX = first.m_mins.x;
    float aMaxX = first.m_maxs.x;
    float aMinY = first.m_mins.y;
    float aMaxY = first.m_maxs.y;
    float aMinZ = first.m_mins.z;
    float aMaxZ = first.m_maxs.z;
    float bMinX = second.m_mins.x;
    float bMaxX = second.m_maxs.x;
    float bMinY = second.m_mins.y;
    float bMaxY = second.m_maxs.y;
    float bMinZ = second.m_mins.z;
    float bMaxZ = second.m_maxs.z;

    if (aMaxX > bMinX && bMaxX > aMinX && aMaxY > bMinY && bMaxY > aMinY && aMaxZ > bMinZ && bMaxZ > aMinZ) {
        return true;
    }
    return false;
}

bool DoZCylindersOverlap3D(Vec2 cylinder1CenterXY, float cylinder1Radius, FloatRange cylinder1MinMaxZ, Vec2 cylinder2CenterXY, float cylinder2Radius, FloatRange cylinder2MinMaxZ)
{
    return false;
}

bool DoSphereAndAABB3Overlap3D(Vec3 sphereCenter, float sphereRadius, AABB3 box)
{
    return false;
}

bool DoZCylinderAndAABB3Overlap3D(Vec2 cylinderCenterXY, float cylinderRadius, FloatRange cylinderMinMaxZ, AABB3 box)
{
    return false;
}

bool DoZCylinderAndSphereOverlap3D(Vec2 cylinderCenterXY, float cylinderRadius, FloatRange cylinderMinMaxZ, Vec3 sphereCenter, float sphereRadius)
{
    return false;
}

//----------------------------------------------------------------------------------------------------
bool PushDiscOutOfPoint2D(Vec2&       mobileDiscCenter,
                          const float discRadius,
                          Vec2 const& fixedPoint)
{
    Vec2        fixedPointToDiscCenter = mobileDiscCenter - fixedPoint;
    const float discRadiusSquared      = discRadius * discRadius;

    if (fixedPointToDiscCenter.GetLengthSquared() >= discRadiusSquared)
    {
        return false;
    }

    fixedPointToDiscCenter.SetLength(discRadius);

    mobileDiscCenter = fixedPoint + fixedPointToDiscCenter;

    return true;
}

//----------------------------------------------------------------------------------------------------
bool PushDiscOutOfDisc2D(Vec2&       mobileDiscCenter,
                         const float mobileDiscRadius,
                         Vec2 const& fixedDiscCenter,
                         const float fixedDiscRadius)
{
    Vec2        fixedPointToDiscCenter  = mobileDiscCenter - fixedDiscCenter;
    const float twoDiscRadiusSum        = mobileDiscRadius + fixedDiscRadius;
    const float twoDiscRadiusSumSquared = twoDiscRadiusSum * twoDiscRadiusSum;

    if (fixedPointToDiscCenter.GetLengthSquared() >= twoDiscRadiusSumSquared)
    {
        return false;
    }

    fixedPointToDiscCenter.SetLength(twoDiscRadiusSum);

    mobileDiscCenter = fixedDiscCenter + fixedPointToDiscCenter;

    return true;
}

//----------------------------------------------------------------------------------------------------
bool PushDiscsOutOfEachOther2D(Vec2&       aCenter,
                               const float aRadius,
                               Vec2&       bCenter,
                               const float bRadius)
{
    const Vec2  aCenterTobCenter        = bCenter - aCenter;
    const float twoDiscRadiusSum        = aRadius + bRadius;
    const float twoDiscRadiusSumSquared = twoDiscRadiusSum * twoDiscRadiusSum;

    if (aCenterTobCenter.GetLengthSquared() >= twoDiscRadiusSumSquared)
    {
        return false;
    }

    const float overlapDist   = twoDiscRadiusSum - aCenterTobCenter.GetLength();
    const Vec2  correctionVec = aCenterTobCenter.GetNormalized() * (overlapDist / 2.f);

    aCenter = aCenter - correctionVec;
    bCenter = bCenter + correctionVec;

    return true;
}

//----------------------------------------------------------------------------------------------------
bool PushDiscOutOfAABB2D(Vec2&        mobileDiscCenter,
                         const float  discRadius,
                         AABB2 const& fixedBox)
{
    const Vec2 nearestPoint = fixedBox.GetNearestPoint(mobileDiscCenter);

    return PushDiscOutOfPoint2D(mobileDiscCenter, discRadius, nearestPoint);
}

//-End-of-Geometry-Query-Utilities--------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
//-Start-of-Is-Point-Inside-Utilities-----------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
bool IsPointInsideDisc2D(Vec2 const& point,
                         Vec2 const& discCenter,
                         float const discRadius)
{
    float const distanceSquared = GetDistanceSquared2D(point, discCenter);
    float const radiusSquared   = discRadius * discRadius;

    return distanceSquared <= radiusSquared;
}

//----------------------------------------------------------------------------------------------------
bool IsPointInsideAABB2D(Vec2 const& point,
                         Vec2 const& aabb2Mins,
                         Vec2 const& aabb2Maxs)
{
    return
        point.x >= aabb2Mins.x &&
        point.x <= aabb2Maxs.x &&
        point.y >= aabb2Mins.y &&
        point.y <= aabb2Maxs.y;
}

//----------------------------------------------------------------------------------------------------
bool IsPointInsideOBB2D(Vec2 const& point,
                        Vec2 const& obb2Center,
                        Vec2 const& obb2IBasisNormal,
                        Vec2 const& obb2HalfDimensions)
{
    Vec2 const  centerToWorldPosition = point - obb2Center;
    float const localX                = DotProduct2D(centerToWorldPosition, obb2IBasisNormal);
    float const localY                = DotProduct2D(centerToWorldPosition, Vec2(-obb2IBasisNormal.y, obb2IBasisNormal.x));

    Vec2 const localPoint = Vec2(localX, localY);
    Vec2 const obbMins    = -obb2HalfDimensions;
    Vec2 const obbMaxs    = obb2HalfDimensions;

    return
        localPoint.x >= obbMins.x &&
        localPoint.x <= obbMaxs.x &&
        localPoint.y >= obbMins.y &&
        localPoint.y <= obbMaxs.y;
}

//----------------------------------------------------------------------------------------------------
bool IsPointInsideCapsule(Vec2 const& point, Vec2 const& capsuleStartPosition, Vec2 const& capsuleEndPosition, float const capsuleRadius)
{
    // Calculate the capsule's direction vector
    Vec2 const startToEnd = capsuleEndPosition - capsuleStartPosition;

    if (startToEnd == Vec2::ZERO)
    {
        float const distanceSquared = GetDistanceSquared2D(point, capsuleStartPosition);
        float const radiusSquared   = capsuleRadius * capsuleRadius;

        return distanceSquared <= radiusSquared;
    }

    float const capsuleLength    = startToEnd.GetLength();
    Vec2 const  startToEndNormal = startToEnd.GetNormalized();

    // Calculate the projection of the point onto the capsule's direction
    float const projectionLength        = GetProjectedLength2D(point - capsuleStartPosition, startToEndNormal);
    float const clampedProjectionLength = GetClamped(projectionLength, 0.f, capsuleLength);

    // Find the nearest point on the capsule segment
    Vec2 const nearestPointOnSegment = capsuleStartPosition + startToEndNormal * clampedProjectionLength;

    // Calculate the distance from the point to the nearest point on the segment
    float const distanceToSegment = (point - nearestPointOnSegment).GetLength();

    // Check if the point is within the radius of the capsule
    return
        distanceToSegment <= capsuleRadius ||
        (point - capsuleStartPosition).GetLengthSquared() <= capsuleRadius * capsuleRadius ||
        (point - capsuleEndPosition).GetLengthSquared() <= capsuleRadius * capsuleRadius;
}

//----------------------------------------------------------------------------------------------------
bool IsPointInsideTriangle(Vec2 const& point, Vec2 const& ccw1, Vec2 const& ccw2, Vec2 const& ccw3)
{
    // Implement the barycentric method to check if the point is inside the triangle
    Vec2 const v0 = ccw2 - ccw1;
    Vec2 const v1 = ccw3 - ccw1;
    Vec2 const v2 = point - ccw1;

    float const dot00 = DotProduct2D(v0, v0);
    float const dot01 = DotProduct2D(v0, v1);
    float const dot02 = DotProduct2D(v0, v2);
    float const dot11 = DotProduct2D(v1, v1);
    float const dot12 = DotProduct2D(v1, v2);

    // Barycentric coordinates
    float invDenom = 1.0f / (dot00 * dot11 - dot01 * dot01);
    float u        = (dot11 * dot02 - dot01 * dot12) * invDenom;
    float v        = (dot00 * dot12 - dot01 * dot02) * invDenom;

    return (u >= 0) && (v >= 0) && (u + v <= 1);
}

//----------------------------------------------------------------------------------------------------
bool IsPointInsideOrientedSector2D(Vec2 const& point,
                                   Vec2 const& sectorTip,
                                   const float sectorForwardDegrees,
                                   const float sectorApertureDegrees,
                                   const float sectorRadius)
{
    const Vec2  pointToSectorTip = point - sectorTip;
    const float distanceToPoint  = pointToSectorTip.GetLength();

    // Check if the point is within the sector's radius
    if (distanceToPoint > sectorRadius)
    {
        return false; // Point is outside the sector's radius
    }

    // Calculate the forward direction vector of the sector
    const Vec2 sectorForwardVec = Vec2(CosDegrees(sectorForwardDegrees), SinDegrees(sectorForwardDegrees));

    // Compute the angle between the sector's forward vector and the vector to the point
    const float angleToPoint = GetAngleDegreesBetweenVectors2D(sectorForwardVec, pointToSectorTip);

    // Check if the angle is within half the sector's aperture
    const float halfAperture = sectorApertureDegrees / 2.f;

    return fabsf(angleToPoint) <= halfAperture;
}

//----------------------------------------------------------------------------------------------------
bool IsPointInsideDirectedSector2D(Vec2 const& point,
                                   Vec2 const& sectorTip,
                                   Vec2 const& sectorForwardNormal,
                                   const float sectorApertureDegrees,
                                   const float sectorRadius)
{
    // Calculate the vector from the sector tip to the point
    const Vec2 pointToSectorTip = point - sectorTip;

    // Check if the point is within the sector's radius
    if (pointToSectorTip.GetLengthSquared() > sectorRadius * sectorRadius)
    {
        return false; // Point is outside the sector's radius
    }

    // Normalize the direction from the sector tip to the point
    const Vec2 directionToPoint = pointToSectorTip.GetNormalized();

    // Compute the dot product between the sector's forward direction and the point direction
    const float dotProduct = DotProduct2D(sectorForwardNormal, directionToPoint);

    // Use dot product to directly determine if the point lies within the sector's aperture
    const float cosHalfAperture = CosDegrees(sectorApertureDegrees / 2.f);

    return dotProduct >= cosHalfAperture;
}

//-End-of-Is-Point-Inside-Utilities-------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
//-Start-of-Get-Nearest-Point-Utilities---------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
Vec2 GetNearestPointOnDisc2D(Vec2 const& point,
                             Vec2 const& discCenter,
                             float const discRadius)
{
    // 1. If the point is inside the disc, return the point itself.
    if (IsPointInsideDisc2D(point, discCenter, discRadius) == true)
    {
        return point;
    }

    // 2. Calculate the nearest point on the disc.
    Vec2 const centerToPoint       = point - discCenter;
    Vec2 const centerToPointNormal = centerToPoint.GetNormalized();

    return discCenter + centerToPointNormal * discRadius;
}

//----------------------------------------------------------------------------------------------------
Vec2 GetNearestPointOnDisc2D(Vec2 const&  point,
                             Disc2 const& disc)
{
    return disc.GetNearestPoint(point);
}

//----------------------------------------------------------------------------------------------------
Vec2 GetNearestPointOnAABB2D(Vec2 const& point,
                             Vec2 const& aabb2Mins,
                             Vec2 const& aabb2Maxs)
{
    if (IsPointInsideAABB2D(point, aabb2Mins, aabb2Maxs) == true)
    {
        return point;
    }

    float const clampedX = GetClamped(point.x, aabb2Mins.x, aabb2Maxs.x);
    float const clampedY = GetClamped(point.y, aabb2Mins.y, aabb2Maxs.y);

    return
        Vec2(clampedX, clampedY);
}

//----------------------------------------------------------------------------------------------------
Vec2 GetNearestPointOnOBB2D(Vec2 const& point,
                            Vec2 const& obb2Center,
                            Vec2 const& obb2IBasisNormal,
                            Vec2 const& obb2HalfDimensions)
{
    if (IsPointInsideOBB2D(point, obb2Center, obb2IBasisNormal, obb2HalfDimensions) == true)
    {
        return point;
    }

    Vec2 const centerToWorldPosition = point - obb2Center;
    float      localPointX           = DotProduct2D(centerToWorldPosition, obb2IBasisNormal);
    float      localPointY           = DotProduct2D(centerToWorldPosition, Vec2(-obb2IBasisNormal.y, obb2IBasisNormal.x));

    // Vec2 localPoint = GetLocalPosFromWorldPos(point);

    localPointX = GetClamped(localPointX, -obb2HalfDimensions.x, obb2HalfDimensions.x);
    localPointY = GetClamped(localPointY, -obb2HalfDimensions.y, obb2HalfDimensions.y);

    Vec2 worldPosition = obb2Center;
    worldPosition += obb2IBasisNormal * localPointX;
    worldPosition += Vec2(-obb2IBasisNormal.y, obb2IBasisNormal.x) * localPointY;

    return worldPosition;
}

//----------------------------------------------------------------------------------------------------
Vec2 GetNearestPointOnLineSegment2D(Vec2 const&         point,
                                    Vec2 const& lineStartPosition,
                                    Vec2 const& lineEndPosition,
                                    bool const isLineInfinite)
{
    Vec2 const  startToEnd           = lineEndPosition - lineStartPosition;
    float const startToEndLengthSquared = startToEnd.GetLengthSquared();


    if (startToEndLengthSquared == 0.f)
    {
        return lineStartPosition; // Return the start point as the nearest point
    }

    // Project the reference position onto the infinite line defined by m_start and m_end
    float const t = DotProduct2D((point - lineStartPosition), startToEnd) / startToEndLengthSquared;

    if (isLineInfinite)
    {
        // Return the nearest point on the infinite line
        return lineStartPosition + t * startToEnd;
    }

    // Clamp t to the range [0, 1] for the finite line segment
    float const clampedT = GetClampedZeroToOne(t);
    return lineStartPosition + clampedT * startToEnd; // Return the nearest point on the line segment
}

//----------------------------------------------------------------------------------------------------
Vec2 GetNearestPointOnCapsule2D(Vec2 const& point,
                                Vec2 const& capsuleStartPosition,
                                Vec2 const& capsuleEndPosition,
                                float const capsuleRadius)
{
    if (IsPointInsideCapsule(point, capsuleStartPosition, capsuleEndPosition, capsuleRadius) == true)
    {
        return point;
    }

    // Calculate the capsule's direction vector
    Vec2 const startToEnd = capsuleEndPosition - capsuleStartPosition;

    if (startToEnd == Vec2::ZERO)
    {
        Vec2 const startToPoint        = point - capsuleStartPosition;
        Vec2 const centerToPointNormal = startToPoint.GetNormalized();

        return capsuleStartPosition + centerToPointNormal * capsuleRadius;
    }

    float const capsuleLength    = startToEnd.GetLength();
    Vec2 const  startToEndNormal = startToEnd.GetNormalized();

    // Calculate the projection of the point onto the capsule's direction
    float const projectionLength        = GetProjectedLength2D(point - capsuleStartPosition, startToEndNormal);
    float const clampedProjectionLength = GetClamped(projectionLength, 0.f, capsuleLength);

    // Find the nearest point on the capsule segment
    Vec2 const nearestPointOnSegment              = capsuleStartPosition + startToEndNormal * clampedProjectionLength;
    Vec2 const nearestPointOnSegmentToPointNormal = (point - nearestPointOnSegment).GetNormalized();

    return
        nearestPointOnSegment + nearestPointOnSegmentToPointNormal * capsuleRadius;
}

//----------------------------------------------------------------------------------------------------
Vec2 GetNearestPointOnTriangle2D(Vec2 const& point,
                                 Vec2 const  triangle2Points[3])
{
    if (IsPointInsideTriangle(point, triangle2Points[0], triangle2Points[1], triangle2Points[2]))
    {
        return point;
    }

    // Start by finding the closest point on each edge of the triangle
    Vec2  nearestPoint       = triangle2Points[0];
    float minDistanceSquared = (point - nearestPoint).GetLengthSquared();

    for (int i = 0; i < 3; ++i)
    {
        // Define the endpoints of the current edge
        Vec2 edgeStart = triangle2Points[i];
        Vec2 edgeEnd   = triangle2Points[(i + 1) % 3];

        // Find the nearest point on the edge segment to referencePosition
        Vec2  edgeDirection     = edgeEnd - edgeStart;
        float edgeLengthSquared = edgeDirection.GetLengthSquared();

        if (edgeLengthSquared > 0.0f)
        {
            // Project referencePosition onto the edge (using dot product) and clamp to segment
            Vec2  startToPoint = point - edgeStart;
            float t            = DotProduct2D(startToPoint, edgeDirection) / edgeLengthSquared;
            t                  = GetClamped(t, 0.0f, 1.0f); // Clamp t to the range [0, 1] to stay within the segment

            Vec2  closestPointOnEdge = edgeStart + edgeDirection * t;
            float distanceSquared    = (point - closestPointOnEdge).GetLengthSquared();

            // Update the nearest point if this edge is closer
            if (distanceSquared < minDistanceSquared)
            {
                minDistanceSquared = distanceSquared;
                nearestPoint       = closestPointOnEdge;
            }
        }
    }

    return nearestPoint;
}

//----------------------------------------------------------------------------------------------------
Vec3 GetNearestPointOnAABB3D(Vec3 const& point, AABB3 const& aabb3)
{
    return aabb3.GetNearestPoint(point);
}

//----------------------------------------------------------------------------------------------------
Vec3 GetNearestPointOnSphere3D(Vec3 const& point, Sphere3 const& sphere)
{
    return sphere.GetNearestPoint(point);
}

Vec3 GetNearestPointOnZCylinder3D(Vec3 const& point, Cylinder3 const& cylinder3)
{
    return cylinder3.GetNearestPoint(point);
}

//-End-of-Get-Nearest-Point-Utilities-----------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
//-Start-of-Transform-Utilities-----------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
void TransformPosition2D(Vec2&       posToTransform,
                         float const uniformScale,
                         float const rotationDegrees,
                         Vec2 const& translation)	// TODO: refactor 
{
    posToTransform.x *= uniformScale;
    posToTransform.y *= uniformScale;
    posToTransform.RotateDegrees(rotationDegrees);
    posToTransform += translation;
}

//----------------------------------------------------------------------------------------------------
void TransformPosition2D(Vec2&       posToTransform,
                         Vec2 const& iBasis,
                         Vec2 const& jBasis,
                         Vec2 const& translation)
{
    Vec2 transformedPos;

    transformedPos.x = posToTransform.x * iBasis.x + posToTransform.y * jBasis.x;
    transformedPos.y = posToTransform.x * iBasis.y + posToTransform.y * jBasis.y;

    posToTransform = transformedPos + translation;
}

//----------------------------------------------------------------------------------------------------
void TransformPositionXY3D(Vec3&       posToTransform,
                           float const scaleXY,
                           float const zRotationDegrees,
                           Vec2 const& translationXY)
{
    posToTransform.x *= scaleXY;
    posToTransform.y *= scaleXY;

    Vec2 posXY = Vec2(posToTransform.x, posToTransform.y);

    posXY.RotateDegrees(zRotationDegrees);
    posXY += translationXY;

    posToTransform.x = posXY.x;
    posToTransform.y = posXY.y;
}

//----------------------------------------------------------------------------------------------------
void TransformPositionXY3D(Vec3&       posToTransform,
                           Vec2 const& iBasis,
                           Vec2 const& jBasis,
                           Vec2 const& translationXY)
{
    Vec2 transformedPos;

    transformedPos.x = posToTransform.x * iBasis.x + posToTransform.y * jBasis.x;
    transformedPos.y = posToTransform.x * iBasis.y + posToTransform.y * jBasis.y;

    posToTransform.x = transformedPos.x + translationXY.x;
    posToTransform.y = transformedPos.y + translationXY.y;
}

//-End-of-Transform-Utilities-------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
//-Start-of-Byte-Denormalization----------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
float NormalizeByte(unsigned char const byte)
{
    float const floatZeroToOne = static_cast<float>(byte) / 255.f;
    return floatZeroToOne;
}

//----------------------------------------------------------------------------------------------------
unsigned char DenormalizeByte(float const zeroToOne)
{
    if (zeroToOne <= 0.0f) return 0;

    if (zeroToOne >= 1.0f) return 255;

    return static_cast<unsigned char>(zeroToOne * 256.0f);
}

//-End-of-Byte-Denormalization------------------------------------------------------------------------

Mat44 GetBillboardMatrix(eBillboardType const billboardType, Mat44 const& targetMatrix, Vec3 const& billboardPosition, Vec2 const& billboardScale)
{
    Mat44 billboardMatrix;
    Vec3  iBasis, jBasis, kBasis;

    switch (billboardType)
    {
    case eBillboardType::FULL_FACING:
        {
            Vec3 const forwardDirection = targetMatrix.GetTranslation3D() - billboardPosition;
            iBasis                      = forwardDirection.GetNormalized();

            iBasis.GetOrthonormalBasis(iBasis, &jBasis, &kBasis);

            break;
        }
    case eBillboardType::FULL_OPPOSING:
        {
            iBasis = -targetMatrix.GetIBasis3D();
            jBasis = -targetMatrix.GetJBasis3D();
            kBasis = targetMatrix.GetKBasis3D();

            break;
        }
    case eBillboardType::WORLD_UP_FACING:
        {
            Vec3 forwardDirection = billboardPosition - targetMatrix.GetTranslation3D();
            forwardDirection.z    = 0.f;

            iBasis = forwardDirection.GetNormalized();
            kBasis = Vec3::Z_BASIS;
            jBasis = CrossProduct3D(Vec3::Z_BASIS, iBasis);

            break;
        }

    case eBillboardType::WORLD_UP_OPPOSING:
        {
            Vec3 forwardDirection = -targetMatrix.GetIBasis3D();
            forwardDirection.z    = 0.f;

            iBasis = forwardDirection.GetNormalized();
            kBasis = Vec3::Z_BASIS;
            jBasis = CrossProduct3D(Vec3::Z_BASIS, iBasis);

            break;
        }
    }

    billboardMatrix.SetIJK3D(iBasis, jBasis * billboardScale.x, kBasis * billboardScale.y);
    billboardMatrix.SetTranslation3D(billboardPosition);

    return billboardMatrix;
}
