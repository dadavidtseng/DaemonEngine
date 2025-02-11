//-----------------------------------------------------------------------------------------------
// InputSystem.hpp
//

//-----------------------------------------------------------------------------------------------
#pragma once
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Core/NamedStrings.hpp"
#include "Engine/Input/XboxController.hpp"

//-----------------------------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------------------------
int constexpr NUM_KEYCODES         = 256;
int constexpr NUM_XBOX_CONTROLLERS = 4;

struct InputSystemConfig
{
};

//-----------------------------------------------------------------------------------------------
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

    static bool Event_KeyPressed(EventArgs& args);
    static bool Event_KeyReleased(EventArgs& args);

protected:
    KeyButtonState m_keyStates[NUM_KEYCODES];
    XboxController m_controllers[NUM_XBOX_CONTROLLERS];

private:
    InputSystemConfig m_inputConfig;
};
