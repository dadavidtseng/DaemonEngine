//----------------------------------------------------------------------------------------------------
// FloatRange.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Math/FloatRange.hpp"

#include <cmath>

#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Math/MathUtils.hpp"

//----------------------------------------------------------------------------------------------------
STATIC FloatRange FloatRange::ONE         = FloatRange(1.f, 1.f);
STATIC FloatRange FloatRange::ZERO        = FloatRange(0.f, 0.f);
STATIC FloatRange FloatRange::ZERO_TO_ONE = FloatRange(0.f, 1.f);

//----------------------------------------------------------------------------------------------------
FloatRange::FloatRange(float const min,
                       float const max)
    : m_min(min),
      m_max(max)
{
}

//----------------------------------------------------------------------------------------------------
bool FloatRange::IsOnRange(float const value) const
{
    return
        m_min <= value &&
        value <= m_max;
}

//----------------------------------------------------------------------------------------------------
bool FloatRange::IsOverlappingWith(FloatRange const& other) const
{
    return
        m_max >= other.m_min &&
        m_min <= other.m_max;
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
void FloatRange::ExpandToInclude(float const value)
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
void FloatRange::SetFromText(char const* text)
{
    // Use SplitStringOnDelimiter to divide the input text into parts based on the delimiter ','
    StringList const parts = SplitStringOnDelimiter(text, ',');

    // Input must contain exactly two parts; otherwise, reset to default values
    if (parts.size() != 2)
    {
        m_min = 0.f;
        m_max = 0.f;

        return;
    }

    // Convert the two parts to floats using atof
    m_min = static_cast<float>(atof(parts[0].c_str()));
    m_max = static_cast<float>(atof(parts[1].c_str()));
}

//----------------------------------------------------------------------------------------------------
bool FloatRange::operator==(FloatRange const& compare) const
{
    return
        fabs(m_max - compare.m_max) < FLOAT_MIN &&
        fabs(m_min - compare.m_min) < FLOAT_MIN;
}

//----------------------------------------------------------------------------------------------------
bool FloatRange::operator!=(FloatRange const& compare) const
{
    return
        fabs(m_max - compare.m_max) >= FLOAT_MIN ||
        fabs(m_min - compare.m_min) >= FLOAT_MIN;
}

//----------------------------------------------------------------------------------------------------
FloatRange& FloatRange::operator=(FloatRange const& copyFrom)
{
    // Check for self-assignment
    if (this != &copyFrom)
    {
        m_min = copyFrom.m_min;
        m_max = copyFrom.m_max;
    }

    return *this; // Return the current object by reference (dereference)
}
