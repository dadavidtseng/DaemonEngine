//-----------------------------------------------------------------------------------------------
// IntRange.hpp
//-----------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------
#pragma once

//-----------------------------------------------------------------------------------------------
struct IntRange
{
	int m_min = 0;
	int m_max = 0;

	IntRange() = default;
	explicit IntRange(int min, int max);

	// Operator overloads
	bool      operator==(const IntRange& compare) const;
	bool      operator!=(const IntRange& compare) const;
	IntRange& operator=(const IntRange& copyFrom);

	// Methods
	bool  IsOnRange(int value) const;
	bool  IsOverlappingWith(const IntRange& other) const;
	void  ExpandToInclude(int value); // Expands the range to include a given value
	void  ClampToRange(int& value) const; // Clamps a value within the range
	int   GetLength() const; // Gets the length of the range
	float GetMidpoint() const; // Gets the midpoint of the range as a float

	// Constants
	static IntRange ZERO;
	static IntRange ONE;
	static IntRange ZERO_TO_ONE;
};

//-----------------------------------------------------------------------------------------------
