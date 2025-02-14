//----------------------------------------------------------------------------------------------------
// MathUtils.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include "Vec4.hpp"

//----------------------------------------------------------------------------------------------------
struct AABB2;
struct Capsule2;
struct Disc2;
struct IntVec2;
struct LineSegment2;
struct OBB2;
struct Triangle2;
struct Vec2;
struct Vec3;

//----------------------------------------------------------------------------------------------------
constexpr float PI      = 3.14159265358979323846f;
constexpr float EPSILON = 1e-5f;	// Define a small tolerance value

//-Start-of-Clamp-and-Lerp----------------------------------------------------------------------------

template <typename T>
T GetClamped(T const value, T const minValue, T const maxValue)
{
    if (value > maxValue)
        return maxValue;

    if (value < minValue)
        return minValue;

    return value;
}

float GetClampedZeroToOne(float value);
float Interpolate(float start, float end, float fractionTowardEnd);
float GetFractionWithinRange(float value, float rangeStart, float rangeEnd);
float RangeMap(float inValue, float inStart, float inEnd, float outStart, float outEnd);
float RangeMapClamped(float inValue, float inStart, float inEnd, float outStart, float outEnd);
int   RoundDownToInt(float value);

//-End-of-Clamp-and-Lerp------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
//-Start-of-Angle-Utilities---------------------------------------------------------------------------

float ConvertDegreesToRadians(float degrees);
float ConvertRadiansToDegrees(float radians);
float CosDegrees(float degrees);
float SinDegrees(float degrees);
float Atan2Degrees(float y, float x);
float GetShortestAngularDispDegrees(float startDegrees, float endDegrees);
float GetTurnedTowardDegrees(float currentDegrees, float goalDegrees, float maxDeltaDegrees);
float GetAngleDegreesBetweenVectors2D(Vec2 const& a, Vec2 const& b);

//-End-of-Angle-Utilities-----------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
//-Start-of-Dot-and-Cross-----------------------------------------------------------------------------

float DotProduct2D(Vec2 const& a, Vec2 const& b);
float DotProduct3D(Vec3 const& a, Vec3 const& b);
float DotProduct4D(Vec4 const& a, Vec4 const& b);
float CrossProduct2D(Vec2 const& a, Vec2 const& b);
Vec3  CrossProduct3D(Vec3 const& a, Vec3 const& b);

//-End-of-Dot-and-Cross-------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
//-Start-of-Distance-&-Projections-Utilities----------------------------------------------------------

float GetDistance2D(Vec2 const& positionA, Vec2 const& positionB);
float GetDistanceSquared2D(Vec2 const& positionA, Vec2 const& positionB);
float GetDistance3D(Vec3 const& positionA, Vec3 const& positionB);
float GetDistanceSquared3D(Vec3 const& positionA, Vec3 const& positionB);
float GetDistanceXY3D(Vec3 const& positionA, Vec3 const& positionB);
float GetDistanceXYSquared3D(Vec3 const& positionA, Vec3 const& positionB);
int   GetTaxicabDistance2D(IntVec2 const& pointA, IntVec2 const& pointB);
float GetProjectedLength2D(Vec2 const& vectorToProject, Vec2 const& vectorToProjectOnto);
Vec2  GetProjectedOnto2D(Vec2 const& vectorToProject, Vec2 const& vectorToProjectOnto);

//-End-of-Distance-&-Projections-Utilities------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
//-Start-of-Geometry-Query-Utilities------------------------------------------------------------------

bool DoDiscsOverlap(Vec2 const& centerA, float radiusA, Vec2 const& centerB, float radiusB);
bool DoSpheresOverlap(Vec3 const& centerA, float radiusA, Vec3 const& centerB, float radiusB);
bool PushDiscOutOfPoint2D(Vec2& mobileDiscCenter, float discRadius, Vec2 const& fixedPoint);
bool PushDiscOutOfDisc2D(Vec2& mobileDiscCenter, float mobileDiscRadius, Vec2 const& fixedDiscCenter, float fixedDiscRadius);
bool PushDiscsOutOfEachOther2D(Vec2& aCenter, float aRadius, Vec2& bCenter, float bRadius);
bool PushDiscOutOfAABB2D(Vec2& mobileDiscCenter, float discRadius, AABB2 const& fixedBox);

//-End-of-Geometry-Query-Utilities--------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
//-Start-of-Is-Point-Inside-Utilities-----------------------------------------------------------------

bool IsPointInsideDisc2D(Vec2 const& point, Vec2 const& discCenter, float discRadius);
bool IsPointInsideDisc2D(Vec2 const& point, Disc2 const& disc);
bool IsPointInsideAABB2D(Vec2 const& point, AABB2 const& box);
bool IsPointInsideOBB2D(Vec2 const& point, OBB2 const& box);
bool IsPointInsideCapsule(Vec2 const& point, Capsule2 const& capsule);
bool IsPointInsideTriangle(Vec2 const& point, Triangle2 const& triangle);
bool IsPointInsideOrientedSector2D(Vec2 const& point, Vec2 const& sectorTip, float sectorForwardDegrees, float sectorApertureDegrees, float sectorRadius);
bool IsPointInsideDirectedSector2D(Vec2 const& point, Vec2 const& sectorTip, Vec2 const& sectorForwardNormal, float sectorApertureDegrees,
                                   float       sectorRadius);

//-End-of-Is-Point-Inside-Utilities-------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
//-Start-of-Get-Nearest-Point-Utilities---------------------------------------------------------------

Vec2 GetNearestPointOnDisc2D(Vec2 const& referencePosition, Vec2 const& discCenter, float discRadius);
Vec2 GetNearestPointOnDisc2D(const Vec2& referencePosition, const Disc2& disc);
Vec2 GetNearestPointOnAABB2D(Vec2 const& referencePos, AABB2 const& aabbBox);
Vec2 GetNearestPointOnOBB2D(Vec2 const& referencePos, OBB2 const& obbBox);
Vec2 GetNearestPointOnInfiniteLine2D(Vec2 const& referencePos, LineSegment2 const& infiniteLine);
Vec2 GetNearestPointOnLineSegment2D(Vec2 const& referencePos, LineSegment2 const& lineSegment);
Vec2 GetNearestPointOnCapsule2D(Vec2 const& referencePos, Capsule2 const& capsule);
Vec2 GetNearestPointOnTriangle2D(Vec2 const& referencePos, Triangle2 const& triangle);

//-End-of-Get-Nearest-Point-Utilities-----------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
//-Start-of-Transform-Utilities-----------------------------------------------------------------------

void TransformPosition2D(Vec2& posToTransform, float uniformScale, float rotationDegrees, Vec2 const& translation);
void TransformPosition2D(Vec2& posToTransform, Vec2 const& iBasis, Vec2 const& jBasis, Vec2 const& translation);
void TransformPositionXY3D(Vec3& posToTransform, float scaleXY, float zRotationDegrees, Vec2 const& translationXY);
void TransformPositionXY3D(Vec3& posToTransform, Vec2 const& iBasis, Vec2 const& jBasis, Vec2 const& translationXY);

//-End-of-Transform-Utilities-------------------------------------------------------------------------

float         NormalizeByte(unsigned char byte);
unsigned char DenormalizeByte(float zeroToOne);
