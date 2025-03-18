//----------------------------------------------------------------------------------------------------
// MathUtils.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include <cstdint>

#include "Engine/Math/Vec2.hpp"

//-Forward-Declaration--------------------------------------------------------------------------------
struct Cylinder3;
struct FloatRange;
struct AABB2;
struct AABB3;
struct Vec2;
struct Vec3;
struct Sphere3;
struct Vec4;
struct Mat44;

//----------------------------------------------------------------------------------------------------
float constexpr PI        = 3.14159265358979323846f;
float constexpr FLOAT_MIN = 1.175494351e-38F;
float constexpr FLOAT_MAX = 3.402823466e+38F;

//----------------------------------------------------------------------------------------------------
enum class eBillboardType : int8_t
{
    NONE = -1,
    FULL_FACING,
    FULL_OPPOSING,
    WORLD_UP_FACING,
    WORLD_UP_OPPOSING,
    COUNT
};

//-Start-of-Clamp-and-Lerp----------------------------------------------------------------------------

template <typename T>
T GetClamped(T const value, T const minValue, T const maxValue)
{
    if (value > maxValue) return maxValue;

    if (value < minValue) return minValue;

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
float GetProjectedLength3D(Vec3 const& vectorToProject, Vec3 const& vectorToProjectOnto);
Vec2  GetProjectedOnto2D(Vec2 const& vectorToProject, Vec2 const& vectorToProjectOnto);

//-End-of-Distance-&-Projections-Utilities------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
//-Start-of-Geometry-Query-Utilities------------------------------------------------------------------

bool DoDiscsOverlap(Vec2 const& centerA, float radiusA, Vec2 const& centerB, float radiusB);
bool DoSpheresOverlap(Vec3 const& centerA, float radiusA, Vec3 const& centerB, float radiusB);
bool DoAABB2sOverlap2D(AABB2 const& boxA, AABB2 const& boxB);
bool DoAABB3sOverlap3D(AABB3 const& first, AABB3 const& second);
bool DoZCylindersOverlap3D(Vec2 cylinder1CenterXY, float cylinder1Radius, FloatRange cylinder1MinMaxZ, Vec2 cylinder2CenterXY, float cylinder2Radius, FloatRange cylinder2MinMaxZ);
bool DoSphereAndAABB3Overlap3D(Vec3 sphereCenter, float sphereRadius, AABB3 box);
bool DoZCylinderAndAABB3Overlap3D(Vec2 cylinderCenterXY, float cylinderRadius, FloatRange cylinderMinMaxZ, AABB3 box);
bool DoZCylinderAndSphereOverlap3D(Vec2 cylinderCenterXY, float cylinderRadius, FloatRange cylinderMinMaxZ, Vec3 sphereCenter, float sphereRadius);
bool PushDiscOutOfPoint2D(Vec2& mobileDiscCenter, float discRadius, Vec2 const& fixedPoint);
bool PushDiscOutOfDisc2D(Vec2& mobileDiscCenter, float mobileDiscRadius, Vec2 const& fixedDiscCenter, float fixedDiscRadius);
bool PushDiscsOutOfEachOther2D(Vec2& aCenter, float aRadius, Vec2& bCenter, float bRadius);
bool PushDiscOutOfAABB2D(Vec2& mobileDiscCenter, float discRadius, AABB2 const& fixedBox);

//-End-of-Geometry-Query-Utilities--------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
//-Start-of-Is-Point-Inside-Utilities-----------------------------------------------------------------

bool IsPointInsideDisc2D(Vec2 const& point, Vec2 const& discCenter, float discRadius);
bool IsPointInsideAABB2D(Vec2 const& point, Vec2 const& aabb2Mins, Vec2 const& aabb2Maxs);
bool IsPointInsideOBB2D(Vec2 const& point, Vec2 const& obb2Center, Vec2 const& obb2IBasisNormal, Vec2 const& obb2HalfDimensions);
bool IsPointInsideCapsule(Vec2 const& point, Vec2 const& capsuleStartPosition, Vec2 const& capsuleEndPosition, float capsuleRadius);
bool IsPointInsideTriangle(Vec2 const& point, Vec2 const& ccw1, Vec2 const& ccw2, Vec2 const& ccw3);
bool IsPointInsideOrientedSector2D(Vec2 const& point, Vec2 const& sectorTip, float sectorForwardDegrees, float sectorApertureDegrees, float sectorRadius);
bool IsPointInsideDirectedSector2D(Vec2 const& point, Vec2 const& sectorTip, Vec2 const& sectorForwardNormal, float sectorApertureDegrees,
                                   float       sectorRadius);

//-End-of-Is-Point-Inside-Utilities-------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
//-Start-of-Get-Nearest-Point-Utilities---------------------------------------------------------------

Vec2 GetNearestPointOnDisc2D(Vec2 const& point, Vec2 const& discCenter, float discRadius);
Vec2 GetNearestPointOnLineSegment2D(Vec2 const& point, Vec2 const& lineStartPosition, Vec2 const& lineEndPosition, bool isLineInfinite);
Vec2 GetNearestPointOnTriangle2D(Vec2 const& point, Vec2 const triangle2Points[3]);
Vec2 GetNearestPointOnAABB2D(Vec2 const& point, Vec2 const& aabb2Mins, Vec2 const& aabb2Maxs);
Vec2 GetNearestPointOnOBB2D(Vec2 const& point, Vec2 const& obb2Center, Vec2 const& obb2IBasisNormal, Vec2 const& obb2HalfDimensions);
Vec2 GetNearestPointOnCapsule2D(Vec2 const& point, Vec2 const& capsuleStartPosition, Vec2 const& capsuleEndPosition, float capsuleRadius);
Vec3 GetNearestPointOnAABB3D(Vec3 const& point, AABB3 const& aabb3);
Vec3 GetNearestPointOnSphere3D(Vec3 const& point, Sphere3 const& sphere3);
Vec3 GetNearestPointOnZCylinder3D(Vec3 const& point, Cylinder3 const& cylinder3);

//-End-of-Get-Nearest-Point-Utilities-----------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
//-Start-of-Transform-Utilities-----------------------------------------------------------------------

void TransformPosition2D(Vec2& posToTransform, float uniformScale, float rotationDegrees, Vec2 const& translation);
void TransformPosition2D(Vec2& posToTransform, Vec2 const& iBasis, Vec2 const& jBasis, Vec2 const& translation);
void TransformPositionXY3D(Vec3& posToTransform, float scaleXY, float zRotationDegrees, Vec2 const& translationXY);
void TransformPositionXY3D(Vec3& posToTransform, Vec2 const& iBasis, Vec2 const& jBasis, Vec2 const& translationXY);

//-End-of-Transform-Utilities-------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
//-Start-of-Byte-Denormalization----------------------------------------------------------------------

float         NormalizeByte(unsigned char byte);
unsigned char DenormalizeByte(float zeroToOne);

//-End-of-Byte-Denormalization------------------------------------------------------------------------

Mat44 GetBillboardMatrix(eBillboardType billboardType, Mat44 const& targetMatrix, Vec3 const& billboardPosition, Vec2 const& billboardScale = Vec2::ONE);
