//-----------------------------------------------------------------------------------------------
// FloatRange.hpp
//-----------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------
#pragma once

//-----------------------------------------------------------------------------------------------
struct FloatRange
{
	float m_min = 0.f;
	float m_max = 0.f;

	FloatRange();
	explicit FloatRange(float min, float max);

	// Operator overloads
	bool        operator==(const FloatRange& compare) const;
	bool        operator!=(const FloatRange& compare) const;
	FloatRange& operator=(const FloatRange& copyFrom);

	// Methods
	bool  IsOnRange(float value) const;
	bool  IsOverlappingWith(const FloatRange& other) const;
	void  ExpandToInclude(float value);     // Expands the range to include a given value
	void  ClampToRange(float& value) const; // Clamps a value within the range
	float GetLength() const;                // Gets the length of the range
	float GetMidpoint() const;              // Gets the midpoint of the range

	static FloatRange ZERO;
	static FloatRange ONE;
	static FloatRange ZERO_TO_ONE;
};
