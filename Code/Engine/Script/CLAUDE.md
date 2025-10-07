[Root Directory](../../../CLAUDE.md) > [Code](../../) > [Engine](../) > **Scripting**

# Scripting Module Documentation

## Module Responsibilities

The Scripting module provides JavaScript integration through Google's V8 engine, enabling dynamic gameplay programming, configuration scripting, and runtime code execution. It offers a bridge between C++ engine systems and JavaScript game logic with comprehensive binding capabilities.

## Entry and Startup

### Primary Entry Point
- `V8Subsystem.hpp` - Main V8 integration subsystem
- `IScriptableObject.hpp` - Interface for C++ objects exposed to scripts

### Initialization Pattern
```cpp
sV8SubsystemConfig config;
config.enableDebugging = false;
config.heapSizeLimit = 256; // MB
config.enableScriptBindings = true;
config.scriptPath = "Data/Scripts/";
config.enableConsoleOutput = true;

V8Subsystem* v8System = new V8Subsystem(config);
v8System->Startup();

// Register C++ objects and functions
v8System->RegisterScriptableObject("input", inputInterface);
v8System->RegisterGlobalFunction("log", logFunction);
```

## External Interfaces

### Script Execution API
```cpp
class V8Subsystem {
    // Script execution
    bool ExecuteScript(std::string const& script);
    bool ExecuteScriptFile(std::string const& filename);
    std::any ExecuteScriptWithResult(std::string const& script);
    
    // Object and function binding
    void RegisterScriptableObject(String const& name, 
                                 std::shared_ptr<IScriptableObject> const& object);
    void RegisterGlobalFunction(String const& name, ScriptFunction const& function);
    
    // Error handling
    bool HasError() const;
    std::string GetLastError() const;
    void ClearError();
};
```

### Scriptable Object Interface
```cpp
class IScriptableObject {
    virtual ~IScriptableObject() = default;

    // Initialize method registry during construction (NEW)
    virtual void InitializeMethodRegistry() = 0;

    // Called by V8Subsystem when object methods are invoked from JavaScript
    virtual ScriptMethodResult CallMethod(String const& methodName,
                                         ScriptArgs const& args) = 0;

    // Return list of available methods for this object
    virtual std::vector<ScriptMethodInfo> GetAvailableMethods() const = 0;

    // Get object type name for debugging
    virtual String GetTypeName() const = 0;

protected:
    // Method registry for efficient dispatch (NEW)
    std::unordered_map<String, MethodFunction> m_methodRegistry;
};

// Modern implementation pattern (recommended)
class MyScriptInterface : public IScriptableObject {
    void InitializeMethodRegistry() override {
        m_methodRegistry["doSomething"] = [this](ScriptArgs const& args) {
            return ExecuteDoSomething(args);
        };
        m_methodRegistry["getValue"] = [this](ScriptArgs const& args) {
            return ExecuteGetValue(args);
        };
    }

    ScriptMethodResult CallMethod(String const& methodName, ScriptArgs const& args) override {
        auto it = m_methodRegistry.find(methodName);
        if (it != m_methodRegistry.end()) {
            return it->second(args);
        }
        return ScriptMethodResult::Error("Unknown method: " + methodName);
    }
};
```

### Function Binding System
```cpp
using ScriptFunction = std::function<std::any(std::vector<std::any> const&)>;

// Example function binding
ScriptFunction logFunction = [](std::vector<std::any> const& args) -> std::any {
    if (!args.empty()) {
        std::string message = std::any_cast<std::string>(args[0]);
        g_devConsole->AddLine(Rgba8::WHITE, message);
    }
    return std::any{};
};
```

## Key Dependencies and Configuration

### External Dependencies
- **Google V8 Engine**: JavaScript runtime (`v8-v143-x64.13.0.245.25`)
- **V8 Platform**: Multi-threading and memory management
- **libplatform**: V8 platform abstraction layer

### Internal Dependencies
- Core module for string utilities and basic types
- Input module for `InputScriptInterface` integration
- Event system for script-driven events

### Configuration Structure
```cpp
struct sV8SubsystemConfig {
    bool        enableDebugging      = false;     // Enable V8 debugging features
    size_t      heapSizeLimit        = 256;       // Heap size limit in MB
    bool        enableScriptBindings = true;      // Enable C++ object bindings
    std::string scriptPath           = "Data/Scripts/"; // Script file directory
    bool        enableConsoleOutput  = true;      // Enable console.log output
};
```

## Data Models

### Script Interface Management
```cpp
struct MethodCallbackData {
    std::shared_ptr<IScriptableObject> m_object;
    std::string                        m_methodName;
};

// Memory management for V8 callbacks
std::vector<std::unique_ptr<MethodCallbackData>> m_methodCallbacks;
std::vector<std::unique_ptr<ScriptFunction>>     m_functionCallbacks;
```

### Execution Statistics
```cpp
struct ExecutionStats {
    size_t scriptsExecuted    = 0;
    size_t errorsEncountered  = 0;
    size_t totalExecutionTime = 0; // milliseconds
};

struct MemoryUsage {
    size_t usedHeapSize    = 0;       // Used heap size (bytes)
    size_t totalHeapSize   = 0;       // Total heap size (bytes)
    size_t heapSizeLimit   = 0;       // Heap size limit (bytes)
    double usagePercentage = 0.0;     // Usage percentage
};
```

### Type Conversion System
```cpp
// Automatic conversion between C++ std::any and V8 values
void* ConvertToV8Value(const std::any& value);
std::any ConvertFromV8Value(void* v8Value);

// Supported types: string, int, float, double, bool, vectors, custom objects
```

## Testing and Quality

### Built-in Testing Features
- **Execution Statistics**: Track script performance and error rates
- **Memory Monitoring**: Heap usage and garbage collection metrics
- **Error Reporting**: Comprehensive JavaScript error capturing
- **Debug Output**: Console.log integration with engine logging

### Current Testing Approach
- Manual script execution through console commands
- Interactive debugging via developer console
- Memory usage monitoring for leak detection
- Performance profiling of script execution times

### Quality Assurance Features
- Automatic garbage collection management
- Script path validation and security checks
- Memory limit enforcement
- Comprehensive error handling and reporting

### Recommended Testing Additions
- Automated JavaScript unit test runner
- Performance benchmarks for binding operations
- Memory leak detection in long-running scripts
- Cross-platform V8 compatibility testing

## FAQ

### Q: How do I expose a new C++ class to JavaScript?
A: 1) Inherit from `IScriptableObject`, 2) Implement required virtual methods, 3) Register with `RegisterScriptableObject()`.

### Q: What JavaScript features are supported?
A: Full ES6+ support via V8 engine, including async/await, modules, classes, and modern JavaScript APIs.

### Q: How do I handle script errors gracefully?
A: Check `HasError()` after script execution, use `GetLastError()` for details, and `ClearError()` to reset state.

### Q: Can scripts access engine systems directly?
A: Scripts access engine systems through registered scriptable objects (like `InputScriptInterface`) and global functions.

### Q: How is memory managed between C++ and JavaScript?
A: V8 handles JavaScript memory automatically. C++ objects use shared_ptr for safe cross-boundary management.

### Q: Is the scripting system thread-safe?
A: V8 isolates provide thread isolation. Each V8Subsystem instance should be used from a single thread.

## Related Files

### Core Implementation
- `V8Subsystem.cpp` - Main V8 integration implementation
- `IScriptableObject.cpp` - Base scriptable object implementation

### Script Interfaces
- `InputScriptInterface.cpp` - Input system JavaScript bindings
- `InputScriptInterface.hpp` - Input API exposure definitions

### Integration Examples
The module is designed to be extended with additional scriptable interfaces for:
- Audio system control
- Renderer state manipulation  
- Resource loading and management
- Game-specific object interactions

## Changelog

- 2025-10-07: **Method Registry Pattern for Scriptable Objects**
  - Introduced `InitializeMethodRegistry()` virtual method for structured method registration
  - Added `MethodFunction` type alias: `std::function<ScriptMethodResult(ScriptArgs const&)>`
  - Implemented `m_methodRegistry` member for O(1) method dispatch lookup
  - Modernized `CallMethod` signature to use `ScriptMethodResult` and `ScriptArgs`
  - Simplified `ScriptMethodInfo` to use `StringList` instead of `std::vector<String>`
  - Eliminates repetitive if-else chains in favor of map-based dispatch
  - Improved type safety and compile-time error detection for method signatures
  - Updated InputScriptInterface and AudioScriptInterface to use new pattern
- 2025-09-06 19:54:50: Initial module documentation created
- Recent developments: InputScriptInterface and AudioScriptInterface with method registry pattern, RendererScriptInterface for graphics API exposure, comprehensive error handling and memory management improvements