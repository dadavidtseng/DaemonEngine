//----------------------------------------------------------------------------------------------------
// InputSystem.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Input/InputSystem.hpp"

#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Renderer/Window.hpp"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

//----------------------------------------------------------------------------------------------------
unsigned char const NUMCODE_0             = 0x30;
unsigned char const NUMCODE_1             = 0x31;
unsigned char const NUMCODE_2             = 0x32;
unsigned char const NUMCODE_3             = 0x33;
unsigned char const NUMCODE_4             = 0x34;
unsigned char const NUMCODE_5             = 0x35;
unsigned char const NUMCODE_6             = 0x36;
unsigned char const NUMCODE_7             = 0x37;
unsigned char const NUMCODE_8             = 0x38;
unsigned char const NUMCODE_9             = 0x39;
unsigned char const KEYCODE_A             = 0x41;
unsigned char const KEYCODE_B             = 0x42;
unsigned char const KEYCODE_C             = 0x43;
unsigned char const KEYCODE_D             = 0x44;
unsigned char const KEYCODE_E             = 0x45;
unsigned char const KEYCODE_F             = 0x46;
unsigned char const KEYCODE_G             = 0x47;
unsigned char const KEYCODE_H             = 0x48;
unsigned char const KEYCODE_I             = 0x49;
unsigned char const KEYCODE_J             = 0x4A;
unsigned char const KEYCODE_K             = 0x4B;
unsigned char const KEYCODE_L             = 0x4C;
unsigned char const KEYCODE_M             = 0x4D;
unsigned char const KEYCODE_N             = 0x4E;
unsigned char const KEYCODE_O             = 0x4F;
unsigned char const KEYCODE_P             = 0x50;
unsigned char const KEYCODE_Q             = 0x51;
unsigned char const KEYCODE_R             = 0x52;
unsigned char const KEYCODE_S             = 0x53;
unsigned char const KEYCODE_T             = 0x54;
unsigned char const KEYCODE_U             = 0x55;
unsigned char const KEYCODE_V             = 0x56;
unsigned char const KEYCODE_W             = 0x57;
unsigned char const KEYCODE_X             = 0x58;
unsigned char const KEYCODE_Y             = 0x59;
unsigned char const KEYCODE_Z             = 0x5A;
unsigned char const KEYCODE_F1            = VK_F1;
unsigned char const KEYCODE_F2            = VK_F2;
unsigned char const KEYCODE_F3            = VK_F3;
unsigned char const KEYCODE_F4            = VK_F4;
unsigned char const KEYCODE_F5            = VK_F5;
unsigned char const KEYCODE_F6            = VK_F6;
unsigned char const KEYCODE_F7            = VK_F7;
unsigned char const KEYCODE_F8            = VK_F8;
unsigned char const KEYCODE_F9            = VK_F9;
unsigned char const KEYCODE_F10           = VK_F10;
unsigned char const KEYCODE_F11           = VK_F11;
unsigned char const KEYCODE_F12           = VK_F12;
unsigned char const KEYCODE_F13           = VK_F13;
unsigned char const KEYCODE_F14           = VK_F14;
unsigned char const KEYCODE_F15           = VK_F15;
unsigned char const KEYCODE_ESC           = VK_ESCAPE;
unsigned char const KEYCODE_UPARROW       = VK_UP;
unsigned char const KEYCODE_DOWNARROW     = VK_DOWN;
unsigned char const KEYCODE_LEFTARROW     = VK_LEFT;
unsigned char const KEYCODE_RIGHTARROW    = VK_RIGHT;
unsigned char const KEYCODE_ENTER         = VK_RETURN;
unsigned char const KEYCODE_SPACE         = VK_SPACE;
unsigned char const KEYCODE_BACKSPACE     = VK_BACK;
unsigned char const KEYCODE_LEFT_MOUSE    = VK_LBUTTON;
unsigned char const KEYCODE_RIGHT_MOUSE   = VK_RBUTTON;
unsigned char const KEYCODE_INSERT        = VK_INSERT;
unsigned char const KEYCODE_DELETE        = VK_DELETE;
unsigned char const KEYCODE_HOME          = VK_HOME;
unsigned char const KEYCODE_END           = VK_END;
unsigned char const KEYCODE_TILDE         = VK_OEM_3;
unsigned char const KEYCODE_LEFT_BRACKET  = VK_OEM_4;
unsigned char const KEYCODE_RIGHT_BRACKET = VK_OEM_6;
unsigned char const KEYCODE_SHIFT         = VK_SHIFT;
unsigned char const KEYCODE_CONTROL       = VK_CONTROL;

//----------------------------------------------------------------------------------------------------
InputSystem* g_theInput = nullptr;

//----------------------------------------------------------------------------------------------------
InputSystem::InputSystem(sInputSystemConfig const& config)
{
    m_inputConfig = config;
}

//----------------------------------------------------------------------------------------------------
void InputSystem::Startup()
{
    g_theEventSystem->SubscribeEventCallbackFunction("OnWindowKeyPressed", OnWindowKeyPressed);
    g_theEventSystem->SubscribeEventCallbackFunction("OnWindowKeyReleased", OnWindowKeyReleased);

    for (int controllerIndex = 0; controllerIndex < NUM_XBOX_CONTROLLERS; ++controllerIndex)
    {
        m_controllers[controllerIndex].m_id = controllerIndex;
    }
}

//----------------------------------------------------------------------------------------------------
void InputSystem::Shutdown()
{
}

//----------------------------------------------------------------------------------------------------
void InputSystem::BeginFrame()
{
    for (int controllerIndex = 0; controllerIndex < NUM_XBOX_CONTROLLERS; ++controllerIndex)
    {
        m_controllers[controllerIndex].Update();
    }

    // Check if our hidden mode matches Windows cursor state
    static bool cursorHidden     = false;
    bool const  shouldHideCursor = m_cursorState.m_cursorMode == CursorMode::FPS;

    if (shouldHideCursor != cursorHidden)
    {
        while (ShowCursor(!shouldHideCursor) >= 0 && shouldHideCursor)
        {
        }
        while (ShowCursor(!shouldHideCursor) < 0 && !shouldHideCursor)
        {
        }
        cursorHidden = shouldHideCursor;
    }

    // Save off the previous cursor client position from last frame.
    IntVec2 const previousCursorClientPosition = m_cursorState.m_cursorClientPosition;

    // Get the current cursor client position from Windows.
    POINT currentCursorPosition;
    GetCursorPos(&currentCursorPosition);
    ScreenToClient(GetActiveWindow(), &currentCursorPosition);
    m_cursorState.m_cursorClientPosition.x = currentCursorPosition.x;
    m_cursorState.m_cursorClientPosition.y = currentCursorPosition.y;

    // If we are in relative mode
    if (m_cursorState.m_cursorMode == CursorMode::FPS)
    {
        // Calculate our cursor client delta
        m_cursorState.m_cursorClientDelta = m_cursorState.m_cursorClientPosition - previousCursorClientPosition;

        // Set the Windows cursor position back to the center of our client region
        int const clientX = Window::s_mainWindow->GetClientDimensions().x;
        int const clientY = Window::s_mainWindow->GetClientDimensions().y;
        POINT     center  = {clientX / 2, clientY / 2};
        ClientToScreen(GetActiveWindow(), &center);
        SetCursorPos(center.x, center.y);

        // Get the Windows cursor position again and save that as our current cursor client position.
        POINT currentCursorPositionX;
        GetCursorPos(&currentCursorPositionX);
        ScreenToClient(GetActiveWindow(), &currentCursorPositionX);
        m_cursorState.m_cursorClientPosition.x = currentCursorPositionX.x;
        m_cursorState.m_cursorClientPosition.y = currentCursorPositionX.y;
    }
    else
    {
        m_cursorState.m_cursorClientDelta = IntVec2::ZERO;
    }
}

//----------------------------------------------------------------------------------------------------
void InputSystem::EndFrame()
{
    //Copy current-frame key state to "previous" in preparation of new WM_KEYDOWN, etc. messages
    for (int keyCode = 0; keyCode < NUM_KEYCODES; ++keyCode)
    {
        m_keyStates[keyCode].m_wasKeyDownLastFrame = m_keyStates[keyCode].m_isKeyDown;
    }
}

//----------------------------------------------------------------------------------------------------
bool InputSystem::WasKeyJustPressed(const unsigned char keyCode) const
{
    return
        m_keyStates[keyCode].m_isKeyDown &&
        !m_keyStates[keyCode].m_wasKeyDownLastFrame;
}

//----------------------------------------------------------------------------------------------------
bool InputSystem::WasKeyJustReleased(const unsigned char keyCode) const
{
    return
        !m_keyStates[keyCode].m_isKeyDown &&
        m_keyStates[keyCode].m_wasKeyDownLastFrame;
}

//----------------------------------------------------------------------------------------------------
bool InputSystem::IsKeyDown(const unsigned char keyCode) const
{
    return m_keyStates[keyCode].m_isKeyDown;
}

//----------------------------------------------------------------------------------------------------
void InputSystem::HandleKeyPressed(const unsigned char keyCode)
{
    m_keyStates[keyCode].m_isKeyDown = true;
}

//----------------------------------------------------------------------------------------------------
void InputSystem::HandleKeyReleased(unsigned char const keyCode)
{
    m_keyStates[keyCode].m_isKeyDown = false;
}

//----------------------------------------------------------------------------------------------------
XboxController const& InputSystem::GetController(int const controllerID)
{
    return m_controllers[controllerID];
}

//----------------------------------------------------------------------------------------------------
// In pointer mode, the cursor should be visible, freely able to move, and not
// locked to the window. In FPS mode, the cursor should be hidden, reset to the
// center of the window each frame, and record the delta each frame.
//
void InputSystem::SetCursorMode(CursorMode const mode)
{
    m_cursorState.m_cursorMode = mode;
}

//----------------------------------------------------------------------------------------------------
// Returns the current frame cursor delta in pixels, relative to the client
// region. This is how much the cursor moved last frame before it was reset
// to the center of the screen. Only valid in FPS mode, will be zero otherwise.
//
Vec2 InputSystem::GetCursorClientDelta() const
{
    switch (m_cursorState.m_cursorMode)
    {
    case CursorMode::POINTER: return Vec2::ZERO;
    case CursorMode::FPS: return static_cast<Vec2>(m_cursorState.m_cursorClientDelta);
    }

    return Vec2::ZERO;
}

//----------------------------------------------------------------------------------------------------
// Returns the cursor position, in pixels relative to the client region.
//
Vec2 InputSystem::GetCursorClientPosition() const
{
    return static_cast<Vec2>(m_cursorState.m_cursorClientPosition);
}

//----------------------------------------------------------------------------------------------------
// Returns the cursor position, normalized to the range [0, 1], relative
// to the client region, with the y-axis inverted to map from Windows
// conventions to game screen camera conventions.
//
Vec2 InputSystem::GetCursorNormalizedPosition() const
{
    RECT clientRect;
    GetClientRect(GetActiveWindow(), &clientRect);

    Vec2 const  clientPosition = Vec2(m_cursorState.m_cursorClientPosition);
    float const normalizedX    = clientPosition.x / static_cast<float>(clientRect.right);
    float const normalizedY    = clientPosition.y / static_cast<float>(clientRect.bottom);

    Vec2 cursorPosition = Vec2(normalizedX, 1.f - normalizedY);

    return cursorPosition;
}

//----------------------------------------------------------------------------------------------------
STATIC bool InputSystem::OnWindowKeyPressed(EventArgs& args)
{
    if (g_theDevConsole == nullptr)
    {
        ERROR_RECOVERABLE("g_theDevConsole is nullptr")
    }

    if (g_theInput == nullptr)
    {
        return false;
    }

    int const           value   = args.GetValue("OnWindowKeyPressed", -1);
    unsigned char const keyCode = static_cast<unsigned char>(value);
    g_theInput->HandleKeyPressed(keyCode);

    return true;
}

//----------------------------------------------------------------------------------------------------
STATIC bool InputSystem::OnWindowKeyReleased(EventArgs& args)
{
    if (g_theDevConsole == nullptr)
    {
        return false;
    }

    if (g_theInput == nullptr)
    {
        return false;
    }

    // if (g_theDevConsole->IsOpen())
    // {
    //     return false;
    // }

    int const           value   = args.GetValue("OnWindowKeyReleased", -1);
    unsigned char const keyCode = static_cast<unsigned char>(value);
    g_theInput->HandleKeyReleased(keyCode);

    return true;
}
