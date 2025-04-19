//----------------------------------------------------------------------------------------------------
// InputSystem.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Input/XboxController.hpp"
#include "Engine/Math/IntVec2.hpp"

//-----------------------------------------------------------------------------------------------
extern unsigned char const NUMCODE_0;
extern unsigned char const NUMCODE_1;
extern unsigned char const NUMCODE_2;
extern unsigned char const NUMCODE_3;
extern unsigned char const NUMCODE_4;
extern unsigned char const NUMCODE_5;
extern unsigned char const NUMCODE_6;
extern unsigned char const NUMCODE_7;
extern unsigned char const NUMCODE_8;
extern unsigned char const NUMCODE_9;
extern unsigned char const KEYCODE_A;
extern unsigned char const KEYCODE_B;
extern unsigned char const KEYCODE_C;
extern unsigned char const KEYCODE_D;
extern unsigned char const KEYCODE_E;
extern unsigned char const KEYCODE_F;
extern unsigned char const KEYCODE_G;
extern unsigned char const KEYCODE_H;
extern unsigned char const KEYCODE_I;
extern unsigned char const KEYCODE_J;
extern unsigned char const KEYCODE_K;
extern unsigned char const KEYCODE_L;
extern unsigned char const KEYCODE_M;
extern unsigned char const KEYCODE_N;
extern unsigned char const KEYCODE_O;
extern unsigned char const KEYCODE_P;
extern unsigned char const KEYCODE_Q;
extern unsigned char const KEYCODE_R;
extern unsigned char const KEYCODE_S;
extern unsigned char const KEYCODE_T;
extern unsigned char const KEYCODE_U;
extern unsigned char const KEYCODE_V;
extern unsigned char const KEYCODE_W;
extern unsigned char const KEYCODE_X;
extern unsigned char const KEYCODE_Y;
extern unsigned char const KEYCODE_Z;
extern unsigned char const KEYCODE_F1;
extern unsigned char const KEYCODE_F2;
extern unsigned char const KEYCODE_F3;
extern unsigned char const KEYCODE_F4;
extern unsigned char const KEYCODE_F5;
extern unsigned char const KEYCODE_F6;
extern unsigned char const KEYCODE_F7;
extern unsigned char const KEYCODE_F8;
extern unsigned char const KEYCODE_F9;
extern unsigned char const KEYCODE_F10;
extern unsigned char const KEYCODE_F11;
extern unsigned char const KEYCODE_F12;
extern unsigned char const KEYCODE_F13;
extern unsigned char const KEYCODE_F14;
extern unsigned char const KEYCODE_F15;
extern unsigned char const KEYCODE_ESC;
extern unsigned char const KEYCODE_UPARROW;
extern unsigned char const KEYCODE_DOWNARROW;
extern unsigned char const KEYCODE_LEFTARROW;
extern unsigned char const KEYCODE_RIGHTARROW;
extern unsigned char const KEYCODE_ENTER;
extern unsigned char const KEYCODE_SPACE;
extern unsigned char const KEYCODE_BACKSPACE;
extern unsigned char const KEYCODE_LEFT_MOUSE;
extern unsigned char const KEYCODE_RIGHT_MOUSE;
extern unsigned char const KEYCODE_INSERT;
extern unsigned char const KEYCODE_DELETE;
extern unsigned char const KEYCODE_HOME;
extern unsigned char const KEYCODE_END;
extern unsigned char const KEYCODE_TILDE;
extern unsigned char const KEYCODE_LEFT_BRACKET;
extern unsigned char const KEYCODE_RIGHT_BRACKET;
extern unsigned char const KEYCODE_SHIFT;
extern unsigned char const KEYCODE_CONTROL;

//----------------------------------------------------------------------------------------------------
int constexpr NUM_KEYCODES         = 256;
int constexpr NUM_XBOX_CONTROLLERS = 4;

//----------------------------------------------------------------------------------------------------
enum class CursorMode
{
    POINTER,
    FPS
};

//----------------------------------------------------------------------------------------------------
struct CursorState
{
    IntVec2 m_cursorClientDelta;
    IntVec2 m_cursorClientPosition;

    CursorMode m_cursorMode = CursorMode::POINTER;
};

struct InputSystemConfig
{
};

//----------------------------------------------------------------------------------------------------
class InputSystem
{
public:
    explicit InputSystem(InputSystemConfig const& config);
    ~InputSystem() = default;
    void                  Startup();
    void                  Shutdown();
    void                  BeginFrame();
    void                  EndFrame();
    bool                  WasKeyJustPressed(unsigned char keyCode) const;
    bool                  WasKeyJustReleased(unsigned char keyCode) const;
    bool                  IsKeyDown(unsigned char keyCode) const;
    void                  HandleKeyPressed(unsigned char keyCode);
    void                  HandleKeyReleased(unsigned char keyCode);
    XboxController const& GetController(int controllerID);

    void SetCursorMode(CursorMode mode);
    Vec2 GetCursorClientDelta() const;
    Vec2 GetCursorClientPosition() const;
    Vec2 GetCursorNormalizedPosition() const;

    static bool OnWindowKeyPressed(EventArgs& args);
    static bool OnWindowKeyReleased(EventArgs& args);

protected:
    KeyButtonState m_keyStates[NUM_KEYCODES];
    XboxController m_controllers[NUM_XBOX_CONTROLLERS];
    CursorState    m_cursorState;

private:
    InputSystemConfig m_inputConfig;
};
