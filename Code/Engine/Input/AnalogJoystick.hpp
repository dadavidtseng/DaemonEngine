//----------------------------------------------------------------------------------------------------
// AnalogJoystick.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include "Engine/Math/Vec2.hpp"

//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
/// @brief Analog joystick input handler with dead zone correction and position tracking
///
/// @details Manages analog stick input from game controllers (Xbox, PlayStation, etc.) with
///          automatic dead zone correction. Handles raw joystick input and applies inner/outer
///          dead zone thresholds to provide clean, corrected positional data for gameplay.
///
/// @remark Dead zone correction eliminates joystick drift and provides consistent control response.
/// @remark Inner dead zone removes unintended small movements; outer dead zone ensures full range.
///
/// @warning Raw position values are uncorrected and may exhibit drift - use GetPosition() for gameplay.
/// @see XboxController for typical usage with Xbox 360/One controllers
/// @see https://docs.microsoft.com/en-us/windows/win32/xinput/xinput-and-controller-subtypes
//----------------------------------------------------------------------------------------------------
class AnalogJoystick
{
public:
    //----------------------------------------------------------------------------------------------------
    /// @brief Get the dead zone-corrected joystick position
    ///
    /// @return Vec2 Corrected 2D position in normalized space [-1, 1] for both axes
    ///
    /// @remark This is the primary method for gameplay input - applies full dead zone correction.
    /// @remark Returns Vec2(0,0) when joystick is within inner dead zone threshold.
    ///
    /// @see GetRawUncorrectedPosition() for raw hardware values without correction
    //----------------------------------------------------------------------------------------------------
    Vec2  GetPosition() const;

    //----------------------------------------------------------------------------------------------------
    /// @brief Get the magnitude (length) of the corrected joystick position vector
    ///
    /// @return float Corrected magnitude in range [0, 1] representing distance from center
    ///
    /// @remark Useful for variable speed movement (e.g., walk vs run based on stick push amount).
    /// @remark Returns 0.0f when within inner dead zone, 1.0f at or beyond outer dead zone.
    ///
    /// @see GetPosition() for full 2D corrected position vector
    //----------------------------------------------------------------------------------------------------
    float GetMagnitude() const;

    //----------------------------------------------------------------------------------------------------
    /// @brief Get the orientation angle of the joystick position in degrees
    ///
    /// @return float Angle in degrees [0, 360), measured clockwise from north (up direction)
    ///
    /// @remark Returns 0° for up, 90° for right, 180° for down, 270° for left.
    /// @remark Useful for directional input detection (e.g., 8-way movement, menu navigation).
    ///
    /// @warning Undefined behavior when joystick is at origin (0,0) - check GetMagnitude() first.
    /// @see Vec2::GetOrientationDegrees() for angle calculation implementation
    //----------------------------------------------------------------------------------------------------
    float GetOrientationDegrees() const;

    //----------------------------------------------------------------------------------------------------
    /// @brief Get the raw, uncorrected joystick position directly from hardware
    ///
    /// @return Vec2 Raw 2D position without dead zone correction (may exhibit drift)
    ///
    /// @remark Use for debugging, calibration, or advanced input processing scenarios.
    /// @remark Values typically range [-1, 1] but may not rest at exactly (0, 0) due to drift.
    ///
    /// @warning Do not use for gameplay - hardware drift causes unintended movement.
    /// @see GetPosition() for corrected values suitable for gameplay
    //----------------------------------------------------------------------------------------------------
    Vec2  GetRawUncorrectedPosition() const;

    //----------------------------------------------------------------------------------------------------
    /// @brief Get the configured inner dead zone threshold fraction
    ///
    /// @return float Normalized inner dead zone radius [0, 1] (default: 0.35)
    ///
    /// @remark Inputs below this magnitude are clamped to zero to eliminate drift.
    /// @remark Typical values: 0.20-0.40 depending on hardware quality and user preference.
    ///
    /// @see SetDeadZoneThresholds() for configuration
    //----------------------------------------------------------------------------------------------------
    float GetInnerDeadZoneFraction() const;

    //----------------------------------------------------------------------------------------------------
    /// @brief Get the configured outer dead zone threshold fraction
    ///
    /// @return float Normalized outer dead zone radius [0, 1] (default: 0.95)
    ///
    /// @remark Inputs beyond this magnitude are clamped to 1.0 for consistent max range.
    /// @remark Typical values: 0.90-0.98 to account for hardware manufacturing variance.
    ///
    /// @see SetDeadZoneThresholds() for configuration
    //----------------------------------------------------------------------------------------------------
    float GetOuterDeadZoneFraction() const;

    //----------------------------------------------------------------------------------------------------
    /// @brief Reset joystick state to center position (all values zeroed)
    ///
    /// @remark Called by XboxController during initialization and controller disconnect events.
    /// @remark Clears both raw and corrected position values to Vec2(0, 0).
    ///
    /// @see XboxController::Reset() for typical usage pattern
    //----------------------------------------------------------------------------------------------------
    void Reset();

    //----------------------------------------------------------------------------------------------------
    /// @brief Configure inner and outer dead zone thresholds for correction behavior
    ///
    /// @param normalizedInnerDeadZoneThreshold Inner radius [0, 1] below which input = 0
    /// @param normalizedOuterDeadZoneThreshold Outer radius [0, 1] beyond which input = 1
    ///
    /// @remark Dead zones create a corrective range mapping: [inner, outer] → [0, 1].
    /// @remark Inner threshold eliminates drift; outer threshold ensures full range capability.
    ///
    /// @warning innerThreshold must be less than outerThreshold for correct behavior.
    /// @warning Values outside [0, 1] range may cause undefined behavior.
    ///
    /// @see https://www.gamasutra.com/blogs/JoshSutphin/20130416/190541/Doing_Thumbstick_Dead_Zones_Right.php
    //----------------------------------------------------------------------------------------------------
    void SetDeadZoneThresholds(float normalizedInnerDeadZoneThreshold, float normalizedOuterDeadZoneThreshold);

    //----------------------------------------------------------------------------------------------------
    /// @brief Update joystick position from raw hardware input (called per-frame by XboxController)
    ///
    /// @param rawNormalizedX Raw X-axis input [-1, 1] from hardware (right = positive)
    /// @param rawNormalizedY Raw Y-axis input [-1, 1] from hardware (up = positive)
    ///
    /// @remark Stores raw values and computes corrected position using dead zone thresholds.
    /// @remark Automatically applies radial dead zone correction (not axis-independent).
    ///
    /// @warning Must be called every frame for accurate state tracking.
    /// @see XboxController::Update() for integration pattern
    //----------------------------------------------------------------------------------------------------
    void UpdatePosition(float rawNormalizedX, float rawNormalizedY);

protected:
    Vec2  m_rawPosition;                   ///< Flaky; doesn't rest at zero (or consistently snap to rest position)
    Vec2  m_correctedPosition;             ///< Dead zone-corrected position for gameplay use
    float m_innerDeadZoneFraction = 0.35f; ///< If R < this%, R = 0; "input range start" for corrective range map
    float m_outerDeadZoneFraction = 0.95f; ///< If R > this%, R = 1; "input range end" for corrective range map
};
