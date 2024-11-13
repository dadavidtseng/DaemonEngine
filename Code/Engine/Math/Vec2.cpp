//----------------------------------------------------------------------------------------------------
// Vec2.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Math/Vec2.hpp"

#include <cmath>

#include "Engine/Math/MathUtils.hpp"

//----------------------------------------------------------------------------------------------------
constexpr float EPSILON = 1e-5f;	// Define a small tolerance value
Vec2 Vec2::ZERO = Vec2(0, 0);
Vec2 Vec2::ONE = Vec2(1, 1);

//----------------------------------------------------------------------------------------------------
Vec2::Vec2(const float initialX, const float initialY)
	: x(initialX)
	, y(initialY)
{
}

//----------------------------------------------------------------------------------------------------
Vec2 Vec2::MakeFromPolarRadians(const float orientationRadians,
                                const float length)
{
	const float degree = ConvertRadiansToDegrees(orientationRadians);
	const float x      = length * CosDegrees(degree);
	const float y      = length * SinDegrees(degree);

	return Vec2(x, y);
}

//----------------------------------------------------------------------------------------------------
Vec2 Vec2::MakeFromPolarDegrees(const float orientationDegrees,
                                const float length)
{
	const float x = length * CosDegrees(orientationDegrees);
	const float y = length * SinDegrees(orientationDegrees);

	return Vec2(x, y);
}

//----------------------------------------------------------------------------------------------------
float Vec2::GetLength() const
{
	return sqrt(x * x + y * y);
}

//----------------------------------------------------------------------------------------------------
float Vec2::GetLengthSquared() const
{
	return x * x + y * y;
}

//----------------------------------------------------------------------------------------------------
float Vec2::GetOrientationRadians() const
{
	return ConvertDegreesToRadians(Atan2Degrees(y, x));
}

//----------------------------------------------------------------------------------------------------
float Vec2::GetOrientationDegrees() const
{
	return Atan2Degrees(y, x);
}

//----------------------------------------------------------------------------------------------------
Vec2 Vec2::GetRotated90Degrees() const
{
	return Vec2(-y, x);
}

//----------------------------------------------------------------------------------------------------
Vec2 Vec2::GetRotatedMinus90Degrees() const
{
	return Vec2(y, -x);
}

//----------------------------------------------------------------------------------------------------
Vec2 Vec2::GetRotatedRadians(const float deltaRadians) const
{
	const float theta        = Atan2Degrees(y, x);
	const float radius       = GetLength();
	const float rotatedTheta = theta + ConvertRadiansToDegrees(deltaRadians);

	return Vec2(radius * CosDegrees(rotatedTheta), radius * SinDegrees(rotatedTheta));
}

//----------------------------------------------------------------------------------------------------
Vec2 Vec2::GetRotatedDegrees(const float deltaDegrees) const
{
	const float theta        = Atan2Degrees(y, x);
	const float radius       = GetLength();
	const float rotatedTheta = theta + deltaDegrees;

	return Vec2(radius * CosDegrees(rotatedTheta), radius * SinDegrees(rotatedTheta));
}

//----------------------------------------------------------------------------------------------------
Vec2 Vec2::GetClamped(const float maxLength) const
{
	const float length = GetLength();

	if (length > maxLength)
	{
		const float scale = maxLength / length;

		return Vec2(x * scale, y * scale);
	}

	return Vec2(x, y);
}

//----------------------------------------------------------------------------------------------------
Vec2 Vec2::GetNormalized() const
{
	const float length      = GetLength();
	const float scale       = 1 / length;
	const float normalizedX = x * scale;
	const float normalizedY = y * scale;

	return Vec2(normalizedX, normalizedY);
}

//----------------------------------------------------------------------------------------------------
Vec2 Vec2::GetReflected(Vec2 const& normalOfSurfaceToReflectOffOf) const
{
	const float dotProduct = DotProduct2D(*this, normalOfSurfaceToReflectOffOf);

	return *this - 2 * dotProduct * normalOfSurfaceToReflectOffOf;
}

//----------------------------------------------------------------------------------------------------
void Vec2::SetOrientationRadians(const float newOrientationRadians)
{
	const float length    = GetLength();
	const float newDegree = ConvertRadiansToDegrees(newOrientationRadians);

	x = length * CosDegrees(newDegree);
	y = length * SinDegrees(newDegree);
}

//----------------------------------------------------------------------------------------------------
void Vec2::SetOrientationDegrees(const float newOrientationDegrees)
{
	const float length = GetLength();

	x = length * CosDegrees(newOrientationDegrees);
	y = length * SinDegrees(newOrientationDegrees);
}

//----------------------------------------------------------------------------------------------------
void Vec2::SetPolarRadians(const float newOrientationRadians,
                           const float newLength)
{
	const float newDegree = ConvertRadiansToDegrees(newOrientationRadians);

	x = newLength * CosDegrees(newDegree);
	y = newLength * SinDegrees(newDegree);
}

//----------------------------------------------------------------------------------------------------
void Vec2::SetPolarDegrees(const float newOrientationDegrees,
                           const float newLength)
{
	x = newLength * CosDegrees(newOrientationDegrees);
	y = newLength * SinDegrees(newOrientationDegrees);
}

//----------------------------------------------------------------------------------------------------
void Vec2::Rotate90Degrees()
{
	const float oldX = x;

	x = -y;
	y = oldX;
}

//----------------------------------------------------------------------------------------------------
void Vec2::RotateMinus90Degrees()
{
	const float oldX = x;

	x = y;
	y = -oldX;
}
//----------------------------------------------------------------------------------------------------
void Vec2::RotateRadians(const float deltaRadians)
{
	const float length    = GetLength();
	const float degree    = GetOrientationDegrees();
	const float newDegree = degree + ConvertRadiansToDegrees(deltaRadians);

	x = length * CosDegrees(newDegree);
	y = length * SinDegrees(newDegree);
}

//----------------------------------------------------------------------------------------------------
void Vec2::RotateDegrees(const float deltaDegrees)
{
	const float length    = GetLength();
	const float newDegree = GetOrientationDegrees() + deltaDegrees;

	x = length * CosDegrees(newDegree);
	y = length * SinDegrees(newDegree);
}

//----------------------------------------------------------------------------------------------------
void Vec2::SetLength(const float newLength)
{
	Normalize();

	x *= newLength;
	y *= newLength;
}

//----------------------------------------------------------------------------------------------------
void Vec2::ClampLength(const float maxLength)
{
	if (GetLength() > maxLength)
	{
		Normalize();
		x *= maxLength;
		y *= maxLength;
	}
}

//----------------------------------------------------------------------------------------------------
void Vec2::Normalize()
{
	const float length = GetLength();
	const float scale  = 1.f / length;

	x *= scale;
	y *= scale;
}

//----------------------------------------------------------------------------------------------------
float Vec2::NormalizeAndGetPreviousLength()
{
	const float length = GetLength();

	if (length == 0.f)
		return 0.f;

	x *= 1.f / length;
	y *= 1.f / length;

	return length;
}

//----------------------------------------------------------------------------------------------------
void Vec2::Reflect(Vec2 const& normalOfSurfaceToReflectOffOf)
{
	const float dotProduct = DotProduct2D(*this, normalOfSurfaceToReflectOffOf);

	*this = *this - 2 * dotProduct * normalOfSurfaceToReflectOffOf;
}

//----------------------------------------------------------------------------------------------------
Vec2 Vec2::operator+(Vec2 const& vecToAdd) const
{
	return Vec2(x + vecToAdd.x, y + vecToAdd.y);
}

//----------------------------------------------------------------------------------------------------
Vec2 Vec2::operator-(Vec2 const& vecToSubtract) const
{
	return Vec2(x - vecToSubtract.x, y - vecToSubtract.y);
}

//----------------------------------------------------------------------------------------------------
Vec2 Vec2::operator-() const
{
	return Vec2(-x, -y);
}

//----------------------------------------------------------------------------------------------------
Vec2 Vec2::operator*(const float uniformScale) const
{
	return Vec2(x * uniformScale, y * uniformScale);
}

//----------------------------------------------------------------------------------------------------
Vec2 Vec2::operator*(Vec2 const& vecToMultiply) const
{
	return Vec2(x * vecToMultiply.x, y * vecToMultiply.y);
}

//----------------------------------------------------------------------------------------------------
Vec2 Vec2::operator/(const float inverseScale) const
{
	return Vec2(x / inverseScale, y / inverseScale);
}

//----------------------------------------------------------------------------------------------------
void Vec2::operator+=(Vec2 const& vecToAdd)
{
	x += vecToAdd.x;
	y += vecToAdd.y;
}

//----------------------------------------------------------------------------------------------------
void Vec2::operator-=(Vec2 const& vecToSubtract)
{
	x -= vecToSubtract.x;
	y -= vecToSubtract.y;
}

//----------------------------------------------------------------------------------------------------
void Vec2::operator*=(const float uniformScale)
{
	x *= uniformScale;
	y *= uniformScale;
}

//----------------------------------------------------------------------------------------------------
void Vec2::operator/=(const float uniformDivisor)
{
	x /= uniformDivisor;
	y /= uniformDivisor;
}

//----------------------------------------------------------------------------------------------------
Vec2& Vec2::operator=(Vec2 const& copyFrom)
{
	// Check for self-assignment
	if (this != &copyFrom)
	{
		x = copyFrom.x;
		y = copyFrom.y;
	}

	return *this;	// Return the current object by reference (dereference)
}

//----------------------------------------------------------------------------------------------------
Vec2 operator*(const float uniformScale,
               Vec2 const& vecToScale)
{
	return Vec2(vecToScale.x * uniformScale, vecToScale.y * uniformScale);
}

//----------------------------------------------------------------------------------------------------
bool Vec2::operator==(Vec2 const& compare) const
{
	return
		std::fabs(x - compare.x) < EPSILON &&
		std::fabs(y - compare.y) < EPSILON;
}

//----------------------------------------------------------------------------------------------------
bool Vec2::operator!=(Vec2 const& compare) const
{
	return
		!(*this == compare);
}
