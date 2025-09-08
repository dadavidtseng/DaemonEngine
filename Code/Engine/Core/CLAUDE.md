[Root Directory](../../../CLAUDE.md) > [Code](../../) > [Engine](../) > **Core**

# Core Module Documentation

## Module Responsibilities

The Core module provides fundamental engine systems and utilities that serve as the foundation for all other engine modules. It handles essential services like event management, time systems, logging, developer tools, and basic data structures.

## Entry and Startup

### Primary Headers
- `EngineCommon.hpp` - Global engine declarations and utilities
- `EventSystem.hpp` - Event-driven communication system
- `LogSubsystem.hpp` - Centralized logging system
- `Clock.hpp` - Time management and timing utilities

### Initialization Pattern
```cpp
// Global system instances declared in EngineCommon.hpp
extern EventSystem* g_eventSystem;
extern DevConsole*  g_devConsole;

// Typical startup in application
g_eventSystem = new EventSystem(eventConfig);
g_devConsole = new DevConsole(consoleConfig);
```

## External Interfaces

### Event System API
```cpp
// Event subscription and firing
void SubscribeEventCallbackFunction(String const& eventName, EventCallbackFunction functionPtr);
void FireEvent(String const& eventName, EventArgs& args);

// Usage example
SubscribeEventCallbackFunction("KeyPressed", &InputSystem::OnWindowKeyPressed);
EventArgs args;
args.SetValue("keyCode", keyCode);
FireEvent("KeyPressed", args);
```

### Developer Console
```cpp
// Console command registration and execution
void AddLine(Rgba8 const& color, String const& text);
void ToggleDisplay();
bool IsOpen() const;
```

### Timing and Clock System
```cpp
// High-precision timing
class Clock {
    double GetTotalSeconds() const;
    double GetDeltaSeconds() const;
    void Update(double deltaTimeSeconds);
};

class Timer {
    double GetElapsedSeconds() const;
    void Start();
    void Stop();
};
```

## Key Dependencies and Configuration

### Internal Dependencies
- Math module for basic mathematical operations
- Platform module for OS-specific functionality
- String utilities for text processing

### External Dependencies
- Standard C++ libraries (`<string>`, `<vector>`, `<map>`)
- Windows API for platform-specific operations
- TinyXML2 for XML parsing capabilities

### Configuration Structures
```cpp
struct sEventSystemConfig {
    // Currently minimal configuration
};

// Global configuration blackboard
extern NamedStrings g_gameConfigBlackboard;
```

## Data Models

### Core Data Structures
- **NamedStrings**: Key-value string storage for configuration and event arguments
- **EventArgs**: Type alias for NamedStrings used in event system
- **EventSubscription**: Callback function wrapper for event handling
- **Rgba8**: 8-bit RGBA color representation
- **StringList**: Vector of strings with utility functions

### Memory Management Utilities
```cpp
template <typename T>
void ENGINE_SAFE_RELEASE(T*& pointer) {
    if (pointer != nullptr) {
        delete pointer;
        pointer = nullptr;
    }
}
```

## Testing and Quality

### Current Testing Approach
- Manual testing through developer console
- Event system tested via inter-system communication
- Debug output and error reporting through console
- Time system validation through frame rate monitoring

### Quality Assurance Features
- Error and warning assertion system (`ErrorWarningAssert.hpp`)
- Debug logging with multiple severity levels
- Memory leak detection utilities
- Consistent error reporting patterns

### Recommended Testing Additions
- Unit tests for string utilities and data structures
- Event system regression tests
- Performance benchmarks for core utilities
- Mock objects for testing event-driven systems

## FAQ

### Q: How do I add a new global system?
A: 1) Declare extern pointer in `EngineCommon.hpp`, 2) Define in `EngineCommon.cpp`, 3) Initialize in your application startup code.

### Q: What's the difference between Clock and Timer?
A: Clock tracks global engine time and frame delta, Timer measures specific durations for gameplay or profiling.

### Q: How do I handle errors consistently?
A: Use the assertion macros in `ErrorWarningAssert.hpp` for debug-time checks, and logging system for runtime error reporting.

### Q: Can I extend the event system?
A: Yes, register new event names by calling `SubscribeEventCallbackFunction()` with your custom event names and callbacks.

## Related Files

### Core Implementation Files
- `EngineCommon.cpp` - Global variable definitions
- `EventSystem.cpp` - Event management implementation
- `LogSubsystem.cpp` - Logging system implementation
- `Clock.cpp` - Time management implementation
- `Timer.cpp` - Timer utilities
- `DevConsole.cpp` - Developer console system
- `StringUtils.cpp` - String manipulation utilities
- `FileUtils.cpp` - File I/O operations
- `XmlUtils.cpp` - XML parsing utilities
- `HeatMaps.cpp` - Performance visualization
- `NamedStrings.cpp` - Configuration management
- `Rgba8.cpp` - Color utilities
- `SimpleTriangleFont.cpp` - Basic font rendering

### Header Files
- `Time.hpp` - Time constants and utilities
- `FileUtils.hpp` - File system operations
- `StringUtils.hpp` - String processing functions

## Changelog

- 2025-09-06 19:54:50: Initial module documentation created
- Recent developments: LogSubsystem integration with DevConsole, StringUtils enhancements for naming conventions