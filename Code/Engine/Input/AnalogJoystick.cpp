//----------------------------------------------------------------------------------------------------
// AnalogJoystick.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Input/AnalogJoystick.hpp"
//----------------------------------------------------------------------------------------------------
#include "Engine/Math/MathUtils.hpp"
//----------------------------------------------------------------------------------------------------
#include <cmath>

//----------------------------------------------------------------------------------------------------
Vec2 AnalogJoystick::GetPosition() const
{
    return m_correctedPosition;
}

//----------------------------------------------------------------------------------------------------
float AnalogJoystick::GetMagnitude() const
{
    return m_correctedPosition.GetLength();
}

//----------------------------------------------------------------------------------------------------
float AnalogJoystick::GetOrientationDegrees() const
{
    return m_correctedPosition.GetOrientationDegrees();
}

//----------------------------------------------------------------------------------------------------
Vec2 AnalogJoystick::GetRawUncorrectedPosition() const
{
    return m_rawPosition;
}

//----------------------------------------------------------------------------------------------------
float AnalogJoystick::GetInnerDeadZoneFraction() const
{
    return m_innerDeadZoneFraction;
}

//----------------------------------------------------------------------------------------------------
float AnalogJoystick::GetOuterDeadZoneFraction() const
{
    return m_outerDeadZoneFraction;
}

//----------------------------------------------------------------------------------------------------
void AnalogJoystick::Reset()
{
    m_rawPosition       = Vec2::ZERO;
    m_correctedPosition = Vec2::ZERO;
}

//----------------------------------------------------------------------------------------------------
void AnalogJoystick::SetDeadZoneThresholds(float const normalizedInnerDeadZoneThreshold,
                                           float const normalizedOuterDeadZoneThreshold)
{
    m_innerDeadZoneFraction = normalizedInnerDeadZoneThreshold;
    m_outerDeadZoneFraction = normalizedOuterDeadZoneThreshold;
}

//----------------------------------------------------------------------------------------------------
void AnalogJoystick::UpdatePosition(float const rawNormalizedX,
                                    float const rawNormalizedY)
{
    m_rawPosition.x = rawNormalizedX;
    m_rawPosition.y = rawNormalizedY;

    // Turn to raw polar
    float const length = sqrt(m_rawPosition.x * m_rawPosition.x + m_rawPosition.y * m_rawPosition.y); // x * x + y * y
    float const theta  = Atan2Degrees(m_rawPosition.y, m_rawPosition.x);

    // Clamped polar
    float const lengthClamped = GetClamped(length, m_innerDeadZoneFraction, m_outerDeadZoneFraction);

    // corrected polar
    float const lengthCorrected = RangeMap(lengthClamped, m_innerDeadZoneFraction, m_outerDeadZoneFraction, 0, 1);

    // convert to cardigan
    float const convertX = lengthCorrected * cosf(ConvertDegreesToRadians(theta));
    float const convertY = lengthCorrected * sinf(ConvertDegreesToRadians(theta));

    m_correctedPosition = Vec2(convertX, convertY);
}
