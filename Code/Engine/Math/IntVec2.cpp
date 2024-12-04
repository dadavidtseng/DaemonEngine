//----------------------------------------------------------------------------------------------------
// IntVec2.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Math/IntVec2.hpp"

#include <cmath>

#include "Engine/Core/StringUtils.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/Vec2.hpp"

//----------------------------------------------------------------------------------------------------
IntVec2 IntVec2::ZERO = IntVec2(0, 0);
IntVec2 IntVec2::ONE  = IntVec2(1, 1);

//----------------------------------------------------------------------------------------------------
IntVec2::IntVec2(int const initialX, int const initialY)
    : x(initialX),
      y(initialY)
{
}

//----------------------------------------------------------------------------------------------------
IntVec2::IntVec2(float const initialX, float const initialY)
    : x(static_cast<int>(initialX)),
      y(static_cast<int>(initialY))
{
}

//----------------------------------------------------------------------------------------------------
IntVec2::IntVec2(Vec2 const& vec2)
    : x(static_cast<int>(vec2.x)),
      y(static_cast<int>(vec2.y))
{
}

//----------------------------------------------------------------------------------------------------
float IntVec2::GetLength() const
{
    float const floatX = static_cast<float>(x);
    float const floatY = static_cast<float>(y);

    return sqrtf(floatX * floatX + floatY * floatY);
}

//----------------------------------------------------------------------------------------------------
int IntVec2::GetLengthSquared() const
{
    return x * x + y * y;
}

//----------------------------------------------------------------------------------------------------
int IntVec2::GetTaxicabLength() const
{
    return abs(x) + abs(y);
}

//----------------------------------------------------------------------------------------------------
float IntVec2::GetOrientationDegrees() const
{
    float const floatX  = static_cast<float>(x);
    float const floatY  = static_cast<float>(y);
    float const degrees = Atan2Degrees(floatY, floatX);

    return degrees;
}

//----------------------------------------------------------------------------------------------------
float IntVec2::GetOrientationRadians() const
{
    float const degrees = GetOrientationDegrees();

    return ConvertDegreesToRadians(degrees);
}

//----------------------------------------------------------------------------------------------------
IntVec2 IntVec2::GetRotated90Degrees() const
{
    return IntVec2(-y, x);
}

//----------------------------------------------------------------------------------------------------
IntVec2 IntVec2::GetRotatedMinus90Degrees() const
{
    return IntVec2(y, -x);
}

//----------------------------------------------------------------------------------------------------
void IntVec2::Rotate90Degrees()
{
    int const oldX = x;

    x = -y;
    y = oldX;
}

//----------------------------------------------------------------------------------------------------
void IntVec2::RotateMinus90Degrees()
{
    int const oldX = x;

    x = y;
    y = -oldX;
}

//----------------------------------------------------------------------------------------------------
void IntVec2::SetFromText(char const* text)
{
    // Use SplitStringOnDelimiter to divide the input text into parts based on the delimiter ','
    Strings const parts = SplitStringOnDelimiter(text, ',');

    // Input must contain exactly two parts; otherwise, reset to default values
    if (parts.size() != 2)
    {
        x = 0;
        y = 0;
        return;
    }

    // Convert the two parts to integers using atoi
    x = atoi(parts[0].c_str());
    y = atoi(parts[1].c_str());
}

//----------------------------------------------------------------------------------------------------
bool IntVec2::operator==(IntVec2 const& compare) const
{
    return
        x == compare.x &&
        y == compare.y;
}

//----------------------------------------------------------------------------------------------------
bool IntVec2::operator!=(IntVec2 const& compare) const
{
    return
        x != compare.x ||
        y != compare.y;
}

//----------------------------------------------------------------------------------------------------
IntVec2& IntVec2::operator=(IntVec2 const& copyFrom)
{
    // Self-assignment check
    if (this != &copyFrom)
    {
        x = copyFrom.x;
        y = copyFrom.y;
    }

    return *this; // Return reference to this object
}

//----------------------------------------------------------------------------------------------------
IntVec2 const IntVec2::operator+(IntVec2 const& vecToAdd) const
{
    return IntVec2(x + vecToAdd.x, y + vecToAdd.y);
}

//----------------------------------------------------------------------------------------------------
IntVec2 const IntVec2::operator-(IntVec2 const& vecToSubtract) const
{
    return IntVec2(x - vecToSubtract.x, y - vecToSubtract.y);
}
