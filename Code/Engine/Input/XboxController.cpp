//----------------------------------------------------------------------------------------------------
// XboxController.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Input/XboxController.hpp"

#include "Engine/Math/MathUtils.hpp"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Xinput.h>
#pragma comment(lib, "xinput")

//----------------------------------------------------------------------------------------------------
float constexpr XBOX_JOYSTICK_INNER_DEAD_ZONE = 0.35f;
float constexpr XBOX_JOYSTICK_OUTER_DEAD_ZONE = 0.95f;

//----------------------------------------------------------------------------------------------------
XboxController::XboxController()
{
    m_leftStick.SetDeadZoneThresholds(XBOX_JOYSTICK_INNER_DEAD_ZONE, XBOX_JOYSTICK_OUTER_DEAD_ZONE);
    m_rightStick.SetDeadZoneThresholds(XBOX_JOYSTICK_INNER_DEAD_ZONE, XBOX_JOYSTICK_OUTER_DEAD_ZONE);
}

//----------------------------------------------------------------------------------------------------
XboxController::XboxController(int const id)
    : m_id(id)
{
    m_leftStick.SetDeadZoneThresholds(XBOX_JOYSTICK_INNER_DEAD_ZONE, XBOX_JOYSTICK_OUTER_DEAD_ZONE);
    m_rightStick.SetDeadZoneThresholds(XBOX_JOYSTICK_INNER_DEAD_ZONE, XBOX_JOYSTICK_OUTER_DEAD_ZONE);
}

//----------------------------------------------------------------------------------------------------
bool XboxController::IsConnected() const
{
    return m_isConnected;
}

//----------------------------------------------------------------------------------------------------
int XboxController::GetControllerID() const
{
    return m_id;
}

//----------------------------------------------------------------------------------------------------
AnalogJoystick const& XboxController::GetLeftStick() const
{
    return m_leftStick;
}

//----------------------------------------------------------------------------------------------------
AnalogJoystick const& XboxController::GetRightStick() const
{
    return m_rightStick;
}

//----------------------------------------------------------------------------------------------------
float XboxController::GetLeftTrigger() const
{
    return m_leftTrigger;
}

//----------------------------------------------------------------------------------------------------
float XboxController::GetRightTrigger() const
{
    return m_rightTrigger;
}

//----------------------------------------------------------------------------------------------------
sKeyButtonState const& XboxController::GetButton(eXboxButtonID const buttonID) const
{
    return m_buttons[static_cast<int>(buttonID)];
}

//----------------------------------------------------------------------------------------------------
bool XboxController::IsButtonDown(eXboxButtonID const buttonID) const
{
    return m_buttons[static_cast<int>(buttonID)].m_isKeyDown;
}

//----------------------------------------------------------------------------------------------------
bool XboxController::WasButtonJustPressed(eXboxButtonID const buttonID) const
{
    return
        m_buttons[static_cast<int>(buttonID)].m_isKeyDown &&
        !m_buttons[static_cast<int>(buttonID)].m_wasKeyDownLastFrame;
}

//----------------------------------------------------------------------------------------------------
bool XboxController::WasButtonJustReleased(eXboxButtonID const buttonID) const
{
    return
        !m_buttons[static_cast<int>(buttonID)].m_isKeyDown &&
        m_buttons[static_cast<int>(buttonID)].m_wasKeyDownLastFrame;
}

//----------------------------------------------------------------------------------------------------
void XboxController::SetVibration(unsigned short const leftSpeed,
                                  unsigned short const rightSpeed) const
{
    XINPUT_VIBRATION xVibration;
    xVibration.wLeftMotorSpeed  = leftSpeed;
    xVibration.wRightMotorSpeed = rightSpeed;
    XInputSetState(m_id, &xVibration);
}

//----------------------------------------------------------------------------------------------------
void XboxController::Update()
{
    // Read raw controller state via XInput API
    XINPUT_STATE xboxControllerState = {};
    DWORD const  errorStatus         = XInputGetState(m_id, &xboxControllerState);

    if (errorStatus != ERROR_SUCCESS)
    {
        Reset();
        m_isConnected = false;
        return;
    }

    m_isConnected = true;

    // Update internal data structure(s) based on raw controller state
    XINPUT_GAMEPAD const& state = xboxControllerState.Gamepad;
    UpdateJoystick(m_leftStick, state.sThumbLX, state.sThumbLY);
    UpdateJoystick(m_rightStick, state.sThumbRX, state.sThumbRY);

    UpdateTrigger(m_leftTrigger, state.bLeftTrigger);
    UpdateTrigger(m_rightTrigger, state.bRightTrigger);

    UpdateButton(XBOX_BUTTON_A, state.wButtons, XINPUT_GAMEPAD_A);
    UpdateButton(XBOX_BUTTON_B, state.wButtons, XINPUT_GAMEPAD_B);
    UpdateButton(XBOX_BUTTON_X, state.wButtons, XINPUT_GAMEPAD_X);
    UpdateButton(XBOX_BUTTON_Y, state.wButtons, XINPUT_GAMEPAD_Y);
    UpdateButton(XBOX_BUTTON_BACK, state.wButtons, XINPUT_GAMEPAD_BACK);
    UpdateButton(XBOX_BUTTON_START, state.wButtons, XINPUT_GAMEPAD_START);
    UpdateButton(XBOX_BUTTON_LSHOULDER, state.wButtons, XINPUT_GAMEPAD_LEFT_SHOULDER);
    UpdateButton(XBOX_BUTTON_RSHOULDER, state.wButtons, XINPUT_GAMEPAD_RIGHT_SHOULDER);
    UpdateButton(XBOX_BUTTON_LTHUMB, state.wButtons, XINPUT_GAMEPAD_LEFT_THUMB);
    UpdateButton(XBOX_BUTTON_RTHUMB, state.wButtons, XINPUT_GAMEPAD_RIGHT_THUMB);
    UpdateButton(XBOX_BUTTON_DPAD_RIGHT, state.wButtons, XINPUT_GAMEPAD_DPAD_RIGHT);
    UpdateButton(XBOX_BUTTON_DPAD_UP, state.wButtons, XINPUT_GAMEPAD_DPAD_UP);
    UpdateButton(XBOX_BUTTON_DPAD_LEFT, state.wButtons, XINPUT_GAMEPAD_DPAD_LEFT);
    UpdateButton(XBOX_BUTTON_DPAD_DOWN, state.wButtons, XINPUT_GAMEPAD_DPAD_DOWN);
}

//----------------------------------------------------------------------------------------------------
void XboxController::Reset()
{
    for (sKeyButtonState& button : m_buttons)
    {
        button.m_isKeyDown           = false;
        button.m_wasKeyDownLastFrame = false;
    }

    m_id           = -1;
    m_isConnected  = false;
    m_leftTrigger  = 0.f;
    m_rightTrigger = 0.f;
    m_leftStick.Reset();
    m_rightStick.Reset();
}

//----------------------------------------------------------------------------------------------------
void XboxController::UpdateJoystick(AnalogJoystick& out_joystick,
                                    short const     rawX,
                                    short const     rawY)
{
    float const rawNormalizedX = RangeMap(rawX, -32768, 32767, -1, 1);
    float const rawNormalizedY = RangeMap(rawY, -32768, 32767, -1, 1);

    out_joystick.UpdatePosition(rawNormalizedX, rawNormalizedY);
}

//----------------------------------------------------------------------------------------------------
void XboxController::UpdateTrigger(float&              out_triggerValue,
                                   unsigned char const rawValue)
{
    out_triggerValue = rawValue;
}

//----------------------------------------------------------------------------------------------------
void XboxController::UpdateButton(eXboxButtonID const   buttonID,
                                  unsigned short const buttonFlags,
                                  unsigned short const buttonFlag)
{
    sKeyButtonState& button = m_buttons[buttonID];

    button.m_wasKeyDownLastFrame = button.m_isKeyDown;
    button.m_isKeyDown           = (buttonFlags & buttonFlag) != 0;
}
