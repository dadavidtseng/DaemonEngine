//----------------------------------------------------------------------------------------------------
// MathUtils.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Math/MathUtils.hpp"

#include <cmath>

#include "Capsule2.hpp"
// #include "LineSegment2.hpp"
#include "OBB2.hpp"
#include "Triangle2.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/Disc2.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Game/TileDefinition.hpp"

//-Start-of-Clamp-and-Lerp----------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
float GetClamped(const float value,
                 const float minValue,
                 const float maxValue)
{
    if (value > maxValue)
        return maxValue;

    if (value < minValue)
        return minValue;

    return value;
}

//----------------------------------------------------------------------------------------------------
float GetClampedZeroToOne(const float value)
{
    return GetClamped(value, 0.f, 1.f);
}

//----------------------------------------------------------------------------------------------------
float Interpolate(const float start,
                  const float end,
                  const float fractionTowardEnd)
{
    const float range = end - start;

    return start + range * fractionTowardEnd;
}

//----------------------------------------------------------------------------------------------------
float GetFractionWithinRange(const float value,
                             const float rangeStart,
                             const float rangeEnd)
{
    const float range = rangeEnd - rangeStart;

    return range == 0.f ? 0.f : (value - rangeStart) / range;
}

//----------------------------------------------------------------------------------------------------
float RangeMap(const float inValue,
               const float inStart,
               const float inEnd,
               const float outStart,
               const float outEnd)
{
    const float fractionTowardEnd = GetFractionWithinRange(inValue, inStart, inEnd);

    return Interpolate(outStart, outEnd, fractionTowardEnd);
}

//----------------------------------------------------------------------------------------------------
float RangeMapClamped(const float inValue,
                      const float inStart,
                      const float inEnd,
                      const float outStart,
                      const float outEnd)
{
    const float clampedInValue = GetClamped(inValue, inStart, inEnd);

    return RangeMap(clampedInValue, inStart, inEnd, outStart, outEnd);
}

//----------------------------------------------------------------------------------------------------
int RoundDownToInt(const float value)
{
    return static_cast<int>(floorf(value));
}

//-End-of-Clamp-and-Lerp------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
//-Start-of-Angle-Utilities---------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
float ConvertDegreesToRadians(const float degrees)
{
    return degrees * (PI / 180.f);
}

//----------------------------------------------------------------------------------------------------
float ConvertRadiansToDegrees(const float radians)
{
    return radians * (180.f / PI);
}

//----------------------------------------------------------------------------------------------------
float CosDegrees(const float degrees)
{
    return cosf(ConvertDegreesToRadians(degrees));
}

//----------------------------------------------------------------------------------------------------
float SinDegrees(const float degrees)
{
    return sinf(ConvertDegreesToRadians(degrees));
}

//----------------------------------------------------------------------------------------------------
float Atan2Degrees(const float y, const float x)
{
    return ConvertRadiansToDegrees(atan2(y, x));
}

//----------------------------------------------------------------------------------------------------
float GetShortestAngularDispDegrees(const float startDegrees,
                                    const float endDegrees)
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
float GetTurnedTowardDegrees(const float currentDegrees,
                             const float goalDegrees,
                             const float maxDeltaDegrees)
{
    float angDispDeg = GetShortestAngularDispDegrees(currentDegrees, goalDegrees);

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
    const float dotProduct = DotProduct2D(a, b);
    const float magnitudeA = a.GetLength();
    const float magnitudeB = b.GetLength();

    // If one of the vectors is zero, the angle is undefined, so return 0
    if (magnitudeA == 0.f || magnitudeB == 0.f)
    {
        return 0.f;
    }

    float cosTheta = dotProduct / (magnitudeA * magnitudeB);

    // Clamp the value to the valid range of acos (to avoid numerical issues)
    cosTheta = GetClamped(cosTheta, -1.0f, 1.0f);

    // Compute the angle in radians and convert to degrees
    const float angleRadians = acosf(cosTheta);
    const float angleDegrees = ConvertRadiansToDegrees(angleRadians);

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

//-End-of-Dot-and-Cross-------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
//-Start-of-Distance-&-Projections-Utilities----------------------------------------------------------

//----------------------------------------------------------------------------------------------------
float GetDistance2D(Vec2 const& positionA,
                    Vec2 const& positionB)
{
    const float deltaX = positionA.x - positionB.x;
    const float deltaY = positionA.y - positionB.y;

    return sqrtf(deltaX * deltaX + deltaY * deltaY);
}

//----------------------------------------------------------------------------------------------------
float GetDistanceSquared2D(Vec2 const& positionA,
                           Vec2 const& positionB)
{
    const float deltaX = positionA.x - positionB.x;
    const float deltaY = positionA.y - positionB.y;

    return
        deltaX * deltaX +
        deltaY * deltaY;
}

//----------------------------------------------------------------------------------------------------
float GetDistance3D(Vec3 const& positionA,
                    Vec3 const& positionB)
{
    const float deltaX = positionA.x - positionB.x;
    const float deltaY = positionA.y - positionB.y;
    const float deltaZ = positionA.z - positionB.z;

    return
        sqrtf(deltaX * deltaX + deltaY * deltaY + deltaZ * deltaZ);
}

//----------------------------------------------------------------------------------------------------
float GetDistanceSquared3D(Vec3 const& positionA,
                           Vec3 const& positionB)
{
    const float deltaX = positionA.x - positionB.x;
    const float deltaY = positionA.y - positionB.y;
    const float deltaZ = positionA.z - positionB.z;

    return
        deltaX * deltaX + deltaY * deltaY + deltaZ * deltaZ;
}

//----------------------------------------------------------------------------------------------------
float GetDistanceXY3D(Vec3 const& positionA,
                      Vec3 const& positionB)
{
    const float deltaX = positionA.x - positionB.x;
    const float deltaY = positionA.y - positionB.y;

    return sqrtf(deltaX * deltaX + deltaY * deltaY);
}

//----------------------------------------------------------------------------------------------------
float GetDistanceXYSquared3D(Vec3 const& positionA,
                             Vec3 const& positionB)
{
    const float deltaX = positionA.x - positionB.x;
    const float deltaY = positionA.y - positionB.y;

    return
        deltaX * deltaX +
        deltaY * deltaY;
}

//----------------------------------------------------------------------------------------------------
int GetTaxicabDistance2D(IntVec2 const& pointA,
                         IntVec2 const& pointB)
{
    const int deltaX = abs(pointA.x - pointB.x);
    const int deltaY = abs(pointA.y - pointB.y);

    return deltaX + deltaY;
}

//----------------------------------------------------------------------------------------------------
float GetProjectedLength2D(Vec2 const& vectorToProject,
                           Vec2 const& vectorToProjectOnto)
{
    const float vectorToProjectOntoLength = vectorToProjectOnto.GetLength();

    if (vectorToProjectOntoLength == 0.f)
    {
        return 0.f;
    }

    const float dotProduct      = DotProduct2D(vectorToProject, vectorToProjectOnto);
    const float projectedLength = dotProduct / vectorToProjectOntoLength;

    return projectedLength;
}

//----------------------------------------------------------------------------------------------------
Vec2 GetProjectedOnto2D(Vec2 const& vectorToProject,
                        Vec2 const& vectorToProjectOnto)
{
    const float vectorToProjectOntoLengthSquared = vectorToProjectOnto.GetLengthSquared();

    if (vectorToProjectOntoLengthSquared == 0.f)
    {
        return Vec2(0.f, 0.f);
    }

    const float dotProduct      = DotProduct2D(vectorToProject, vectorToProjectOnto);
    const float projectionScale = dotProduct / vectorToProjectOntoLengthSquared;

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
                      const float radiusA,
                      Vec3 const& centerB,
                      const float radiusB)
{
    return
        GetDistance3D(centerA, centerB) < (radiusA + radiusB);
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
                         const float discRadius)
{
    const float distSquared       = GetDistanceSquared2D(point, discCenter);
    const float discRadiusSquared = discRadius * discRadius;

    return distSquared <= discRadiusSquared;
}
//----------------------------------------------------------------------------------------------------
bool IsPointInsideDisc2D(Vec2 const& point, Disc2 const& disc)
{
    return disc.IsPointInside(point);
}

//----------------------------------------------------------------------------------------------------
bool IsPointInsideAABB2D(Vec2 const& point, AABB2 const& box)
{
    return box.IsPointInside(point);
}

//----------------------------------------------------------------------------------------------------
bool IsPointInsideOBB2D(Vec2 const& point, OBB2 const& box)
{
    return box.IsPointInside(point);
}

//----------------------------------------------------------------------------------------------------
bool IsPointInsideCapsule(Vec2 const& point, Capsule2 const& capsule)
{
    return capsule.IsPointInside(point);
}

//----------------------------------------------------------------------------------------------------
bool IsPointInsideTriangle(Vec2 const& point, Triangle2 const& triangle)
{
    return triangle.IsPointInside(point);
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
Vec2 GetNearestPointOnDisc2D(Vec2 const& referencePosition,
                             Vec2 const& discCenter,
                             const float discRadius)
{
    const float distSquared       = GetDistanceSquared2D(referencePosition, discCenter);
    const float discRadiusSquared = discRadius * discRadius;

    if (distSquared <= discRadiusSquared)
        return referencePosition;

    Vec2 discCenterToReferencePos = referencePosition - discCenter;

    discCenterToReferencePos.SetLength(discRadius);

    return discCenter + discCenterToReferencePos;
}

//----------------------------------------------------------------------------------------------------
Vec2 GetNearestPointOnDisc2D(const Vec2&  referencePosition,
                             const Disc2& disc)
{
    return disc.GetNearestPoint(referencePosition);
}

//----------------------------------------------------------------------------------------------------
Vec2 GetNearestPointOnAABB2D(Vec2 const&  referencePos,
                             AABB2 const& aabbBox)
{
    return aabbBox.GetNearestPoint(referencePos);
}

//----------------------------------------------------------------------------------------------------
Vec2 GetNearestPointOnOBB2D(Vec2 const& referencePos,
                            OBB2 const& obbBox)
{
    return obbBox.GetNearestPoint(referencePos);
}

//----------------------------------------------------------------------------------------------------
Vec2 GetNearestPointOnInfiniteLine2D(Vec2 const&         referencePos,
                                     LineSegment2 const& infiniteLine)
{
    UNUSED(referencePos)
    UNUSED(infiniteLine)

    return {};
}

//----------------------------------------------------------------------------------------------------
Vec2 GetNearestPointOnLineSegment2D(Vec2 const&         referencePos,
                                    LineSegment2 const& lineSegment)
{
    UNUSED(referencePos)
    UNUSED(lineSegment)

    return {};
}

//----------------------------------------------------------------------------------------------------
Vec2 GetNearestPointOnCapsule2D(Vec2 const&     referencePos,
                                Capsule2 const& capsule)
{
    return capsule.GetNearestPoint(referencePos);
}

//----------------------------------------------------------------------------------------------------
Vec2 GetNearestPointOnTriangle2D(Vec2 const&      referencePos,
                                 Triangle2 const& triangle)
{
    return triangle.GetNearestPoint(referencePos);
}

//-End-of-Get-Nearest-Point-Utilities-----------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
//-Start-of-Transform-Utilities-----------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
void TransformPosition2D(Vec2&       posToTransform,
                         const float uniformScale,
                         const float rotationDegrees,
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
                           const float scaleXY,
                           const float zRotationDegrees,
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

// TODO: normalize and denormalize
float NormalizeByte(unsigned char const byte)
{
    float const floatZeroToOne = static_cast<float>(byte) / 255.0f;
    return floatZeroToOne;
}

unsigned char DenormalizeByte(float const zeroToOne)
{
    return static_cast<unsigned char>(zeroToOne * 255.f);
}
