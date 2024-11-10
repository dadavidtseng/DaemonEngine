#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "cmath"

//-----------------------------------------------------------------------------------------------

Vec3::Vec3(float initialX, float initialY, float initialZ)
	: x(initialX)
	, y(initialY)
	, z(initialZ)
{
}

float Vec3::GetLength() const
{
	return sqrtf(x * x + y * y + z * z);
}

float Vec3::GetLengthXY() const
{
	return sqrtf(x * x + y * y);
}

float Vec3::GetLengthSquared() const
{
	return x * x + y * y + z * z;
}

float Vec3::GetLengthXYSquared() const
{
	return x * x + y * y;
}

float Vec3::GetAngleAboutZRadians() const
{
	return ConvertDegreesToRadians(Atan2Degrees(y, x));
}

float Vec3::GetAngleAboutZDegrees() const
{
	return Atan2Degrees(y, x);
}

Vec3 const Vec3::GetRotatedAboutZRadians(float deltaRadians) const
{
	float length = GetLengthXY();
	float degree = GetAngleAboutZDegrees();
	float newDegree = degree + ConvertRadiansToDegrees(deltaRadians);

	float deltaX = length * CosDegrees(newDegree);
	float deltaY = length * SinDegrees(newDegree);

	return Vec3(deltaX, deltaY, z);
}

Vec3 const Vec3::GetRotatedAboutZDegrees(float deltaDegrees) const
{
	float length = GetLengthXY();
	float newDegree = GetAngleAboutZDegrees() + deltaDegrees;

	float deltaX = length * CosDegrees(newDegree);
	float deltaY = length * SinDegrees(newDegree);

	return Vec3(deltaX, deltaY, z);
}

Vec3 const Vec3::GetClamped(float maxLength) const
{
	float length = GetLength();

	if (length > maxLength)
	{
		float scale = maxLength / length;

		return Vec3(x * scale, y * scale, z * scale);
	}
	else
	{
		return Vec3(x, y, z);
	}
}

Vec3 const Vec3::GetNormalized() const
{
	float length = GetLength();
	float scale = 1.f / length;

	return Vec3(x * scale, y * scale, z * scale);
}

//-----------------------------------------------------------------------------------------------
bool Vec3::operator==(Vec3 const& compare) const
{
	return (x == compare.x) && (y == compare.y) && (z == compare.z);
}

//-----------------------------------------------------------------------------------------------
bool Vec3::operator!=(Vec3 const& compare) const
{
	return (x != compare.x) || (y != compare.y) || (z != compare.z);
}

//-----------------------------------------------------------------------------------------------
Vec3 const Vec3::operator+(Vec3 const& vecToAdd) const
{
	return Vec3(x + vecToAdd.x, y + vecToAdd.y, z + vecToAdd.z);
}

//-----------------------------------------------------------------------------------------------
Vec3 const Vec3::operator-(Vec3 const& vecToSubtract) const
{
	return Vec3(x - vecToSubtract.x, y - vecToSubtract.y, z - vecToSubtract.z);
}

//-----------------------------------------------------------------------------------------------
Vec3 const Vec3::operator*(float uniformScale) const
{
	return Vec3(x * uniformScale, y * uniformScale, z * uniformScale);
}

//-----------------------------------------------------------------------------------------------
Vec3 const Vec3::operator/(float inverseScale) const
{
	float scale = 1.f / inverseScale;
	return Vec3(x * scale, y * scale, z * scale);
}

//-----------------------------------------------------------------------------------------------
void Vec3::operator+=(Vec3 const& vecToAdd)
{
	x += vecToAdd.x;
	y += vecToAdd.y;
	z += vecToAdd.z;
}

//-----------------------------------------------------------------------------------------------
void Vec3::operator-=(Vec3 const& vecToSubtract)
{
	x -= vecToSubtract.x;
	y -= vecToSubtract.y;
	z -= vecToSubtract.z;
}

//-----------------------------------------------------------------------------------------------
void Vec3::operator*=(const float uniformScale)
{
	x *= uniformScale;
	y *= uniformScale;
	z *= uniformScale;
}

//-----------------------------------------------------------------------------------------------
void Vec3::operator/=(const float uniformDivisor)
{
	float scale = 1.f / uniformDivisor;
	x *= scale;
	y *= scale;
	z *= scale;
}

//-----------------------------------------------------------------------------------------------
void Vec3::operator=(Vec3 const& copyFrom)
{
	x = copyFrom.x;
	y = copyFrom.y;
	z = copyFrom.z;
}

//-----------------------------------------------------------------------------------------------
Vec3 const operator*(float uniformScale, Vec3 const& vecToScale)
{
	return Vec3(uniformScale * vecToScale.x, uniformScale * vecToScale.y, uniformScale * vecToScale.z);
}