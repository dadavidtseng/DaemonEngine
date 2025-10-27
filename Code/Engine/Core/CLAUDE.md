[Root Directory](../../../CLAUDE.md) > [Code](../../) > [Engine](../) > **Core**

# Core Module Documentation

## Module Responsibilities

The Core module provides fundamental engine systems and utilities that serve as the foundation for all other engine modules. It handles essential services like event management, time systems, modular logging with output devices, developer tools, centralized engine singleton (GEngine), and basic data structures.

## Entry and Startup

### Primary Headers
- `Engine.hpp` - Global engine singleton providing centralized access to subsystems
- `EngineCommon.hpp` - Global engine declarations and utilities
- `EventSystem.hpp` - Event-driven communication system
- `LogSubsystem.hpp` - Modular logging system with pluggable output devices
- `ILogOutputDevice.hpp` - Interface for custom log output destinations
- `Clock.hpp` - Time management and timing utilities
- `JobSystem.hpp` - Multi-threaded job system with specialized worker threads
- `StateBuffer.hpp` - Generic double-buffering template for async state synchronization
- `DevConsole.hpp` - Developer console for debugging and command execution

### Initialization Pattern
```cpp
// Modern GEngine centralized initialization (recommended)
GEngine& engine = GEngine::Get();
engine.Construct(); // JSON-based subsystem initialization

// Access subsystems via global pointers
g_logSubsystem->LogInfo("Engine started");
g_eventSystem->FireEvent("EngineStarted", args);
g_jobSystem->QueueJob(job);

// Legacy manual initialization (deprecated)
g_eventSystem = new EventSystem(eventConfig);
g_devConsole = new DevConsole(consoleConfig);
g_jobSystem = new JobSystem();
g_jobSystem->StartUp(4, 1); // 4 generic threads, 1 I/O thread
```

### JSON Configuration Files
The engine supports data-driven configuration through JSON files:

**LogConfig.json** (`Data/Config/LogConfig.json`):
```json
{
  "logFilePath": "Logs/latest.log",
  "enableConsole": true,
  "enableFile": true,
  "enableDebugOut": true,
  "enableOnScreen": true,
  "enableDevConsole": true,
  "asyncLogging": true,
  "maxLogEntries": 50000,
  "timestampEnabled": true,
  "threadIdEnabled": true,
  "autoFlush": false,
  "enableSmartRotation": true,
  "rotationConfigPath": "Data/Config/LogRotation.json",
  "smartRotationConfig": {
    "maxFileSizeBytes": 104857600,
    "maxTimeInterval": 7200,
    "logDirectory": "Logs",
    "currentLogName": "latest.log",
    "sessionPrefix": "session"
  }
}
```

**EngineSubsystems.json** (`Data/Config/EngineSubsystems.json`):
```json
{
  "subsystems": {
    "log": true,
    "events": true,
    "jobs": true,
    "audio": true,
    "renderer": true,
    "input": true,
    "resource": true,
    "script": true
  }
}
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
void ExecuteCommand(String const& command);
```

### Logging System API
```cpp
// Modular logging with pluggable output devices
class LogSubsystem {
    // Add custom output devices
    void AddOutputDevice(ILogOutputDevice* device);
    void RemoveOutputDevice(ILogOutputDevice* device);

    // Logging operations
    void LogMessage(eLogLevel level, String const& message, String const& category = "");
    void LogError(String const& message, String const& category = "");
    void LogWarning(String const& message, String const& category = "");
    void LogInfo(String const& message, String const& category = "");

    // Device management
    void FlushAllDevices();
};

// Output device implementations
class FileOutputDevice : public ILogOutputDevice {
    // Writes logs to file
};

class ConsoleOutputDevice : public ILogOutputDevice {
    // Writes to standard console
};

class DevConsoleOutputDevice : public ILogOutputDevice {
    // Integrates with in-game developer console
};

class DebugOutputDevice : public ILogOutputDevice {
    // Writes to Visual Studio debug output
};

class OnScreenOutputDevice : public ILogOutputDevice {
    // Displays logs on-screen during gameplay
};

class SmartFileOutputDevice : public ILogOutputDevice {
    // Advanced file logging with rotation and management
};
```

### JobSystem API
```cpp
// Multi-threaded job system with specialized workers
void StartUp(int numGenericThreads, int numIOThreads = 1);
void QueueJob(Job* job);
void QueueGenericJob(JobCallback callback, void* userData = nullptr);
void QueueIOJob(JobCallback callback, void* userData = nullptr);
bool AreAllJobsCompleted() const;
void ShutDown();
```


### StateBuffer Template API
```cpp
// Generic double-buffered container for thread-safe state synchronization
template <typename TStateContainer>
class StateBuffer {
    // Lock-free buffer access
    TStateContainer const* GetFrontBuffer() const;  // Main thread reads (rendering)
    TStateContainer* GetBackBuffer();                // Worker thread writes (JavaScript)
    
    // Frame boundary synchronization
    void SwapBuffers();  // Brief locked operation (main thread only)
    
    // Monitoring
    size_t GetElementCount() const;
    uint64_t GetTotalSwaps() const;
};

// Usage example: Entity state synchronization
using EntityStateMap = std::unordered_map<EntityID, EntityState>;
using EntityStateBuffer = StateBuffer<EntityStateMap>;

EntityStateBuffer* buffer = new EntityStateBuffer();

// Worker thread (JavaScript logic)
EntityStateMap* backBuffer = buffer->GetBackBuffer();
(*backBuffer)[entityId] = updatedState;  // Lock-free write

// Main thread (Rendering)
EntityStateMap const* frontBuffer = buffer->GetFrontBuffer();
for (auto const& [id, state] : *frontBuffer) {
    RenderEntity(state);  // Lock-free read
}

// Frame boundary (Main thread)
buffer->SwapBuffers();  // Brief locked copy and swap
```


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
A: Yes, register new event names


### Q: How do I use StateBuffer for async state synchronization?
A: Create a StateBuffer with your container type (e.g., StateBuffer<std::unordered_map<ID, State>>). Worker thread writes to back buffer, main thread reads from front buffer, and call SwapBuffers() at frame boundaries.

### Q: Is StateBuffer thread-safe?
A: Yes. Worker thread writes to back buffer (lock-free), main thread reads from front buffer (lock-free), and SwapBuffers() uses a brief lock to synchronize. Assumes single writer and single reader.
 by calling `SubscribeEventCallbackFunction()` with your custom event names and callbacks.

## Related Files

### Core Implementation Files
- `Engine.cpp` - Global engine singleton implementation
- `EngineCommon.cpp` - Global variable definitions
- `EventSystem.cpp` - Event management implementation
- `LogSubsystem.cpp` - Core logging system implementation

### Logging Output Devices
- `ILogOutputDevice.hpp` - Abstract interface for log output devices
- `FileOutputDevice.cpp` - File-based logging implementation
- `SmartFileOutputDevice.cpp` - Advanced file logging with rotation
- `ConsoleOutputDevice.cpp` - Console output implementation
- `DebugOutputDevice.cpp` - Visual Studio debug output
- `DevConsoleOutputDevice.cpp` - Developer console integration
- `OnScreenOutputDevice.cpp` - On-screen log display

### Core Systems
- `Clock.cpp` - Time management implementation
- `Timer.cpp` - Timer utilities
- `DevConsole.cpp` - Developer console system
- `JobSystem.cpp` - Multi-threaded job system implementation
- `JobWorkerThread.cpp` - Worker thread management
- `Job.cpp` - Job definition and execution
- `StringUtils.cpp` - String manipulation utilities
- `FileUtils.cpp` - File I/O operations
- `XmlUtils.cpp` - XML parsing utilities
- `HeatMaps.cpp` - Performance visualization
- `NamedStrings.cpp` - Configuration management
- `StateBuffer.hpp` - Generic double-buffering template (header-only)
- `Rgba8.cpp` - Color utilities
- `SimpleTriangleFont.cpp` - Basic font rendering

### Header Files
- `Time.hpp` - Time constants and utilities
- `FileUtils.hpp` - File system operations
- `StringUtils.hpp` - String processing functions

## Changelog

- 2025-10-27: **M4-T8 Async Architecture Refactoring**
  - Added StateBuffer.hpp template for generic double-buffered state containers
  - Provides lock-free reads/writes with brief locked swap operations
  - Supports async architecture pattern with worker thread state synchronization
  - Used by Entity and Camera systems for thread-safe state management
  - Template instantiations: StateBuffer<EntityStateMap>, StateBuffer<CameraStateMap>

- 2025-10-07: **Engine Subsystem Initialization Modernization**
  - Implemented `GEngine::Construct()` for centralized, JSON-configured subsystem initialization
  - Added data-driven configuration support through `LogConfig.json` and `EngineSubsystems.json`
  - Automatic LogSubsystem configuration with fallback to hardcoded defaults
  - Hardware-aware JobSystem thread allocation based on CPU core count
  - Comprehensive initialization logging and error handling for missing/malformed config files
  - Support for runtime subsystem enable/disable through JSON configuration
- 2025-10-01: **Major LogSubsystem Refactoring** (SOLID Principles Applied)
  - Extracted 7 output devices from monolithic 2,033-line implementation into separate modular files
  - Created `ILogOutputDevice.hpp` base interface following Interface Segregation Principle
  - Split implementations: `ConsoleOutputDevice`, `FileOutputDevice`, `SmartFileOutputDevice`, `DebugOutputDevice`, `OnScreenOutputDevice`, `DevConsoleOutputDevice`
  - Reduced `LogSubsystem.hpp` from 567 lines to ~120 lines (79% reduction)
  - Reduced `LogSubsystem.cpp` from 1,466 lines to 616 lines (58% reduction)
  - Fixed race condition in `SmartFileOutputDevice` shutdown sequence (Minecraft-style latest.log persistence)
  - Removed duplicate symbol definitions between LogSubsystem.cpp and SmartFileOutputDevice.cpp
  - Updated Visual Studio project files (Engine.vcxproj, Engine.vcxproj.filters) with new modular structure
- 2025-10-01: Introduced GEngine singleton for centralized subsystem access and reduced global namespace pollution
- 2025-10-01: Refactored LogSubsystem to modular output device architecture with ILogOutputDevice interface
- 2025-10-01: Added multiple log output device implementations (File, Console, Debug, DevConsole, OnScreen, SmartFile)
- 2025-09-30: Enhanced JobSystem with thread type specialization and I/O threading support
- 2025-09-30: Improved DevConsole with enhanced functionality and comprehensive documentation
- 2025-09-06 19:54:50: Initial module documentation created
- Recent developments: JSON-based engine configuration, modular subsystem initialization, comprehensive logging architecture