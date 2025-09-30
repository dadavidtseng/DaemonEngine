[Root Directory](../../../CLAUDE.md) > [Code](../../) > [Engine](../) > **Input**

# Input Module Documentation

## Module Responsibilities

The Input module provides comprehensive input handling for keyboard, mouse, and Xbox controllers with event-driven architecture, cursor management, and seamless integration with the engine's event system for responsive user interactions.

## Entry and Startup

### Primary Entry Point
- `InputSystem.hpp` - Main input system and device management
- `InputCommon.hpp` - Shared input constants and utilities
- `XboxController.hpp` - Xbox controller abstraction
- `KeyButtonState.hpp` - Key/button state tracking
- `AnalogJoystick.hpp` - Analog joystick with dead zone correction

### Initialization Pattern
```cpp
sInputSystemConfig config;
InputSystem* inputSystem = new InputSystem(config);
inputSystem->Startup();

// Register for window events
SubscribeEventCallbackFunction("WindowKeyPressed", &InputSystem::OnWindowKeyPressed);
SubscribeEventCallbackFunction("WindowKeyReleased", &InputSystem::OnWindowKeyReleased);

// Basic input queries
if (inputSystem->IsKeyDown(KEYCODE_W)) {
    // Handle forward movement
}

if (inputSystem->WasKeyJustPressed(KEYCODE_SPACE)) {
    // Handle jump action
}
```

## External Interfaces

### Core Input API
```cpp
class InputSystem {
    // System lifecycle
    void Startup();
    void Shutdown();
    void BeginFrame();
    void EndFrame();
    
    // Keyboard input queries
    bool WasKeyJustPressed(unsigned char keyCode) const;
    bool WasKeyJustReleased(unsigned char keyCode) const;
    bool IsKeyDown(unsigned char keyCode) const;
    
    // Key event handling
    void HandleKeyPressed(unsigned char keyCode);
    void HandleKeyReleased(unsigned char keyCode);
    
    // Controller access
    XboxController const& GetController(int controllerID);
    
    // Cursor management
    void SetCursorMode(eCursorMode mode);
    Vec2 GetCursorClientDelta() const;
    Vec2 GetCursorClientPosition() const;
    Vec2 GetCursorNormalizedPosition() const;
};
```

### Xbox Controller Interface
```cpp
class XboxController {
    // Connection status
    bool IsConnected() const;
    
    // Button states
    bool WasButtonJustPressed(eXboxButton button) const;
    bool WasButtonJustReleased(eXboxButton button) const;
    bool IsButtonDown(eXboxButton button) const;
    
    // Analog stick input
    Vec2 GetLeftStick() const;
    Vec2 GetRightStick() const;
    
    // Trigger input
    float GetLeftTrigger() const;
    float GetRightTrigger() const;
    
    // Vibration control
    void SetVibration(float leftMotor, float rightMotor);
};
```

### Key Code Constants
```cpp
// Comprehensive key code definitions
extern unsigned char const KEYCODE_A;
extern unsigned char const KEYCODE_W;
extern unsigned char const KEYCODE_S;
extern unsigned char const KEYCODE_D;
extern unsigned char const KEYCODE_SPACE;
extern unsigned char const KEYCODE_ESC;
extern unsigned char const KEYCODE_ENTER;
extern unsigned char const KEYCODE_F1; // through F15
extern unsigned char const KEYCODE_SHIFT;
extern unsigned char const KEYCODE_CONTROL;
extern unsigned char const KEYCODE_LEFT_MOUSE;
extern unsigned char const KEYCODE_RIGHT_MOUSE;
extern unsigned char const KEYCODE_UPARROW;
extern unsigned char const KEYCODE_DOWNARROW;
extern unsigned char const KEYCODE_LEFTARROW;
extern unsigned char const KEYCODE_RIGHTARROW;

// Numeric keypad support
extern unsigned char const NUMCODE_0; // through NUMCODE_9
```

## Key Dependencies and Configuration

### Internal Dependencies
- Core module for event system integration and basic types
- Math module for Vec2 operations and coordinate transformations
- Platform module for window and OS integration

### Configuration Structure
```cpp
struct sInputSystemConfig {
    // Currently minimal configuration
    // Future expansion: key mapping, controller sensitivity settings
};
```

### System Constants
```cpp
int constexpr NUM_KEYCODES = 256;         // Total keyboard key codes
int constexpr NUM_XBOX_CONTROLLERS = 4;   // Maximum supported controllers
```

### Cursor Management
```cpp
enum class eCursorMode : int8_t {
    POINTER,    // Standard cursor with GUI interaction
    FPS         // First-person camera control with cursor lock
};

struct CursorState {
    IntVec2     m_cursorClientDelta;    // Frame-to-frame cursor movement
    IntVec2     m_cursorClientPosition; // Current cursor position
    eCursorMode m_cursorMode;           // Current cursor interaction mode
};
```

## Data Models

### Key Button State Tracking
```cpp
class KeyButtonState {
    bool m_wasDown;     // Previous frame state
    bool m_isDown;      // Current frame state
    
    // State queries
    bool WasJustPressed() const;
    bool WasJustReleased() const;
    bool IsDown() const;
};
```

### Analog Joystick Handling
```cpp
class AnalogJoystick {
    Vec2  m_rawPosition;        // Raw analog values
    Vec2  m_correctedPosition;  // Deadzone-corrected values
    float m_innerDeadzoneFraction;
    float m_outerDeadzoneFraction;
    
    Vec2 GetPosition() const;
    float GetMagnitude() const;
    float GetAngleDegrees() const;
};
```

### Controller State Management
```cpp
class XboxController {
protected:
    int            m_id;                    // Controller index (0-3)
    bool           m_isConnected;           // Connection status
    KeyButtonState m_buttons[14];           // All button states
    AnalogJoystick m_leftStick;            // Left analog stick
    AnalogJoystick m_rightStick;           // Right analog stick
    float          m_leftTrigger;           // Left trigger (0.0-1.0)
    float          m_rightTrigger;          // Right trigger (0.0-1.0)
};
```

## Testing and Quality

### Built-in Testing Features
- **Input State Debugging**: Visual display of all input states
- **Controller Connection Monitoring**: Automatic detection of controller changes
- **Key Mapping Validation**: Comprehensive key code coverage testing
- **Event System Integration**: Seamless event-driven input processing

### Current Testing Approach
- Manual input testing through game interactions
- Developer console for input state inspection
- Real-time input visualization during gameplay
- Controller calibration and deadzone testing

### Quality Assurance Features
- Automatic controller reconnection handling
- Robust key state management preventing missed inputs
- Frame-perfect input timing with proper state tracking
- Comprehensive error handling for input device failures

### Recommended Testing Additions
- Automated input simulation for regression testing
- Controller mapping verification across different devices
- Input latency measurement and optimization
- Multi-controller simultaneous input testing

## FAQ

### Q: How do I handle input for UI vs gameplay?
A: Use the event system to distinguish contexts. UI systems can subscribe to input events and consume them to prevent gameplay processing.

### Q: What's the difference between WasKeyJustPressed() and IsKeyDown()?
A: `WasKeyJustPressed()` returns true only on the first frame a key is pressed, while `IsKeyDown()` returns true every frame while held.

### Q: How do I implement custom key bindings?
A: Create a key mapping system that translates user-configured keys to game actions, using the raw key codes provided by the InputSystem.

### Q: Can I detect multiple controllers simultaneously?
A: Yes, the system supports up to 4 Xbox controllers with independent state tracking and vibration control.

### Q: How does cursor mode switching work?
A: `eCursorMode::POINTER` provides standard GUI interaction, while `eCursorMode::FPS` locks the cursor for camera control with relative movement.

### Q: Is input processed every frame?
A: Yes, `BeginFrame()` updates all input states, ensuring consistent input timing aligned with the game loop.

## Related Files

### Core Implementation
- `InputSystem.cpp` - Main input system implementation and event handling
- `InputCommon.cpp` - Shared input constants and utility functions
- `XboxController.cpp` - Xbox controller state management and XInput integration
- `KeyButtonState.cpp` - Button state tracking logic
- `AnalogJoystick.cpp` - Analog input processing with deadzone handling

### Integration Files
- `InputScriptInterface.cpp` - JavaScript binding for scripting system access
- `InputScriptInterface.hpp` - Script interface declarations for V8 integration

### Event Integration
The Input module integrates closely with:
- **Event System**: For broadcasting input events to other systems
- **Platform Module**: For OS-specific input device access
- **Scripting System**: Through InputScriptInterface for script-driven input handling

### Planned Extensions
- Support for additional controller types (PlayStation, generic gamepads)
- Advanced input mapping and configuration system
- Touch input support for future platform expansion
- Input recording and playback for testing and demos
- Gesture recognition system for complex input patterns

## Changelog

- 2025-09-30: Added InputCommon module for shared input constants and utilities
- 2025-09-30: Enhanced AnalogJoystick and KeyButtonState with comprehensive documentation
- 2025-09-06 21:17:11: Initial Input module documentation created
- Recent developments: InputScriptInterface for V8 integration, enhanced cursor management, comprehensive Xbox controller support