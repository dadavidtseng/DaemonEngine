//----------------------------------------------------------------------------------------------------
// Vec4.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Math/Vec4.hpp"

#include <cmath>

//----------------------------------------------------------------------------------------------------
Vec4 Vec4::ZERO = Vec4(0.f, 0.f, 0.f, 0.f);
Vec4 Vec4::ONE = Vec4(1.f, 1.f, 1.f, 1.f);

// Constructor
Vec4::Vec4(float initialX, float initialY, float initialZ, float initialW)
    : x(initialX), y(initialY), z(initialZ), w(initialW) {}

// Accessors
float Vec4::GetLength() const
{
    return std::sqrt(x * x + y * y + z * z + w * w);
}

float Vec4::GetLengthSquared() const
{
    return x * x + y * y + z * z + w * w;
}

Vec4 Vec4::GetNormalized() const
{
    float length = GetLength();
    if (length == 0.f)
        return Vec4::ZERO;
    return *this / length;
}

Vec4 Vec4::GetClamped(float maxLength) const
{
    float length = GetLength();
    if (length > maxLength && length > 0.f)
    {
        return *this * (maxLength / length);
    }
    return *this;
}

// Mutators
void Vec4::SetLength(float newLength)
{
    Normalize();
    *this *= newLength;
}

void Vec4::Normalize()
{
    float length = GetLength();
    if (length != 0.f)
    {
        *this /= length;
    }
}

void Vec4::ClampLength(float maxLength)
{
    float length = GetLength();
    if (length > maxLength && length > 0.f)
    {
        *this *= (maxLength / length);
    }
}

// Operators
bool Vec4::operator==(Vec4 const& compare) const
{
    return x == compare.x && y == compare.y && z == compare.z && w == compare.w;
}

bool Vec4::operator!=(Vec4 const& compare) const
{
    return !(*this == compare);
}

Vec4 Vec4::operator+(Vec4 const& vecToAdd) const
{
    return Vec4(x + vecToAdd.x, y + vecToAdd.y, z + vecToAdd.z, w + vecToAdd.w);
}

Vec4 Vec4::operator-(Vec4 const& vecToSubtract) const
{
    return Vec4(x - vecToSubtract.x, y - vecToSubtract.y, z - vecToSubtract.z, w - vecToSubtract.w);
}

Vec4 Vec4::operator-() const
{
    return Vec4(-x, -y, -z, -w);
}

Vec4 Vec4::operator*(float uniformScale) const
{
    return Vec4(x * uniformScale, y * uniformScale, z * uniformScale, w * uniformScale);
}

Vec4 Vec4::operator/(float inverseScale) const
{
    return Vec4(x / inverseScale, y / inverseScale, z / inverseScale, w / inverseScale);
}

void Vec4::operator+=(Vec4 const& vecToAdd)
{
    x += vecToAdd.x;
    y += vecToAdd.y;
    z += vecToAdd.z;
    w += vecToAdd.w;
}

void Vec4::operator-=(Vec4 const& vecToSubtract)
{
    x -= vecToSubtract.x;
    y -= vecToSubtract.y;
    z -= vecToSubtract.z;
    w -= vecToSubtract.w;
}

void Vec4::operator*=(float uniformScale)
{
    x *= uniformScale;
    y *= uniformScale;
    z *= uniformScale;
    w *= uniformScale;
}

void Vec4::operator/=(float uniformDivisor)
{
    x /= uniformDivisor;
    y /= uniformDivisor;
    z /= uniformDivisor;
    w /= uniformDivisor;
}

Vec4& Vec4::operator=(Vec4 const& copyFrom)
{
    x = copyFrom.x;
    y = copyFrom.y;
    z = copyFrom.z;
    w = copyFrom.w;
    return *this;
}

// Friend functions
Vec4 operator*(float uniformScale, Vec4 const& vecToScale)
{
    return Vec4(vecToScale.x * uniformScale, vecToScale.y * uniformScale, vecToScale.z * uniformScale, vecToScale.w * uniformScale);
}
