//----------------------------------------------------------------------------------------------------
// Vec2.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Math/Vec2.hpp"

#include <cmath>

#include "Engine/Core/StringUtils.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/MathUtils.hpp"

//----------------------------------------------------------------------------------------------------
Vec2 Vec2::ZERO = Vec2(0, 0);
Vec2 Vec2::ONE  = Vec2(1, 1);

//----------------------------------------------------------------------------------------------------
Vec2::Vec2(float const initialX, float const initialY)
    : x(initialX)
    , y(initialY)
{
}

//----------------------------------------------------------------------------------------------------
Vec2::Vec2(int const initialX, int const initialY)
    : x(static_cast<float>(initialX)),
      y(static_cast<float>(initialY))
{
}

//----------------------------------------------------------------------------------------------------
Vec2::Vec2(IntVec2 const& intVec2)
    : x(static_cast<float>(intVec2.x)),
      y(static_cast<float>(intVec2.y))
{
}

//----------------------------------------------------------------------------------------------------
Vec2 Vec2::MakeFromPolarRadians(float const orientationRadians,
                                float const length)
{
    float const degree = ConvertRadiansToDegrees(orientationRadians);
    float const x      = length * CosDegrees(degree);
    float const y      = length * SinDegrees(degree);

    return Vec2(x, y);
}

//----------------------------------------------------------------------------------------------------
Vec2 Vec2::MakeFromPolarDegrees(float const orientationDegrees,
                                float const length)
{
    float const x = length * CosDegrees(orientationDegrees);
    float const y = length * SinDegrees(orientationDegrees);

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
Vec2 Vec2::GetRotatedRadians(float const deltaRadians) const
{
    float const theta        = Atan2Degrees(y, x);
    float const radius       = GetLength();
    float const rotatedTheta = theta + ConvertRadiansToDegrees(deltaRadians);

    return Vec2(radius * CosDegrees(rotatedTheta), radius * SinDegrees(rotatedTheta));
}

//----------------------------------------------------------------------------------------------------
Vec2 Vec2::GetRotatedDegrees(float const deltaDegrees) const
{
    float const theta        = Atan2Degrees(y, x);
    float const radius       = GetLength();
    float const rotatedTheta = theta + deltaDegrees;

    return Vec2(radius * CosDegrees(rotatedTheta), radius * SinDegrees(rotatedTheta));
}

//----------------------------------------------------------------------------------------------------
Vec2 Vec2::GetClamped(float const maxLength) const
{
    float const length = GetLength();

    if (length > maxLength)
    {
        float const scale = maxLength / length;

        return Vec2(x * scale, y * scale);
    }

    return Vec2(x, y);
}

//----------------------------------------------------------------------------------------------------
Vec2 Vec2::GetNormalized() const
{
    float const length      = GetLength();
    float const scale       = 1 / length;
    float const normalizedX = x * scale;
    float const normalizedY = y * scale;

    return Vec2(normalizedX, normalizedY);
}

//----------------------------------------------------------------------------------------------------
Vec2 Vec2::GetReflected(Vec2 const& normalOfSurfaceToReflectOffOf) const
{
    float const dotProduct = DotProduct2D(*this, normalOfSurfaceToReflectOffOf);

    return *this - 2 * dotProduct * normalOfSurfaceToReflectOffOf;
}

//----------------------------------------------------------------------------------------------------
void Vec2::SetOrientationRadians(float const newOrientationRadians)
{
    float const length    = GetLength();
    float const newDegree = ConvertRadiansToDegrees(newOrientationRadians);

    x = length * CosDegrees(newDegree);
    y = length * SinDegrees(newDegree);
}

//----------------------------------------------------------------------------------------------------
void Vec2::SetOrientationDegrees(float const newOrientationDegrees)
{
    const float length = GetLength();

    x = length * CosDegrees(newOrientationDegrees);
    y = length * SinDegrees(newOrientationDegrees);
}

//----------------------------------------------------------------------------------------------------
void Vec2::SetPolarRadians(float const newOrientationRadians,
                           float const newLength)
{
    float const newDegree = ConvertRadiansToDegrees(newOrientationRadians);

    x = newLength * CosDegrees(newDegree);
    y = newLength * SinDegrees(newDegree);
}

//----------------------------------------------------------------------------------------------------
void Vec2::SetPolarDegrees(float const newOrientationDegrees,
                           float const newLength)
{
    x = newLength * CosDegrees(newOrientationDegrees);
    y = newLength * SinDegrees(newOrientationDegrees);
}

//----------------------------------------------------------------------------------------------------
void Vec2::Rotate90Degrees()
{
    float const oldX = x;

    x = -y;
    y = oldX;
}

//----------------------------------------------------------------------------------------------------
void Vec2::RotateMinus90Degrees()
{
    float const oldX = x;

    x = y;
    y = -oldX;
}
//----------------------------------------------------------------------------------------------------
void Vec2::RotateRadians(float const deltaRadians)
{
    float const length    = GetLength();
    float const degree    = GetOrientationDegrees();
    float const newDegree = degree + ConvertRadiansToDegrees(deltaRadians);

    x = length * CosDegrees(newDegree);
    y = length * SinDegrees(newDegree);
}

//----------------------------------------------------------------------------------------------------
void Vec2::RotateDegrees(float const deltaDegrees)
{
    float const length    = GetLength();
    float const newDegree = GetOrientationDegrees() + deltaDegrees;

    x = length * CosDegrees(newDegree);
    y = length * SinDegrees(newDegree);
}

//----------------------------------------------------------------------------------------------------
void Vec2::SetLength(float const newLength)
{
    Normalize();

    x *= newLength;
    y *= newLength;
}

//----------------------------------------------------------------------------------------------------
void Vec2::ClampLength(float const maxLength)
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
    float const length = GetLength();
    float const scale  = 1.f / length;

    x *= scale;
    y *= scale;
}

//----------------------------------------------------------------------------------------------------
float Vec2::NormalizeAndGetPreviousLength()
{
    float const length = GetLength();

    if (length == 0.f)
        return 0.f;

    x *= 1.f / length;
    y *= 1.f / length;

    return length;
}

//----------------------------------------------------------------------------------------------------
void Vec2::Reflect(Vec2 const& normalOfSurfaceToReflectOffOf)
{
    float const dotProduct = DotProduct2D(*this, normalOfSurfaceToReflectOffOf);

    *this = *this - 2 * dotProduct * normalOfSurfaceToReflectOffOf;
}

//----------------------------------------------------------------------------------------------------
void Vec2::SetFromText(char const* text)
{
    // Use SplitStringOnDelimiter to divide the input text into parts based on the delimiter ','
    Strings const parts = SplitStringOnDelimiter(text, ',');

    // Input must contain exactly two parts; otherwise, reset to default values
    if (parts.size() != 2)
    {
        x = 0.0f;
        y = 0.0f;
        return;
    }

    // Convert the two parts to floats using atof
    x = static_cast<float>(atof(parts[0].c_str()));
    y = static_cast<float>(atof(parts[1].c_str()));
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
Vec2 Vec2::operator*(float const uniformScale) const
{
    return Vec2(x * uniformScale, y * uniformScale);
}

//----------------------------------------------------------------------------------------------------
Vec2 Vec2::operator*(Vec2 const& vecToMultiply) const
{
    return Vec2(x * vecToMultiply.x, y * vecToMultiply.y);
}

//----------------------------------------------------------------------------------------------------
Vec2 Vec2::operator/(float const inverseScale) const
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
void Vec2::operator*=(float const uniformScale)
{
    x *= uniformScale;
    y *= uniformScale;
}

//----------------------------------------------------------------------------------------------------
void Vec2::operator/=(float const uniformDivisor)
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
Vec2 operator*(float const uniformScale,
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
