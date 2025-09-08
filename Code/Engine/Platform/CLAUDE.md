[Root Directory](../../../CLAUDE.md) > [Code](../../) > [Engine](../) > **Platform**

# Platform Module Documentation

## Module Responsibilities

The Platform module provides OS abstraction layer functionality with comprehensive window management, display context handling, input capture, viewport management, and DirectX integration for cross-platform compatibility and advanced windowing features.

## Entry and Startup

### Primary Entry Point
- `Window.hpp` - Main window management and OS abstraction
- `WindowCommon.hpp` - Shared window constants and enumerations

### Initialization Pattern
```cpp
sWindowConfig config;
config.m_inputSystem = inputSystem;
config.m_windowType = eWindowType::WINDOWED;
config.m_aspectRatio = 16.0f / 9.0f;
config.m_windowTitle = "Daemon Engine Application";
config.m_supportMultipleWindows = false;

Window* mainWindow = new Window(config);
mainWindow->Startup();

// Set as global main window
Window::s_mainWindow = mainWindow;

// Basic window loop
while (isRunning) {
    mainWindow->BeginFrame();
    // Application logic here
    mainWindow->EndFrame();
}
```

## External Interfaces

### Core Window API
```cpp
class Window {
    // System lifecycle
    void Startup();
    void Shutdown();
    void BeginFrame();
    void EndFrame();
    
    // Configuration access
    sWindowConfig const& GetConfig() const;
    void* GetDisplayContext() const;
    void* GetWindowHandle() const;
    
    // Window management
    void SetWindowType(eWindowType newType);
    void ReconfigureWindow();
    
    // Dimension and position control
    Vec2 GetClientDimensions() const;
    Vec2 GetClientPosition() const;
    void SetClientDimensions(Vec2 const& newDimensions);
    void SetClientPosition(Vec2 const& newPosition);
    Vec2 GetWindowPosition() const;
    Vec2 GetWindowDimensions() const;
    void SetWindowDimensions(Vec2 const& newDimensions);
    void SetWindowPosition(Vec2 const& newPosition);
    
    // Viewport management
    Vec2 GetViewportDimensions() const;
    Vec2 GetViewportOffset() const;
    float GetViewportAspectRatio() const;
};
```

### Mouse and Cursor Integration
```cpp
// Coordinate system conversions
Vec2 EngineToWindowsCoords(Vec2 const& engineCoords) const;
Vec2 WindowsToEngineCoords(Vec2 const& windowsCoords) const;
Vec2 GetNormalizedMouseUV() const;
Vec2 GetCursorPositionOnScreen() const;

// Global input capture for FPS-style camera control
void EnableGlobalInputCapture();
void DisableGlobalInputCapture();
```

### Screen and Display Information
```cpp
Vec2 GetScreenDimensions() const;
Vec2 GetBorderOffset();
```

## Key Dependencies and Configuration

### External Dependencies
- **Windows API**: Core window management (`user32.lib`, `gdi32.lib`)
- **DirectX 11**: Display context integration (`d3d11.lib`, `dxgi.lib`)
- **System APIs**: Mouse hooks and global input capture

### Internal Dependencies
- Core module for string utilities and basic types
- Math module for Vec2 coordinate operations
- Renderer module for DirectX integration and swap chain management

### Configuration Structure
```cpp
struct sWindowConfig {
    InputSystem*   m_inputSystem;              // Input system integration
    eWindowType    m_windowType;               // Windowed, fullscreen, borderless
    float          m_aspectRatio;              // Target aspect ratio
    String         m_consoleTitle[11];         // Console window titles
    String         m_windowTitle;              // Main window title
    wchar_t const* m_iconFilePath;            // Window icon resource
    bool           m_supportMultipleWindows;   // Multi-window support flag
};
```

### Window Type Management
```cpp
enum class eWindowType : int8_t {
    INVALID,
    WINDOWED,        // Standard windowed mode
    FULLSCREEN,      // Exclusive fullscreen
    BORDERLESS       // Borderless fullscreen window
};
```

## Data Models

### Window State Management
```cpp
struct WindowRect {
    long left, top, right, bottom;  // Window rectangle bounds
};

class Window {
private:
    void*                   m_windowHandle;      // Windows HWND
    void*                   m_displayContext;    // Windows HDC
    IDXGISwapChain1*        m_swapChain;        // DirectX swap chain
    ID3D11RenderTargetView* m_renderTargetView; // DirectX render target
};
```

### Coordinate Systems and Viewport
```cpp
// Multiple coordinate system support
Vec2 m_screenDimensions;    // Full screen resolution
Vec2 m_windowDimensions;    // Window outer dimensions
Vec2 m_windowPosition;      // Window position on screen
Vec2 m_clientDimensions;    // Client area dimensions
Vec2 m_clientPosition;      // Client area position
Vec2 m_viewportDimensions;  // Rendering viewport size
Vec2 m_viewportPosition;    // Rendering viewport position
Vec2 m_viewportOffset;      // Letterbox/crop offset for aspect ratio
```

### Global Input Capture System
```cpp
// Windows hook system for global input
HHOOK m_globalMouseHook;
HHOOK m_globalKeyboardHook;
bool  m_useGlobalCapture;

// Hook procedures for system-wide input capture
static LRESULT GlobalMouseProc(int nCode, WPARAM wParam, LPARAM lParam);
static LRESULT GlobalKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
```

### Dynamic Window Updates
```cpp
WindowRect lastRect;
bool m_shouldUpdatePosition;
bool m_shouldUpdateDimension;

void UpdatePosition();
void UpdateDimension();
```

## Testing and Quality

### Built-in Quality Features
- **Automatic Window State Tracking**: Continuous monitoring of window changes
- **DirectX Integration Validation**: Swap chain and render target consistency
- **Multi-Monitor Support**: Proper handling across different display configurations
- **Aspect Ratio Preservation**: Automatic letterboxing and viewport adjustment

### Current Testing Approach
- Manual window resize and movement testing
- Multi-monitor configuration validation
- Fullscreen/windowed mode transition testing
- Input capture verification across different scenarios

### Quality Assurance Features
- Robust error handling for Windows API failures
- Automatic recovery from display context loss
- Memory management for DirectX resources
- Safe handling of window destruction and recreation

### Recommended Testing Additions
- Automated window state transition testing
- Multi-monitor edge case validation
- Performance testing for window operations
- Cross-platform abstraction layer testing

## FAQ

### Q: How do I switch between windowed and fullscreen modes?
A: Use `SetWindowType(eWindowType::FULLSCREEN)` followed by `ReconfigureWindow()` for dynamic mode switching.

### Q: What's the difference between fullscreen and borderless modes?
A: Fullscreen uses exclusive display mode for best performance, while borderless creates a maximized window without borders.

### Q: How does the viewport system handle different aspect ratios?
A: The system automatically calculates letterboxing or cropping based on the target aspect ratio, maintaining visual consistency.

### Q: Can I capture input when the window doesn't have focus?
A: Yes, use `EnableGlobalInputCapture()` to register system-wide hooks for scenarios like FPS camera control.

### Q: How do coordinate conversions work between engine and Windows?
A: Use `EngineToWindowsCoords()` and `WindowsToEngineCoords()` for seamless conversion between coordinate systems.

### Q: Is the platform module thread-safe?
A: Window operations should be performed on the main thread. The module provides thread-safe access to window state queries.

## Related Files

### Core Implementation
- `Window.cpp` - Main window management and DirectX integration
- `WindowCommon.cpp` - Shared window utilities and helper functions

### OS Integration
The Platform module provides OS abstraction for:
- **Window Management**: Creation, sizing, positioning, and mode switching
- **Display Context**: DirectX integration and rendering surface management
- **Input Capture**: System-wide input hooks and message processing
- **Multi-Monitor**: Display enumeration and cross-monitor window handling

### Integration Points
- **Renderer Module**: Provides display context and swap chain for DirectX operations
- **Input System**: Integrates with global input capture and message processing
- **Event System**: Window events for resize, focus, and state changes

### Platform-Specific Extensions
- Windows-specific implementation with Win32 API integration
- DirectX 11 swap chain management
- Global input hook system for advanced input scenarios
- Console window management for debugging

### Planned Extensions
- Cross-platform abstraction for Linux and macOS support
- Vulkan and OpenGL display context support
- Advanced multi-monitor window spanning
- High-DPI scaling and display awareness
- Window composition effects and transparency

## Changelog

- 2025-09-06 21:17:11: Initial Platform module documentation created
- Recent developments: Enhanced window state tracking, global input capture system, comprehensive DirectX integration