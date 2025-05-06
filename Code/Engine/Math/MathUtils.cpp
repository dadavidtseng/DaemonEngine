//----------------------------------------------------------------------------------------------------
// MathUtils.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Math/MathUtils.hpp"

#include <cmath>

#include "OBB3.hpp"
#include "Plane3.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/Mat44.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/Vec4.hpp"

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
float DotProduct3D(Vec3 const& a,
                   Vec3 const& b)
{
    return
        a.x * b.x +
        a.y * b.y +
        a.z * b.z;
}

//----------------------------------------------------------------------------------------------------
float DotProduct4D(Vec4 const& a,
                   Vec4 const& b)
{
    return
        a.x * b.x +
        a.y * b.y +
        a.z * b.z +
        a.w * b.w;
}

//----------------------------------------------------------------------------------------------------
float CrossProduct2D(Vec2 const& a,
                     Vec2 const& b)
{
    return
        a.x * b.y -
        a.y * b.x;
}

//----------------------------------------------------------------------------------------------------
Vec3 CrossProduct3D(Vec3 const& a,
                    Vec3 const& b)
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
float GetProjectedLength3D(Vec3 const& vectorToProject,
                           Vec3 const& vectorToProjectOnto)
{
    float const vectorToProjectOntoLength = vectorToProjectOnto.GetLength();

    if (vectorToProjectOntoLength == 0.f)
    {
        return 0.f;
    }

    float const dotProduct      = DotProduct3D(vectorToProject, vectorToProjectOnto);
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
bool DoDiscsOverlap2D(Vec2 const& centerA,
                      float const radiusA,
                      Vec2 const& centerB,
                      float const radiusB)
{
    return
        GetDistanceSquared2D(centerA, centerB) <= (radiusA + radiusB) * (radiusA + radiusB);
}

//----------------------------------------------------------------------------------------------------
bool DoAABB2sOverlap2D(AABB2 const& boxA, AABB2 const& boxB)
{
    float const aMinX = boxA.m_mins.x;
    float const aMaxX = boxA.m_maxs.x;
    float const aMinY = boxA.m_mins.y;
    float const aMaxY = boxA.m_maxs.y;
    float const bMinX = boxB.m_mins.x;
    float const bMaxX = boxB.m_maxs.x;
    float const bMinY = boxB.m_mins.y;
    float const bMaxY = boxB.m_maxs.y;

    if (aMaxX > bMinX &&
        bMaxX > aMinX &&
        aMaxY > bMinY &&
        bMaxY > aMinY)
    {
        return true;
    }

    return false;
}

//----------------------------------------------------------------------------------------------------
bool DoDiscAndAABB2Overlap2D(Vec2 const&  discCenter,
                             float const  discRadius,
                             AABB2 const& aabb2)
{
    Vec2 const nearestPoint = GetNearestPointOnAABB2D(discCenter, aabb2.m_mins, aabb2.m_maxs);

    return IsPointInsideDisc2D(nearestPoint, discCenter, discRadius);
}

//----------------------------------------------------------------------------------------------------
bool DoSpheresOverlap3D(Vec3 const& centerA,
                        float const radiusA,
                        Vec3 const& centerB,
                        float const radiusB)
{
    return
        GetDistanceSquared3D(centerA, centerB) <= (radiusA + radiusB) * (radiusA + radiusB);
}

//----------------------------------------------------------------------------------------------------
bool DoSphereAndAABB3Overlap3D(Vec3 const&  sphereCenter,
                               float const  sphereRadius,
                               AABB3 const& aabb3)
{
    Vec3 const nearestPoint = GetNearestPointOnAABB3D(sphereCenter, aabb3);

    return IsPointInsideSphere3D(nearestPoint, sphereCenter, sphereRadius);
}

//----------------------------------------------------------------------------------------------------
bool DoSphereAndZCylinderOverlap3D(Vec3 const&       sphereCenter,
                                   float const       sphereRadius,
                                   Vec2 const&       cylinderCenterXY,
                                   float const       cylinderRadius,
                                   FloatRange const& cylinderMinMaxZ)
{
    Vec3 const cylinderStartPosition = Vec3(cylinderCenterXY.x, cylinderCenterXY.y, cylinderMinMaxZ.m_min);
    Vec3 const cylinderEndPosition   = Vec3(cylinderCenterXY.x, cylinderCenterXY.y, cylinderMinMaxZ.m_max);

    Vec3 const nearestPoint = GetNearestPointOnZCylinder3D(sphereCenter, cylinderStartPosition, cylinderEndPosition, cylinderRadius);
    return IsPointInsideSphere3D(nearestPoint, sphereCenter, sphereRadius);
}

//----------------------------------------------------------------------------------------------------
bool DoAABB3sOverlap3D(AABB3 const& first, AABB3 const& second)
{
    float const aMinX = first.m_mins.x;
    float const aMaxX = first.m_maxs.x;
    float const aMinY = first.m_mins.y;
    float const aMaxY = first.m_maxs.y;
    float const aMinZ = first.m_mins.z;
    float const aMaxZ = first.m_maxs.z;
    float const bMinX = second.m_mins.x;
    float const bMaxX = second.m_maxs.x;
    float const bMinY = second.m_mins.y;
    float const bMaxY = second.m_maxs.y;
    float const bMinZ = second.m_mins.z;
    float const bMaxZ = second.m_maxs.z;

    if (aMaxX > bMinX &&
        bMaxX > aMinX &&
        aMaxY > bMinY &&
        bMaxY > aMinY &&
        aMaxZ > bMinZ &&
        bMaxZ > aMinZ)
    {
        return true;
    }

    return false;
}

//----------------------------------------------------------------------------------------------------
bool DoAABB3AndZCylinderOverlap3D(AABB3 const&      aabb3,
                                  Vec2 const&       cylinderCenterXY,
                                  float const       cylinderRadius,
                                  FloatRange const& cylinderMinMaxZ)
{
    if (cylinderMinMaxZ.m_max > aabb3.m_mins.z &&
        cylinderMinMaxZ.m_min < aabb3.m_maxs.z &&
        DoDiscAndAABB2Overlap2D(cylinderCenterXY, cylinderRadius, AABB2(Vec2(aabb3.m_mins.x, aabb3.m_mins.y), Vec2(aabb3.m_maxs.x, aabb3.m_maxs.y))))
    {
        return true;
    }

    return false;
}

//----------------------------------------------------------------------------------------------------
bool DoZCylindersOverlap3D(Vec2 const&       cylinder1CenterXY,
                           float const       cylinder1Radius,
                           FloatRange const& cylinder1MinMaxZ,
                           Vec2 const&       cylinder2CenterXY,
                           float const       cylinder2Radius,
                           FloatRange const& cylinder2MinMaxZ)
{
    if (DoDiscsOverlap2D(cylinder1CenterXY, cylinder1Radius, cylinder2CenterXY, cylinder2Radius) &&
        cylinder1MinMaxZ.IsOverlappingWith(cylinder2MinMaxZ))
    {
        return true;
    }

    return false;
}

//----------------------------------------------------------------------------------------------------
bool DoSphereAndOBB3Overlap3D(Vec3 const& sphereCenter,
                              float       sphereRadius,
                              OBB3 const& obb3)
{
    Vec3 nearestPoint = GetNearestPointOnOBB3D(sphereCenter, obb3);
    return IsPointInsideSphere3D(nearestPoint, sphereCenter, sphereRadius);
}

//----------------------------------------------------------------------------------------------------
bool DoSphereAndPlaneOverlap3D(Vec3 const&   sphereCenter,
                               float         sphereRadius,
                               Plane3 const& plane3)
{
    float altitude = plane3.GetAltitudeOfPoint(sphereCenter);
    return sphereRadius > abs(altitude);
}

//----------------------------------------------------------------------------------------------------
bool DoAABB3AndPlane3Overlap3D(AABB3 const&  aabb3,
                               Plane3 const& plane3)
{
    // reference: unreal engine source code
    /*
    * If the point positively farthest to the plane is below the plane
    * or negatively farthest to the plane is above the plane, then there is no intersection
    * So get the max and min point
    */
    Vec3   planeRelativeMin, planeRelativeMax;
    float* planeRelativeMinPtr = (float*)&planeRelativeMin;
    float* planeRelativeMaxPtr = (float*)&planeRelativeMax;

    float const* aabbMinPtr     = (float*)&aabb3.m_mins;
    float const* aabbMaxPtr     = (float*)&aabb3.m_maxs;
    float const* planeNormalPtr = (float*)&plane3.m_normal;

    for (int i = 0; i < 3; i++)
    {
        if (planeNormalPtr[i] > 0.f)
        {
            planeRelativeMinPtr[i] = aabbMinPtr[i];
            planeRelativeMaxPtr[i] = aabbMaxPtr[i];
        }
        else
        {
            planeRelativeMinPtr[i] = aabbMaxPtr[i];
            planeRelativeMaxPtr[i] = aabbMinPtr[i];
        }
    }

    float distanceMax = plane3.GetAltitudeOfPoint(planeRelativeMax);
    float distanceMin = plane3.GetAltitudeOfPoint(planeRelativeMin);
    if (distanceMax <= 0.f || distanceMin >= 0.f)
    {
        return false;
    }
    return true;
}

//----------------------------------------------------------------------------------------------------
bool DoOBB3AndPlane3Overlap3D(OBB3 const&   obb3,
                              Plane3 const& plane3)
{
    Plane3 tempPlane(plane3.m_normal, plane3.m_distanceFromOrigin);
    tempPlane.Translate(-obb3.m_center);
    tempPlane.m_normal = Vec3(DotProduct3D(plane3.m_normal, obb3.m_iBasis), DotProduct3D(plane3.m_normal, obb3.m_jBasis), DotProduct3D(plane3.m_normal, obb3.m_kBasis));

    AABB3 localBox = AABB3(-obb3.m_halfDimensions, obb3.m_halfDimensions);
    return DoAABB3AndPlane3Overlap3D(localBox, tempPlane);
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
    Vec2 const  aCenterTobCenter        = bCenter - aCenter;
    float const twoDiscRadiusSum        = aRadius + bRadius;
    float const twoDiscRadiusSumSquared = twoDiscRadiusSum * twoDiscRadiusSum;

    if (aCenterTobCenter.GetLengthSquared() >= twoDiscRadiusSumSquared)
    {
        return false;
    }

    float const overlapDist   = twoDiscRadiusSum - aCenterTobCenter.GetLength();
    Vec2 const  correctionVec = aCenterTobCenter.GetNormalized() * (overlapDist / 2.f);

    aCenter = aCenter - correctionVec;
    bCenter = bCenter + correctionVec;

    return true;
}

//----------------------------------------------------------------------------------------------------
bool PushDiscOutOfAABB2D(Vec2&        mobileDiscCenter,
                         const float  discRadius,
                         AABB2 const& fixedBox)
{
    Vec2 const nearestPoint = fixedBox.GetNearestPoint(mobileDiscCenter);

    return PushDiscOutOfPoint2D(mobileDiscCenter, discRadius, nearestPoint);
}

//----------------------------------------------------------------------------------------------------
bool BounceDiscOutOfEachOther2D(Vec2&       aCenter,
                                float const aRadius,
                                Vec2&       aVelocity,
                                float const aElasticity,
                                Vec2&       bCenter,
                                float const bRadius,
                                Vec2&       bVelocity,
                                float const bElasticity)
{
    Vec2        normalAtoB    = bCenter - aCenter;
    float const squaredLength = normalAtoB.GetLengthSquared();

    // Do discs overlap
    if (squaredLength >= (aRadius + bRadius) * (aRadius + bRadius) || squaredLength == 0.f)
    {
        return false;
    }

    float length         = sqrtf(squaredLength);
    float inversedLength = 1.f / length;
    normalAtoB           = normalAtoB * inversedLength;

    float dotANormal = DotProduct2D(normalAtoB, aVelocity);
    float dotBNormal = DotProduct2D(normalAtoB, bVelocity);

    // Push disc out of each other
    Vec2 nearestPointFromAonB = bCenter - bRadius * normalAtoB;
    Vec2 nearestPointFromBonA = aCenter + aRadius * normalAtoB;
    Vec2 difference           = nearestPointFromAonB - nearestPointFromBonA;
    aCenter += difference * 0.5f;
    bCenter -= difference * 0.5f;

    if (dotANormal <= dotBNormal)
    {
        return false;
    }
    // accept
    Vec2 velocityAgreeOnNormalDirA = dotANormal * normalAtoB;
    Vec2 velocityAgreeOnNormalDirB = dotBNormal * normalAtoB;
    Vec2 velocityIndependentA      = aVelocity - velocityAgreeOnNormalDirA;
    Vec2 velocityIndependentB      = bVelocity - velocityAgreeOnNormalDirB;

    // Bounce the velocity of disc A
    aVelocity = velocityIndependentA + aElasticity * bElasticity * velocityAgreeOnNormalDirB;

    // Bounce the velocity of disc B
    bVelocity = velocityIndependentB + aElasticity * bElasticity * velocityAgreeOnNormalDirA;

    return true;
}

//----------------------------------------------------------------------------------------------------
bool BounceDiscOutOfFixedPoint2D(Vec2&       discCenter,
                                 float const discRadius,
                                 Vec2&       discVelocity,
                                 float const discElasticity,
                                 Vec2 const& fixedPoint,
                                 float const pointElasticity)
{
    Vec2  normal        = discCenter - fixedPoint;
    float squaredLength = normal.GetLengthSquared();

    // Do disc overlap the point?
    if (squaredLength >= discRadius * discRadius || squaredLength == 0.f)
    {
        return false;
    }

    float length         = sqrtf(squaredLength);
    float inversedLength = 1.f / length;
    normal               = normal * inversedLength;

    // Push disc out of the fixed point
    discCenter += normal * (discRadius - length);

    // velocity not agree?
    if (DotProduct2D(normal, discVelocity) > 0.f)
    {
        return false;
    }

    // Bounce the velocity
    Vec2 velocityAgreeOnNormalDir = DotProduct2D(normal, discVelocity) * normal;
    discVelocity                  = discVelocity - velocityAgreeOnNormalDir - discElasticity * pointElasticity * velocityAgreeOnNormalDir;

    return true;
}

//----------------------------------------------------------------------------------------------------
bool BounceDiscOutOfFixedDisc2D(Vec2&       mobileCenter,
                                float const mobileRadius,
                                Vec2&       mobileVelocity,
                                float const mobileElasticity,
                                Vec2 const& fixedCenter,
                                float const fixedRadius,
                                float const fixedElasticity)
{
    Vec2 const point = GetNearestPointOnDisc2D(mobileCenter, fixedCenter, fixedRadius);

    return BounceDiscOutOfFixedPoint2D(mobileCenter, mobileRadius, mobileVelocity, mobileElasticity, point, fixedElasticity);
}

//----------------------------------------------------------------------------------------------------
bool BounceDiscOutOfFixedOBB2D(Vec2&       mobileCenter,
                               float const mobileRadius,
                               Vec2&       mobileVelocity,
                               float const mobileElasticity,
                               Vec2 const& obbCenter,
                               Vec2 const& obb2IBasisNormal,
                               Vec2 const& obb2HalfDimensions,
                               float const fixedElasticity)
{
    Vec2 const point = GetNearestPointOnOBB2D(mobileCenter, obbCenter, obb2IBasisNormal, obb2HalfDimensions);

    return BounceDiscOutOfFixedPoint2D(mobileCenter, mobileRadius, mobileVelocity, mobileElasticity, point, fixedElasticity);
}

//----------------------------------------------------------------------------------------------------
bool BounceDiscOutOfFixedCapsule2D(Vec2&       mobileCenter,
                                   float const mobileRadius,
                                   Vec2&       mobileVelocity,
                                   float const mobileElasticity,
                                   Vec2 const& fixedBoneStart,
                                   Vec2 const& fixedBoneEnd,
                                   float const fixedRadius,
                                   float const fixedElasticity)
{
    Vec2 const point = GetNearestPointOnCapsule2D(mobileCenter, fixedBoneStart, fixedBoneEnd, fixedRadius);

    return BounceDiscOutOfFixedPoint2D(mobileCenter, mobileRadius, mobileVelocity, mobileElasticity, point, fixedElasticity);
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
bool IsPointInsideTriangle2D(Vec2 const& point,
                             Vec2 const& ccw1,
                             Vec2 const& ccw2,
                             Vec2 const& ccw3)
{
    Vec2 const& A = ccw1;
    Vec2 const& B = ccw2;
    Vec2 const& C = ccw3;

    Vec2 const AB = B - A;
    Vec2 const BC = C - B;
    Vec2 const CA = A - C;
    Vec2 const AP = point - A;
    Vec2 const BP = point - B;
    Vec2 const CP = point - C;

    float const cross1 = CrossProduct2D(AB, AP);
    float const cross2 = CrossProduct2D(BC, BP);
    float const cross3 = CrossProduct2D(CA, CP);

    return
        (cross1 >= 0 && cross2 >= 0 && cross3 >= 0) ||
        (cross1 <= 0 && cross2 <= 0 && cross3 <= 0);
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
bool IsPointInsideCapsule2D(Vec2 const& point,
                            Vec2 const& capsuleStartPosition,
                            Vec2 const& capsuleEndPosition,
                            float const capsuleRadius)
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

//----------------------------------------------------------------------------------------------------
bool IsPointInsideSphere3D(Vec3 const& point,
                           Vec3 const& sphereCenter,
                           float const sphereRadius)
{
    return GetDistanceSquared3D(point, sphereCenter) < sphereRadius * sphereRadius;
}

//----------------------------------------------------------------------------------------------------
/// @brief Checks whether a point is inside the given 3D ZCylinder.
/// @param point The reference point.
/// @param cylinderStartPosition
/// @param cylinderEndPosition
/// @param cylinderRadius
/// @return True if the point is inside the ZCylinder3D; false otherwise.
bool IsPointInsideZCylinder3D(Vec3 const& point,
                              Vec3 const& cylinderStartPosition,
                              Vec3 const& cylinderEndPosition,
                              float const cylinderRadius)
{
    Vec3 const cylinderCenterPosition   = (cylinderStartPosition + cylinderEndPosition) * 0.5f;
    Vec2 const cylinderCenterPositionXY = Vec2(cylinderCenterPosition.x, cylinderCenterPosition.y);
    Vec2 const pointXY                  = Vec2(point.x, point.y);

    return
        IsPointInsideDisc2D(pointXY, cylinderCenterPositionXY, cylinderRadius) &&
        point.z > cylinderStartPosition.z &&
        point.z < cylinderEndPosition.z;
}

bool IsPointInsideAABB3D(Vec3 const& point,
                         Vec3 const& aabb3Mins,
                         Vec3 const& aabb3Maxs)
{
    return
        point.x >= aabb3Mins.x &&
        point.x <= aabb3Maxs.x &&
        point.y >= aabb3Mins.y &&
        point.y <= aabb3Maxs.y &&
        point.z >= aabb3Mins.z &&
        point.z <= aabb3Maxs.z;
}

//----------------------------------------------------------------------------------------------------
/// @brief Checks whether a point is inside the given 3D oriented bounding box (OBB).
/// @param point The 3D position to test.
/// @param obb3 The oriented bounding box to check against.
/// @return True if the point is inside the OBB; false otherwise.
bool IsPointInsideOBB3D(Vec3 const& point,
                        OBB3 const& obb3)
{
    return obb3.IsPointInside(point);
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
    if (IsPointInsideDisc2D(point, discCenter, discRadius))
    {
        return point;
    }

    // 2. Calculate the nearest point on the disc.
    Vec2 const centerToPoint       = point - discCenter;
    Vec2 const centerToPointNormal = centerToPoint.GetNormalized();

    return discCenter + centerToPointNormal * discRadius;
}

//----------------------------------------------------------------------------------------------------
Vec2 GetNearestPointOnLineSegment2D(Vec2 const& point,
                                    Vec2 const& lineStartPosition,
                                    Vec2 const& lineEndPosition,
                                    bool const  isLineInfinite)
{
    // 1. Calculate startToEnd direction on the line and its lengthSquared.
    Vec2 const  startToEnd              = lineEndPosition - lineStartPosition;
    float const startToEndLengthSquared = startToEnd.GetLengthSquared();

    // 2. If the line's lengthSquared is zero, return the startPosition of the line.
    if (startToEndLengthSquared == 0.f)
    {
        return lineStartPosition;
    }

    // 3. Project the point onto the infinite line defined by startToEnd and calculate its proportion t.
    Vec2 const  startToPoint = point - lineStartPosition;
    float const t            = DotProduct2D(startToPoint, startToEnd) / startToEndLengthSquared;

    // 4. If the line is infinite, return the nearest point on the infinite line.
    if (isLineInfinite == true)
    {
        return lineStartPosition + t * startToEnd;
    }

    // 5. If the line is not infinite, clamp t to the range [0, 1] to stay within the line segment,
    // and return the nearest point on the line segment.
    float const clampedT = GetClampedZeroToOne(t);
    return lineStartPosition + clampedT * startToEnd;
}

//----------------------------------------------------------------------------------------------------
Vec2 GetNearestPointOnTriangle2D(Vec2 const& point,
                                 Vec2 const  triangle2Points[3])
{
    // 1. If the point is inside the triangle, return the point itself.
    if (IsPointInsideTriangle2D(point, triangle2Points[0], triangle2Points[1], triangle2Points[2]))
    {
        return point;
    }

    // 2. Set the nearestPoint and minLengthSquared on one of the triangle's corner.
    Vec2  nearestPoint     = triangle2Points[0];
    float minLengthSquared = (point - nearestPoint).GetLengthSquared();

    for (int edgeIndex = 0; edgeIndex < 3; ++edgeIndex)
    {
        // 3. Define the edgeStart and edgeEnd.
        Vec2 edgeStartPosition = triangle2Points[edgeIndex];
        Vec2 edgeEndPosition   = triangle2Points[(edgeIndex + 1) % 3];

        // 4. Calculate startToEnd direction on the edge and its lengthSquared.
        Vec2        edgeStartToEnd    = edgeEndPosition - edgeStartPosition;
        float const edgeLengthSquared = edgeStartToEnd.GetLengthSquared();

        // 5. If the edge's lengthSquared is zero, continue to the next edge.
        if (edgeLengthSquared <= 0.f)
        {
            return point;
        }

        // 6. Project the point onto the infinite line defined by startToEnd, calculate its proportion t,
        // clamp t to the range [0, 1] to stay within the line segment.
        Vec2  startToPoint = point - edgeStartPosition;
        float t            = DotProduct2D(startToPoint, edgeStartToEnd) / edgeLengthSquared;
        t                  = GetClampedZeroToOne(t);

        // 7. Calculate the nearest point on the line segment.
        Vec2        closestPointOnEdge = edgeStartPosition + edgeStartToEnd * t;
        float const distanceSquared    = (point - closestPointOnEdge).GetLengthSquared();

        // 8. Update the nearest point if this edge is closer.
        if (distanceSquared < minLengthSquared)
        {
            minLengthSquared = distanceSquared;
            nearestPoint     = closestPointOnEdge;
        }
    }

    return nearestPoint;
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

    localPointX = GetClamped(localPointX, -obb2HalfDimensions.x, obb2HalfDimensions.x);
    localPointY = GetClamped(localPointY, -obb2HalfDimensions.y, obb2HalfDimensions.y);

    Vec2 worldPosition = obb2Center;
    worldPosition += obb2IBasisNormal * localPointX;
    worldPosition += Vec2(-obb2IBasisNormal.y, obb2IBasisNormal.x) * localPointY;

    return worldPosition;
}

//----------------------------------------------------------------------------------------------------
Vec2 GetNearestPointOnCapsule2D(Vec2 const& point,
                                Vec2 const& capsuleStartPosition,
                                Vec2 const& capsuleEndPosition,
                                float const capsuleRadius)
{
    if (IsPointInsideCapsule2D(point, capsuleStartPosition, capsuleEndPosition, capsuleRadius) == true)
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
Vec3 GetNearestPointOnAABB3D(Vec3 const& point, AABB3 const& aabb3)
{
    return aabb3.GetNearestPoint(point);
}

//----------------------------------------------------------------------------------------------------
Vec3 GetNearestPointOnSphere3D(Vec3 const& point, Vec3 const& sphereCenter, float const sphereRadius)
{
    if (IsPointInsideSphere3D(point, sphereCenter, sphereRadius) == true)
    {
        return point;
    }

    Vec3 const centerToPoint = point - sphereCenter;

    return sphereCenter + centerToPoint.GetClamped(sphereRadius);
}

//----------------------------------------------------------------------------------------------------
Vec3 GetNearestPointOnZCylinder3D(Vec3 const& point,
                                  Vec3 const& cylinderStartPosition,
                                  Vec3 const& cylinderEndPosition,
                                  float const cylinderRadius)
{
    Vec3 const cylinderCenterPosition   = (cylinderStartPosition + cylinderEndPosition) * 0.5f;
    Vec2 const cylinderCenterPositionXY = Vec2(cylinderCenterPosition.x, cylinderCenterPosition.y);
    Vec2 const pointXY                  = Vec2(point.x, point.y);

    if (IsPointInsideZCylinder3D(point, cylinderStartPosition, cylinderEndPosition, cylinderRadius) == true)
    {
        return point;
    }

    Vec2 const nearestPointOnDisc = GetNearestPointOnDisc2D(pointXY, cylinderCenterPositionXY, cylinderRadius);
    Vec3       nearestPoint       = Vec3(nearestPointOnDisc.x, nearestPointOnDisc.y, 0.f);

    nearestPoint.z = GetClamped(point.z, cylinderStartPosition.z, cylinderEndPosition.z);

    return nearestPoint;
}

//----------------------------------------------------------------------------------------------------
Vec3 GetNearestPointOnPlane3D(Vec3 const& point, Plane3 const& plane)
{
    return plane.GetNearestPoint(point);
}

//----------------------------------------------------------------------------------------------------
Vec3 GetNearestPointOnOBB3D(Vec3 const& point, OBB3 const& obb3)
{
    return obb3.GetNearestPoint(point);
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

    case eBillboardType::NONE:
        {
            Mat44 const identity;

            return identity;
        }

    case eBillboardType::COUNT:
        {
            ERROR_AND_DIE("eBillboardType::COUNT is invalid!")
        }
    }

    billboardMatrix.SetIJK3D(iBasis, jBasis * billboardScale.x, kBasis * billboardScale.y);
    billboardMatrix.SetTranslation3D(billboardPosition);

    return billboardMatrix;
}

//----------------------------------------------------------------------------------------------------
// A, B, C, D are the Cubic (3rd order) Bezier control points (A is the start pos, and D is the end pos),
// and t is the parametric in [0,1]
float ComputeCubicBezier1D(float const A,
                           float const B,
                           float const C,
                           float const D,
                           float const t)
{
    float const AB  = Interpolate(A, B, t);
    float const BC  = Interpolate(B, C, t);
    float const CD  = Interpolate(C, D, t);
    float const ABC = Interpolate(AB, BC, t);
    float const BCD = Interpolate(BC, CD, t);

    return Interpolate(ABC, BCD, t);
}

//----------------------------------------------------------------------------------------------------
// A, B, C, D, E, F are the Quintic (5th order) Bezier control points (A is the start, and F is the end),
// and t is the parametric in [0,1].
float ComputeQuinticBezier1D(float const A,
                             float const B,
                             float const C,
                             float const D,
                             float const E,
                             float const F,
                             float const t)
{
    float const AB = Interpolate(A, B, t);
    float const BC = Interpolate(B, C, t);
    float const CD = Interpolate(C, D, t);
    float const DE = Interpolate(D, E, t);
    float const EF = Interpolate(E, F, t);

    float const AC = Interpolate(AB, BC, t);
    float const BD = Interpolate(BC, CD, t);
    float const CE = Interpolate(CD, DE, t);
    float const DF = Interpolate(DE, EF, t);

    float const AD = Interpolate(AC, BD, t);
    float const BE = Interpolate(BD, CE, t);
    float const CF = Interpolate(CE, DF, t);

    float const AE = Interpolate(AD, BE, t);
    float const BF = Interpolate(BE, CF, t);

    return Interpolate(AE, BF, t);
}

//----------------------------------------------------------------------------------------------------
float SmoothStart2(float const t)
{
    return t * t;
}

//----------------------------------------------------------------------------------------------------
float SmoothStart3(float const t)
{
    return t * t * t;
}

//----------------------------------------------------------------------------------------------------
float SmoothStart4(float const t)
{
    return t * t * t * t;
}

//----------------------------------------------------------------------------------------------------
float SmoothStart5(float const t)
{
    return t * t * t * t * t;
}

//----------------------------------------------------------------------------------------------------
float SmoothStart6(float const t)
{
    return t * t * t * t * t * t;
}

//----------------------------------------------------------------------------------------------------
float SmoothStartN(float const t,
                   int const   n)
{
    float result = 1.f;

    for (int i = 0; i < n; ++i)
    {
        result *= t;
    }

    return result;
}

//----------------------------------------------------------------------------------------------------
float SmoothStop2(float const t)
{
    float const inverseT = 1.f - t;

    return 1.f - inverseT * inverseT;
}

//----------------------------------------------------------------------------------------------------
float SmoothStop3(float const t)
{
    float const inverseT = 1.f - t;

    return 1.f - inverseT * inverseT * inverseT;
}

//----------------------------------------------------------------------------------------------------
float SmoothStop4(float const t)
{
    float const inverseT = 1.f - t;

    return 1.f - inverseT * inverseT * inverseT * inverseT;
}

//----------------------------------------------------------------------------------------------------
float SmoothStop5(float const t)
{
    float const inverseT = 1.f - t;

    return 1.f - inverseT * inverseT * inverseT * inverseT * inverseT;
}

//----------------------------------------------------------------------------------------------------
float SmoothStop6(float const t)
{
    float const inverseT = 1.f - t;

    return 1.f - inverseT * inverseT * inverseT * inverseT * inverseT * inverseT;
}

//----------------------------------------------------------------------------------------------------
float SmoothStopN(float const t,
                  int const   n)
{
    float const inverseT = 1.f - t;
    float       result   = 1.f;

    for (int i = 0; i < n; ++i)
    {
        result *= inverseT;
    }

    return 1.f - result;
}

float SmoothStep3(float const t)
{
    return ComputeCubicBezier1D(0.f, 0.f, 1.f, 1.f, t);
}

float SmoothStep5(float const t)
{
    return ComputeQuinticBezier1D(0.f, 0.f, 0.f, 1.f, 1.f, 1.f, t);
}

float Hesitate3(float const t)
{
    return ComputeCubicBezier1D(0.f, 1.f, 0.f, 1.f, t);
}

float Hesitate5(float const t)
{
    return ComputeQuinticBezier1D(0.f, 1.f, 0.f, 1.f, 0.f, 1.f, t);
}

//----------------------------------------------------------------------------------------------------

float CustomFunkyEasingFunction(float const t)
{
    if (t < 0.8f) return SmoothStart3(t);
    return 1.f - 0.1f * (1.f - t);
}
