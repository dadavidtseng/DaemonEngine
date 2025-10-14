//----------------------------------------------------------------------------------------------------
// InputSystem.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Input/InputSystem.hpp"
//----------------------------------------------------------------------------------------------------
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/LogSubsystem.hpp"
#include "Engine/Platform/Window.hpp"
#include "Engine/Core/StringUtils.hpp"
//----------------------------------------------------------------------------------------------------
#include <thread>
#include <chrono>
//----------------------------------------------------------------------------------------------------
InputSystem* g_input = nullptr;

//----------------------------------------------------------------------------------------------------
InputSystem::InputSystem(sInputSystemConfig const& config)
    : m_config(config)
{
}

//----------------------------------------------------------------------------------------------------
void InputSystem::Startup()
{
    SubscribeEventCallbackFunction("OnWindowKeyPressed", OnWindowKeyPressed);
    SubscribeEventCallbackFunction("OnWindowKeyReleased", OnWindowKeyReleased);

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
    bool const  shouldHideCursor = m_cursorState.m_cursorMode == eCursorMode::FPS;

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
    if (m_cursorState.m_cursorMode == eCursorMode::FPS)
    {
        // Calculate our cursor client delta
        m_cursorState.m_cursorClientDelta = m_cursorState.m_cursorClientPosition - previousCursorClientPosition;

        // Set the Windows cursor position back to the center of our client region
        int const clientX = (int)Window::s_mainWindow->GetClientDimensions().x;
        int const clientY = (int)Window::s_mainWindow->GetClientDimensions().y;
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
    for (sKeyButtonState& keyState : m_keyStates)
    {
        keyState.m_wasKeyDownLastFrame = keyState.m_isKeyDown;
    }
}

//----------------------------------------------------------------------------------------------------
bool InputSystem::WasKeyJustPressed(unsigned char const keyCode) const
{
    return
        m_keyStates[keyCode].m_isKeyDown &&
        !m_keyStates[keyCode].m_wasKeyDownLastFrame;
}

//----------------------------------------------------------------------------------------------------
bool InputSystem::WasKeyJustReleased(unsigned char const keyCode) const
{
    return
        !m_keyStates[keyCode].m_isKeyDown &&
        m_keyStates[keyCode].m_wasKeyDownLastFrame;
}

//----------------------------------------------------------------------------------------------------
bool InputSystem::IsKeyDown(unsigned char const keyCode) const
{
    return m_keyStates[keyCode].m_isKeyDown;
}

//----------------------------------------------------------------------------------------------------
void InputSystem::HandleKeyPressed(unsigned char const keyCode)
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
void InputSystem::SetCursorMode(eCursorMode const mode)
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
    case eCursorMode::POINTER: return Vec2::ZERO;
    case eCursorMode::FPS: return static_cast<Vec2>(m_cursorState.m_cursorClientDelta);
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
    // if (g_theDevConsole == nullptr)
    // {
    //     ERROR_RECOVERABLE("g_theDevConsole is nullptr")
    // }

    if (g_input == nullptr)
    {
        return false;
    }

    int const           value   = args.GetValue("OnWindowKeyPressed", -1);
    unsigned char const keyCode = static_cast<unsigned char>(value);
    g_input->HandleKeyPressed(keyCode);

    return true;
}

//----------------------------------------------------------------------------------------------------
STATIC bool InputSystem::OnWindowKeyReleased(EventArgs& args)
{
    // if (g_theDevConsole == nullptr)
    // {
    //     return false;
    // }

    if (g_input == nullptr)
    {
        return false;
    }

    // if (g_theDevConsole->IsOpen())
    // {
    //     return false;
    // }

    int const           value   = args.GetValue("OnWindowKeyReleased", -1);
    unsigned char const keyCode = static_cast<unsigned char>(value);
    g_input->HandleKeyReleased(keyCode);

    return true;
}

//----------------------------------------------------------------------------------------------------
// Phase 6a: KADI Development Tools - Input Injection Implementation
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
void InputSystem::InjectKeyPress(unsigned char keyCode, int durationMs)
{
    DAEMON_LOG(LogScript, eLogVerbosity::Log,
        StringFormat("InputSystem: Injecting key press for keyCode={}, duration={}ms", keyCode, durationMs));

    // Convert virtual key code to scan code
    UINT scanCode = MapVirtualKeyA(keyCode, MAPVK_VK_TO_VSC);

    // Prepare key down event
    INPUT inputDown = {};
    inputDown.type = INPUT_KEYBOARD;
    inputDown.ki.wVk = keyCode;
    inputDown.ki.wScan = static_cast<WORD>(scanCode);
    inputDown.ki.dwFlags = 0;  // Key down
    inputDown.ki.time = 0;
    inputDown.ki.dwExtraInfo = 0;

    // Prepare key up event
    INPUT inputUp = {};
    inputUp.type = INPUT_KEYBOARD;
    inputUp.ki.wVk = keyCode;
    inputUp.ki.wScan = static_cast<WORD>(scanCode);
    inputUp.ki.dwFlags = KEYEVENTF_KEYUP;
    inputUp.ki.time = 0;
    inputUp.ki.dwExtraInfo = 0;

    // Send key down
    UINT result = SendInput(1, &inputDown, sizeof(INPUT));
    if (result != 1)
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Error,
            StringFormat("InputSystem: SendInput failed for key down (keyCode={})", keyCode));
        return;
    }

    // Wait for duration
    if (durationMs > 0)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(durationMs));
    }

    // Send key up
    result = SendInput(1, &inputUp, sizeof(INPUT));
    if (result != 1)
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Error,
            StringFormat("InputSystem: SendInput failed for key up (keyCode={})", keyCode));
        return;
    }

    DAEMON_LOG(LogScript, eLogVerbosity::Log,
        StringFormat("InputSystem: Key press injection completed for keyCode={}", keyCode));
}

//----------------------------------------------------------------------------------------------------
void InputSystem::InjectKeyHold(unsigned char keyCode, int durationMs, bool repeat)
{
    // Convert boolean to string for logging (avoid StringFormat assertion with bool)
    char const* repeatStr = repeat ? "true" : "false";

    DAEMON_LOG(LogScript, eLogVerbosity::Log,
        StringFormat("InputSystem: Injecting key hold for keyCode={}, duration={}ms, repeat={}",
                     keyCode, durationMs, repeatStr));

    // Convert virtual key code to scan code
    UINT scanCode = MapVirtualKeyA(keyCode, MAPVK_VK_TO_VSC);

    // Prepare key down event
    INPUT inputDown = {};
    inputDown.type = INPUT_KEYBOARD;
    inputDown.ki.wVk = keyCode;
    inputDown.ki.wScan = static_cast<WORD>(scanCode);
    inputDown.ki.dwFlags = 0;  // Key down
    inputDown.ki.time = 0;
    inputDown.ki.dwExtraInfo = 0;

    // Send initial key down
    UINT result = SendInput(1, &inputDown, sizeof(INPUT));
    if (result != 1)
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Error,
            StringFormat("InputSystem: SendInput failed for key down (keyCode={})", keyCode));
        return;
    }

    // If repeat is enabled, send multiple key down events during hold duration
    if (repeat && durationMs > 0)
    {
        int const repeatInterval = 50;  // 50ms between repeats (typical keyboard repeat rate)
        int remainingTime = durationMs;

        while (remainingTime > 0)
        {
            int waitTime = (remainingTime < repeatInterval) ? remainingTime : repeatInterval;
            std::this_thread::sleep_for(std::chrono::milliseconds(waitTime));
            remainingTime -= waitTime;

            // Send another key down event to simulate repeat
            if (remainingTime > 0)
            {
                SendInput(1, &inputDown, sizeof(INPUT));
            }
        }
    }
    else if (durationMs > 0)
    {
        // Simple hold without repeat
        std::this_thread::sleep_for(std::chrono::milliseconds(durationMs));
    }

    // Prepare key up event
    INPUT inputUp = {};
    inputUp.type = INPUT_KEYBOARD;
    inputUp.ki.wVk = keyCode;
    inputUp.ki.wScan = static_cast<WORD>(scanCode);
    inputUp.ki.dwFlags = KEYEVENTF_KEYUP;
    inputUp.ki.time = 0;
    inputUp.ki.dwExtraInfo = 0;

    // Send key up
    result = SendInput(1, &inputUp, sizeof(INPUT));
    if (result != 1)
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Error,
            StringFormat("InputSystem: SendInput failed for key up (keyCode={})", keyCode));
        return;
    }

    DAEMON_LOG(LogScript, eLogVerbosity::Log,
        StringFormat("InputSystem: Key hold injection completed for keyCode={}", keyCode));
}
