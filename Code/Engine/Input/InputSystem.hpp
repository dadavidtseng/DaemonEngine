//----------------------------------------------------------------------------------------------------
// InputSystem.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Input/InputCommon.hpp"
#include "Engine/Input/XboxController.hpp"

//----------------------------------------------------------------------------------------------------
struct sInputSystemConfig
{
};

//----------------------------------------------------------------------------------------------------
class InputSystem
{
public:
    explicit InputSystem(sInputSystemConfig const& config);
    ~InputSystem() = default;

    void Startup();
    void Shutdown();
    void BeginFrame();
    void EndFrame();

    bool WasKeyJustPressed(unsigned char keyCode) const;
    bool WasKeyJustReleased(unsigned char keyCode) const;
    bool IsKeyDown(unsigned char keyCode) const;

    void HandleKeyPressed(unsigned char keyCode);
    void HandleKeyReleased(unsigned char keyCode);

    XboxController const& GetController(int controllerID);
    Vec2                  GetCursorClientDelta() const;
    Vec2                  GetCursorClientPosition() const;
    Vec2                  GetCursorNormalizedPosition() const;

    void SetCursorMode(eCursorMode mode);

    static bool OnWindowKeyPressed(EventArgs& args);
    static bool OnWindowKeyReleased(EventArgs& args);

protected:
    sKeyButtonState m_keyStates[NUM_KEYCODES];
    XboxController  m_controllers[NUM_XBOX_CONTROLLERS];
    sCursorState    m_cursorState;

private:
    sInputSystemConfig m_config;
};
