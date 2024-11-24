//----------------------------------------------------------------------------------------------------
// IntVec2.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Math/IntVec2.hpp"

#include <cmath>

#include "Engine/Core/StringUtils.hpp"
#include "Engine/Math/MathUtils.hpp"

//----------------------------------------------------------------------------------------------------
IntVec2 IntVec2::ZERO = IntVec2(0, 0);
IntVec2 IntVec2::ONE  = IntVec2(1, 1);

//----------------------------------------------------------------------------------------------------
IntVec2::IntVec2(const int initialX, const int initialY)
    : x(initialX),
      y(initialY)
{
}

//----------------------------------------------------------------------------------------------------
float IntVec2::GetLength() const
{
    const float floatX = static_cast<float>(x);
    const float floatY = static_cast<float>(y);

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
    const float floatX  = static_cast<float>(x);
    const float floatY  = static_cast<float>(y);
    const float degrees = Atan2Degrees(floatY, floatX);

    return degrees;
}

//----------------------------------------------------------------------------------------------------
float IntVec2::GetOrientationRadians() const
{
    const float degrees = GetOrientationDegrees();

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
    const int oldX = x;

    x = -y;
    y = oldX;
}

void IntVec2::RotateMinus90Degrees()
{
    const int oldX = x;

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
        x - compare.x &&
        y - compare.y;
}

//----------------------------------------------------------------------------------------------------
IntVec2& IntVec2::operator=(const IntVec2& copyFrom)
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
IntVec2 IntVec2::operator+(IntVec2 const& vecToAdd) const
{
    return IntVec2(x + vecToAdd.x, y + vecToAdd.y);
}

//----------------------------------------------------------------------------------------------------
IntVec2 IntVec2::operator-(IntVec2 const& vecToSubtract) const
{
    return IntVec2(x - vecToSubtract.x, y - vecToSubtract.y);
}
