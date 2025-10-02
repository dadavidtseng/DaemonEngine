//----------------------------------------------------------------------------------------------------
// InputSystem.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Input/InputSystem.hpp"
//----------------------------------------------------------------------------------------------------
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Platform/Window.hpp"

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
