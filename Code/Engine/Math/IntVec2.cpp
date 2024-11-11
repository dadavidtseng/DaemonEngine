//----------------------------------------------------------------------------------------------------
// IntVec2.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Math/IntVec2.hpp"

#include <cmath>

#include "Engine/Math/MathUtils.hpp"

//----------------------------------------------------------------------------------------------------
IntVec2 IntVec2::ZERO = IntVec2(0, 0);

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
