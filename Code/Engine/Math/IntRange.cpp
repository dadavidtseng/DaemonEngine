//----------------------------------------------------------------------------------------------------
// IntRange.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Math/IntRange.hpp"

//----------------------------------------------------------------------------------------------------
IntRange IntRange::ONE         = IntRange(1, 1);
IntRange IntRange::ZERO        = IntRange(0, 0);
IntRange IntRange::ZERO_TO_ONE = IntRange(0, 1);

//----------------------------------------------------------------------------------------------------
IntRange::IntRange(const int min, const int max) : m_min(min),
                                                   m_max(max)
{
}

//----------------------------------------------------------------------------------------------------
bool IntRange::operator==(const IntRange& compare) const
{
	return
		m_min == compare.m_min &&
		m_max == compare.m_max;
}

//----------------------------------------------------------------------------------------------------
bool IntRange::operator!=(const IntRange& compare) const
{
	return
		!(*this == compare);
}

//----------------------------------------------------------------------------------------------------
IntRange& IntRange::operator=(const IntRange& copyFrom)
{
	// Check for self-assignment
	if (this != &copyFrom)
	{
		m_min = copyFrom.m_min;
		m_max = copyFrom.m_max;
	}

	return *this;	// Return the current object by reference (dereference)
}

//----------------------------------------------------------------------------------------------------
bool IntRange::IsOnRange(const int value) const
{
	return
		m_min <= value &&
		value <= m_max;
}

//----------------------------------------------------------------------------------------------------
bool IntRange::IsOverlappingWith(const IntRange& other) const
{
	return
		m_max >= other.m_min &&
		m_min <= other.m_max;
}

//----------------------------------------------------------------------------------------------------
void IntRange::ExpandToInclude(const int value)
{
	if (value < m_min)
	{
		m_min = value;
	}
	else if (value > m_max)
	{
		m_max = value;
	}
}

//----------------------------------------------------------------------------------------------------
void IntRange::ClampToRange(int& value) const
{
	if (value < m_min)
	{
		value = m_min;
	}
	else if (value > m_max)
	{
		value = m_max;
	}
}

//----------------------------------------------------------------------------------------------------
int IntRange::GetLength() const
{
	return
		m_max - m_min;
}

//----------------------------------------------------------------------------------------------------
float IntRange::GetMidpoint() const
{
	return
		static_cast<float>(m_min + m_max) * 0.5f;
}
