//----------------------------------------------------------------------------------------------------
// IntVec3.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Math/IntVec3.hpp"
//----------------------------------------------------------------------------------------------------
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/MathUtils.hpp"

//----------------------------------------------------------------------------------------------------
STATIC IntVec3 IntVec3::ZERO         = IntVec3(0, 0, 0);
STATIC IntVec3 IntVec3::ONE          = IntVec3(1, 1, 1);
STATIC IntVec3 IntVec3::NEGATIVE_ONE = IntVec3(-1, -1, -1);

//----------------------------------------------------------------------------------------------------
IntVec3::IntVec3(int const initialX, int const initialY, int const initialZ)
    : x(initialX),
      y(initialY),
      z(initialZ)
{
}

//----------------------------------------------------------------------------------------------------
IntVec3::IntVec3(float const initialX, float const initialY, float const initialZ)
    : x(static_cast<int>(initialX)),
      y(static_cast<int>(initialY)),
      z(static_cast<int>(initialZ))
{
}

//----------------------------------------------------------------------------------------------------
IntVec3::IntVec3(Vec3 const& vec3)
    : x(static_cast<int>(vec3.x)),
      y(static_cast<int>(vec3.y)),
      z(static_cast<int>(vec3.z))
{
}

//----------------------------------------------------------------------------------------------------
float IntVec3::GetLength() const
{
    float const floatX = static_cast<float>(x);
    float const floatY = static_cast<float>(y);
    float const floatZ = static_cast<float>(z);

    return sqrtf(floatX * floatX + floatY * floatY + floatZ * floatZ);
}

//----------------------------------------------------------------------------------------------------
int IntVec3::GetLengthSquared() const
{
    return x * x + y * y + z * z;
}

//----------------------------------------------------------------------------------------------------
int IntVec3::GetTaxicabLength() const
{
    return abs(x) + abs(y) + abs(z);
}

//----------------------------------------------------------------------------------------------------
IntVec2 IntVec3::GetXY() const
{
    return IntVec2(x, y);
}

//----------------------------------------------------------------------------------------------------
void IntVec3::SetFromText(char const* text)
{
    // Use SplitStringOnDelimiter to divide the input text into parts based on the delimiter ','
    StringList const parts = SplitStringOnDelimiter(text, ',');

    // Input must contain exactly three parts; otherwise, reset to default values
    if (parts.size() != 3)
    {
        x = 0;
        y = 0;
        z = 0;
        return;
    }

    // Convert the three parts to integers using atoi
    x = atoi(parts[0].c_str());
    y = atoi(parts[1].c_str());
    z = atoi(parts[2].c_str());
}

//----------------------------------------------------------------------------------------------------
bool IntVec3::operator==(IntVec3 const& compare) const
{
    return
        x == compare.x &&
        y == compare.y &&
        z == compare.z;
}

//----------------------------------------------------------------------------------------------------
bool IntVec3::operator!=(IntVec3 const& compare) const
{
    return
        x != compare.x ||
        y != compare.y ||
        z != compare.z;
}

//----------------------------------------------------------------------------------------------------
bool IntVec3::operator<(IntVec3 const& compare) const
{
    if (x != compare.x) return x < compare.x;
    if (y != compare.y) return y < compare.y;
    return z < compare.z;
}

//----------------------------------------------------------------------------------------------------
IntVec3 const IntVec3::operator+(IntVec3 const& vecToAdd) const
{
    return IntVec3(x + vecToAdd.x, y + vecToAdd.y, z + vecToAdd.z);
}

//----------------------------------------------------------------------------------------------------
IntVec3 const IntVec3::operator-(IntVec3 const& vecToSubtract) const
{
    return IntVec3(x - vecToSubtract.x, y - vecToSubtract.y, z - vecToSubtract.z);
}

//----------------------------------------------------------------------------------------------------
IntVec3 IntVec3::operator-() const
{
    return IntVec3(-x, -y, -z);
}

//----------------------------------------------------------------------------------------------------
IntVec3 const IntVec3::operator*(int const uniformScale) const
{
    return IntVec3(x * uniformScale, y * uniformScale, z * uniformScale);
}

//----------------------------------------------------------------------------------------------------
void IntVec3::operator+=(IntVec3 const& vecToAdd)
{
    x += vecToAdd.x;
    y += vecToAdd.y;
    z += vecToAdd.z;
}

//----------------------------------------------------------------------------------------------------
void IntVec3::operator-=(IntVec3 const& vecToSubtract)
{
    x -= vecToSubtract.x;
    y -= vecToSubtract.y;
    z -= vecToSubtract.z;
}

//----------------------------------------------------------------------------------------------------
void IntVec3::operator*=(int const uniformScale)
{
    x *= uniformScale;
    y *= uniformScale;
    z *= uniformScale;
}

//----------------------------------------------------------------------------------------------------
IntVec3& IntVec3::operator=(IntVec3 const& copyFrom)
{
    // Self-assignment check
    if (this != &copyFrom)
    {
        x = copyFrom.x;
        y = copyFrom.y;
        z = copyFrom.z;
    }

    return *this; // Return reference to this object
}

//----------------------------------------------------------------------------------------------------
IntVec3 const operator*(int const uniformScale, IntVec3 const& vecToScale)
{
    return IntVec3(uniformScale * vecToScale.x, uniformScale * vecToScale.y, uniformScale * vecToScale.z);
}

//----------------------------------------------------------------------------------------------------
IntVec3 Interpolate(IntVec3 const& start, IntVec3 const& end, float const t)
{
    IntVec3 result;
    result.x = static_cast<int>(std::round(Interpolate(static_cast<float>(start.x), static_cast<float>(end.x), t)));
    result.y = static_cast<int>(std::round(Interpolate(static_cast<float>(start.y), static_cast<float>(end.y), t)));
    result.z = static_cast<int>(std::round(Interpolate(static_cast<float>(start.z), static_cast<float>(end.z), t)));

    return result;
}