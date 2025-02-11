//----------------------------------------------------------------------------------------------------
// InputSystem.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Input/InputSystem.hpp"
#include <cstdio>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

//----------------------------------------------------------------------------------------------------
unsigned char const KEYCODE_A           = 0x41;
unsigned char const KEYCODE_B           = 0x42;
unsigned char const KEYCODE_C           = 0x43;
unsigned char const KEYCODE_D           = 0x44;
unsigned char const KEYCODE_E           = 0x45;
unsigned char const KEYCODE_F           = 0x46;
unsigned char const KEYCODE_G           = 0x47;
unsigned char const KEYCODE_H           = 0x48;
unsigned char const KEYCODE_I           = 0x49;
unsigned char const KEYCODE_J           = 0x4A;
unsigned char const KEYCODE_K           = 0x4B;
unsigned char const KEYCODE_L           = 0x4C;
unsigned char const KEYCODE_M           = 0x4D;
unsigned char const KEYCODE_N           = 0x4E;
unsigned char const KEYCODE_O           = 0x4F;
unsigned char const KEYCODE_P           = 0x50;
unsigned char const KEYCODE_Q           = 0x51;
unsigned char const KEYCODE_R           = 0x52;
unsigned char const KEYCODE_S           = 0x53;
unsigned char const KEYCODE_T           = 0x54;
unsigned char const KEYCODE_U           = 0x55;
unsigned char const KEYCODE_V           = 0x56;
unsigned char const KEYCODE_W           = 0x57;
unsigned char const KEYCODE_X           = 0x58;
unsigned char const KEYCODE_Y           = 0x59;
unsigned char const KEYCODE_Z           = 0x5A;
unsigned char const KEYCODE_F1          = VK_F1;
unsigned char const KEYCODE_F2          = VK_F2;
unsigned char const KEYCODE_F3          = VK_F3;
unsigned char const KEYCODE_F4          = VK_F4;
unsigned char const KEYCODE_F5          = VK_F5;
unsigned char const KEYCODE_F6          = VK_F6;
unsigned char const KEYCODE_F7          = VK_F7;
unsigned char const KEYCODE_F8          = VK_F8;
unsigned char const KEYCODE_F9          = VK_F9;
unsigned char const KEYCODE_F10         = VK_F10;
unsigned char const KEYCODE_F11         = VK_F11;
unsigned char const KEYCODE_ESC         = VK_ESCAPE;
unsigned char const KEYCODE_UPARROW     = VK_UP;
unsigned char const KEYCODE_DOWNARROW   = VK_DOWN;
unsigned char const KEYCODE_LEFTARROW   = VK_LEFT;
unsigned char const KEYCODE_RIGHTARROW  = VK_RIGHT;
unsigned char const KEYCODE_ENTER       = VK_RETURN;
unsigned char const KEYCODE_SPACE       = VK_SPACE;
unsigned char const KEYCODE_BACKSPACE   = VK_BACK;
unsigned char const KEYCODE_LEFT_MOUSE  = VK_LBUTTON;
unsigned char const KEYCODE_RIGHT_MOUSE = VK_RBUTTON;
unsigned char const KEYCODE_INSERT      = VK_RBUTTON;
unsigned char const KEYCODE_DELETE      = VK_RBUTTON;
unsigned char const KEYCODE_HOME        = VK_RBUTTON;
unsigned char const KEYCODE_END         = VK_RBUTTON;
unsigned char const KEYCODE_TILDE       = 0xC0;

//-----------------------------------------------------------------------------------------------
InputSystem::InputSystem(InputSystemConfig const& config)
{
    m_inputConfig = config;
}

//-----------------------------------------------------------------------------------------------
InputSystem::~InputSystem() = default;

//-----------------------------------------------------------------------------------------------
void InputSystem::Startup()
{
    for (int controllerIndex = 0; controllerIndex < NUM_XBOX_CONTROLLERS; ++controllerIndex)
    {
        m_controllers[controllerIndex].m_id = controllerIndex;
    }
}

//-----------------------------------------------------------------------------------------------
void InputSystem::Shutdown()
{
}

//-----------------------------------------------------------------------------------------------
void InputSystem::BeginFrame()
{
    for (int controllerIndex = 0; controllerIndex < NUM_XBOX_CONTROLLERS; ++controllerIndex)
    {
        m_controllers[controllerIndex].Update();
    }
}

//-----------------------------------------------------------------------------------------------
void InputSystem::EndFrame()
{
    //Copy current-frame key state to "previous" in preparation of new WM_KEYDOWN, etc. messages
    for (int keyCode = 0; keyCode < NUM_KEYCODES; ++keyCode)
    {
        m_keyStates[keyCode].m_wasKeyDownLastFrame = m_keyStates[keyCode].m_isKeyDown;
    }
}

//-----------------------------------------------------------------------------------------------
bool InputSystem::WasKeyJustPressed(const unsigned char keyCode) const
{
    return m_keyStates[keyCode].m_isKeyDown &&
        !m_keyStates[keyCode].m_wasKeyDownLastFrame;
}

//-----------------------------------------------------------------------------------------------
bool InputSystem::WasKeyJustReleased(const unsigned char keyCode) const
{
    return !m_keyStates[keyCode].m_isKeyDown &&
        m_keyStates[keyCode].m_wasKeyDownLastFrame;
}

//-----------------------------------------------------------------------------------------------
bool InputSystem::IsKeyDown(const unsigned char keyCode) const
{
    return m_keyStates[keyCode].m_isKeyDown;
}

//-----------------------------------------------------------------------------------------------
void InputSystem::HandleKeyPressed(const unsigned char keyCode)
{
    m_keyStates[keyCode].m_isKeyDown = true;
}

//-----------------------------------------------------------------------------------------------
void InputSystem::HandleKeyReleased(const unsigned char keyCode)
{
    m_keyStates[keyCode].m_isKeyDown = false;
}

//-----------------------------------------------------------------------------------------------
XboxController const& InputSystem::GetController(const int controllerID)
{
    return m_controllers[controllerID];
}
