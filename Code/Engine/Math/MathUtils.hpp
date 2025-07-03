//----------------------------------------------------------------------------------------------------
// MathUtils.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include <cstdint>

#include "Engine/Math/Vec2.hpp"

struct Plane3;
struct OBB3;
//-Forward-Declaration--------------------------------------------------------------------------------
struct AABB2;
struct AABB3;
struct FloatRange;
struct IntVec2;
struct Vec2;
struct Vec3;
struct Vec4;
struct Mat44;

//----------------------------------------------------------------------------------------------------
float constexpr PI        = 3.14159265358979323846f;
float constexpr FLOAT_MIN = 1.175494351e-38F;
float constexpr FLOAT_MAX = 3.402823466e+38F;
float constexpr EPSILON   = 1e-6f;

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

bool DoDiscsOverlap2D(Vec2 const& centerA, float radiusA, Vec2 const& centerB, float radiusB);
bool DoAABB2sOverlap2D(AABB2 const& boxA, AABB2 const& boxB);
bool DoDiscAndAABB2Overlap2D(Vec2 const& discCenter, float discRadius, AABB2 const& aabb2);
bool DoSpheresOverlap3D(Vec3 const& centerA, float radiusA, Vec3 const& centerB, float radiusB);
bool DoSphereAndAABB3Overlap3D(Vec3 const& sphereCenter, float sphereRadius, AABB3 const& aabb3);
bool DoSphereAndZCylinderOverlap3D(Vec3 const& sphereCenter, float sphereRadius, Vec2 const& cylinderCenterXY, float cylinderRadius, FloatRange const& cylinderMinMaxZ);
bool DoAABB3sOverlap3D(AABB3 const& first, AABB3 const& second);
bool DoAABB3AndZCylinderOverlap3D(AABB3 const& aabb3, Vec2 const& cylinderCenterXY, float cylinderRadius, FloatRange const& cylinderMinMaxZ);
bool DoZCylindersOverlap3D(Vec2 const& cylinder1CenterXY, float cylinder1Radius, FloatRange const& cylinder1MinMaxZ, Vec2 const& cylinder2CenterXY, float cylinder2Radius, FloatRange const& cylinder2MinMaxZ);
bool DoSphereAndOBB3Overlap3D(Vec3 const& sphereCenter, float sphereRadius, OBB3 const& obb3);
bool DoSphereAndPlaneOverlap3D(Vec3 const& sphereCenter, float sphereRadius, Plane3 const& plane3);
bool DoAABB3AndPlane3Overlap3D(AABB3 const& aabb3, Plane3 const& plane3);
bool DoOBB3AndPlane3Overlap3D(OBB3 const& obb3, Plane3 const& plane3);
bool PushDiscOutOfPoint2D(Vec2& mobileDiscCenter, float discRadius, Vec2 const& fixedPoint);
bool PushDiscOutOfDisc2D(Vec2& mobileDiscCenter, float mobileDiscRadius, Vec2 const& fixedDiscCenter, float fixedDiscRadius);
bool PushDiscsOutOfEachOther2D(Vec2& aCenter, float aRadius, Vec2& bCenter, float bRadius);
bool PushDiscOutOfAABB2D(Vec2& mobileDiscCenter, float discRadius, AABB2 const& fixedBox);
bool BounceDiscOutOfEachOther2D(Vec2& aCenter, float aRadius, Vec2& aVelocity, float aElasticity, Vec2& bCenter, float bRadius, Vec2& bVelocity, float bElasticity);
bool BounceDiscOutOfFixedPoint2D(Vec2& discCenter, float discRadius, Vec2& discVelocity, float discElasticity, Vec2 const& fixedPoint, float pointElasticity);
bool BounceDiscOutOfFixedDisc2D(Vec2& mobileCenter, float mobileRadius, Vec2& mobileVelocity, float mobileElasticity, Vec2 const& fixedCenter, float fixedRadius, float fixedElasticity);
bool BounceDiscOutOfFixedOBB2D(Vec2& mobileCenter, float mobileRadius, Vec2& mobileVelocity, float mobileElasticity, Vec2 const& obbCenter, Vec2 const& obb2IBasisNormal, Vec2 const& obb2HalfDimensions, float fixedElasticity);
bool BounceDiscOutOfFixedCapsule2D(Vec2& mobileCenter, float mobileRadius, Vec2& mobileVelocity, float mobileElasticity, Vec2 const& fixedBoneStart, Vec2 const& fixedBoneEnd, float fixedRadius, float fixedElasticity);

//-End-of-Geometry-Query-Utilities--------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
//-Start-of-Is-Point-Inside-Utilities-----------------------------------------------------------------

bool IsPointInsideDisc2D(Vec2 const& point, Vec2 const& discCenter, float discRadius);
bool IsPointInsideTriangle2D(Vec2 const& point, Vec2 const& ccw1, Vec2 const& ccw2, Vec2 const& ccw3);
bool IsPointInsideAABB2D(Vec2 const& point, Vec2 const& aabb2Mins, Vec2 const& aabb2Maxs);
bool IsPointInsideOBB2D(Vec2 const& point, Vec2 const& obb2Center, Vec2 const& obb2IBasisNormal, Vec2 const& obb2HalfDimensions);
bool IsPointInsideCapsule2D(Vec2 const& point, Vec2 const& capsuleStartPosition, Vec2 const& capsuleEndPosition, float capsuleRadius);
bool IsPointInsideOrientedSector2D(Vec2 const& point, Vec2 const& sectorTip, float sectorForwardDegrees, float sectorApertureDegrees, float sectorRadius);
bool IsPointInsideDirectedSector2D(Vec2 const& point, Vec2 const& sectorTip, Vec2 const& sectorForwardNormal, float sectorApertureDegrees, float sectorRadius);
bool IsPointInsideSphere3D(Vec3 const& point, Vec3 const& sphereCenter, float sphereRadius);
bool IsPointInsideAABB3D(Vec3 const& point, Vec3 const& aabb3Mins, Vec3 const& aabb3Maxs);
bool IsPointInsideZCylinder3D(Vec3 const& point, Vec3 const& cylinderStartPosition, Vec3 const& cylinderEndPosition, float cylinderRadius);
bool IsPointInsideOBB3D(Vec3 const& point, OBB3 const& obb3);

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
Vec3 GetNearestPointOnSphere3D(Vec3 const& point, Vec3 const& sphereCenter, float sphereRadius);
Vec3 GetNearestPointOnZCylinder3D(Vec3 const& point, Vec3 const& cylinderStartPosition, Vec3 const& cylinderEndPosition, float cylinderRadius);
Vec3 GetNearestPointOnPlane3D(Vec3 const& point, Plane3 const& plane);
Vec3 GetNearestPointOnOBB3D(Vec3 const& point, OBB3 const& obb3);

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
//----------------------------------------------------------------------------------------------------

Mat44 GetBillboardMatrix(eBillboardType billboardType, Mat44 const& targetMatrix, Vec3 const& billboardPosition, Vec2 const& billboardScale = Vec2::ONE);


//-Start-of-Curves-and-Splines------------------------------------------------------------------------
float ComputeCubicBezier1D(float A, float B, float C, float D, float t);
float ComputeQuinticBezier1D(float A, float B, float C, float D, float E, float F, float t);
float SmoothStart2(float t);
float SmoothStart3(float t);
float SmoothStart4(float t);
float SmoothStart5(float t);
float SmoothStart6(float t);
float SmoothStartN(float t, int n);
float SmoothStop2(float t);
float SmoothStop3(float t);
float SmoothStop4(float t);
float SmoothStop5(float t);
float SmoothStop6(float t);
float SmoothStopN(float t, int n);
float SmoothStep3(float t);
float SmoothStep5(float t);
float SmoothStep6(float t);
float Hesitate3(float t);
float Hesitate5(float t);
float CustomFunkyEasingFunction(float t);
