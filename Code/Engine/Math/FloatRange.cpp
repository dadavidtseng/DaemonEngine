//----------------------------------------------------------------------------------------------------
// FloatRange.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Math/FloatRange.hpp"

#include <cmath>

//----------------------------------------------------------------------------------------------------
constexpr float EPSILON = 1e-5f;	// Define a small tolerance value

//----------------------------------------------------------------------------------------------------
FloatRange FloatRange::ONE         = FloatRange(1, 1);
FloatRange FloatRange::ZERO        = FloatRange(0, 0);
FloatRange FloatRange::ZERO_TO_ONE = FloatRange(0, 1);

//----------------------------------------------------------------------------------------------------
FloatRange::FloatRange(const float min, const float max)
    : m_min(min),
      m_max(max)
{
}

//----------------------------------------------------------------------------------------------------
FloatRange::FloatRange() = default;

//----------------------------------------------------------------------------------------------------
bool FloatRange::operator==(const FloatRange& compare) const
{
    return
        fabs(m_max - compare.m_max) < EPSILON &&
        fabs(m_min - compare.m_min) < EPSILON;
}

//----------------------------------------------------------------------------------------------------
bool FloatRange::operator!=(const FloatRange& compare) const
{
    return
        fabs(m_max - compare.m_max) >= EPSILON ||
        fabs(m_min - compare.m_min) >= EPSILON;
}

//----------------------------------------------------------------------------------------------------
FloatRange& FloatRange::operator=(const FloatRange& copyFrom)
{
    // Check for self-assignment
    if (this != &copyFrom)
    {
        m_min = copyFrom.m_min;
        m_max = copyFrom.m_max;
    }

    return *this; // Return the current object by reference (dereference)
}

//----------------------------------------------------------------------------------------------------
bool FloatRange::IsOnRange(const float value) const
{
    return
        m_min <= value &&
        value <= m_max;
}

//----------------------------------------------------------------------------------------------------
bool FloatRange::IsOverlappingWith(const FloatRange& other) const
{
    return
        m_max >= other.m_min &&
        m_min <= other.m_max;
}

//----------------------------------------------------------------------------------------------------
void FloatRange::ExpandToInclude(const float value)
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
void FloatRange::ClampToRange(float& value) const
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
/*!
 * @brief Calculates the length of the floating-point range.
 * 
 * This function computes the difference between the maximum and minimum
 * values of the range to determine its length.
 * 
 * @return A float representing the length of the range, calculated as
 *         the difference between the maximum and minimum values.
 */
float FloatRange::GetLength() const
{
    return m_max - m_min;
}

//----------------------------------------------------------------------------------------------------
/*!
 * @brief Calculates the midpoint of the floating-point range.
 * @return A float representing the midpoint value between the minimum and maximum of the range.
 */
float FloatRange::GetMidpoint() const
{
    return (m_min + m_max) * 0.5f;
}

//----------------------------------------------------------------------------------------------------
void FloatRange::StretchToIncludeValue(float const value)
{
    if (value < m_min)
    {
        m_min = value;
    }

    if (value > m_max)
    {
        m_max = value;
    }
}
