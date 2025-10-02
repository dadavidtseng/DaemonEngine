//----------------------------------------------------------------------------------------------------
// ScriptSubsystem.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
//----------------------------------------------------------------------------------------------------
#include "Engine/Scripting/IScriptableObject.hpp"
//----------------------------------------------------------------------------------------------------
#include <functional>
#include <mutex>
#include <queue>
#include <set>
#include <unordered_set>

// Forward declarations
class ChromeDevToolsWebSocketSubsystem;
class FileWatcher;
class ScriptReloader;

//----------------------------------------------------------------------------------------------------
using ScriptFunction = std::function<std::any(std::vector<std::any> const&)>;

//----------------------------------------------------------------------------------------------------
struct MethodCallbackData
{
    std::shared_ptr<IScriptableObject> object;
    std::string                        methodName;
};

struct PropertyCallbackData
{
    std::shared_ptr<IScriptableObject> object;
    std::string                        propertyName;
};

//----------------------------------------------------------------------------------------------------
struct sScriptSubsystemConfig
{
    bool        enableDebugging      = false;        // Enable V8 debugging functionality
    size_t      heapSizeLimit        = 256;          // Heap size limit (MB)
    bool        enableScriptBindings = true;         // Enable script bindings
    std::string scriptPath           = "Data/Scripts/"; // Script file path
    bool        enableConsoleOutput  = true;         // Enable console.log output

    // Chrome DevTools Inspector Configuration
#ifdef _DEBUG
    bool enableInspector = true;         // Enable Chrome DevTools integration (default in Debug)
#else
    bool enableInspector = false;        // Disable Chrome DevTools in Release
#endif
    int         inspectorPort   = 9229;         // Chrome DevTools connection port
    std::string inspectorHost   = "127.0.0.1"; // Inspector server bind address (localhost only)
    bool        waitForDebugger = false;        // Pause JavaScript execution until debugger connects

    // Hot-reload configuration
    bool enableHotReload = true;         // Enable hot-reload functionality
};

//----------------------------------------------------------------------------------------------------
// ScriptSubsystem - Merged V8 and Hot-reload functionality
// Provides JavaScript execution environment with hot-reload capabilities
// Supports Chrome DevTools integration and script bindings
//----------------------------------------------------------------------------------------------------
class ScriptSubsystem
{
public:
    explicit ScriptSubsystem(sScriptSubsystemConfig config);
    ~ScriptSubsystem();

    //------------------------------------------------------------------------------------------------
    // Subsystem lifecycle
    //------------------------------------------------------------------------------------------------
    void Startup();
    void Shutdown();
    void Update();

    //------------------------------------------------------------------------------------------------
    // Script execution
    //------------------------------------------------------------------------------------------------
    bool ExecuteScript(String const& script);
    bool ExecuteScriptFile(String const& scriptFilename);

    // SCRIPT REGISTRY APPROACH: Selective Chrome DevTools integration
    bool ExecuteRegisteredScript(String const& script, String const& scriptName);
    bool ExecuteUnregisteredScript(String const& script);

    // Execute JavaScript code and return result
    std::any ExecuteScriptWithResult(std::string const& script);

    //------------------------------------------------------------------------------------------------
    // Error handling and status queries
    //------------------------------------------------------------------------------------------------

    // Check for errors
    bool HasError() const;

    // Get last error message
    String GetLastError() const;

    // Get last execution result
    String GetLastResult() const;

    // Check if V8 is initialized
    bool IsInitialized() const;

    // Clear error state
    void ClearError();

    //------------------------------------------------------------------------------------------------
    // Object and function registration
    //------------------------------------------------------------------------------------------------
    void RegisterScriptableObject(String const& name, std::shared_ptr<IScriptableObject> const& object);
    void UnregisterScriptableObject(String const& name);
    void RegisterGlobalFunction(String const& name, ScriptFunction const& function);
    void UnregisterGlobalFunction(String const& name);

    // Check if object/function is registered
    bool HasRegisteredObject(String const& name) const;
    bool HasRegisteredFunction(String const& name) const;

    // Get all registered names
    StringList GetRegisteredObjectNames() const;
    StringList GetRegisteredFunctionNames() const;

    //------------------------------------------------------------------------------------------------
    // Debug and tool functions
    //------------------------------------------------------------------------------------------------

    // Set debug output enabled
    void SetDebugOutput(bool enabled);

    // Get JavaScript execution statistics
    struct ExecutionStats
    {
        size_t scriptsExecuted    = 0;
        size_t errorsEncountered  = 0;
        size_t totalExecutionTime = 0; // milliseconds
    };

    ExecutionStats GetExecutionStats() const;

    // Reset execution statistics
    void ResetExecutionStats();

    //------------------------------------------------------------------------------------------------
    // Memory management
    //------------------------------------------------------------------------------------------------

    // Force garbage collection
    void ForceGarbageCollection();

    // Get memory usage
    struct MemoryUsage
    {
        size_t usedHeapSize    = 0;       // Used heap size (bytes)
        size_t totalHeapSize   = 0;      // Total heap size (bytes)
        size_t heapSizeLimit   = 0;      // Heap size limit (bytes)
        double usagePercentage = 0.0;  // Usage percentage
    };

    MemoryUsage GetMemoryUsage() const;

    //------------------------------------------------------------------------------------------------
    // Hot-reload functionality
    //------------------------------------------------------------------------------------------------

    // Initialize hot-reload system
    bool InitializeHotReload(const std::string& projectRoot);

    // Hot-reload configuration and control
    void SetHotReloadEnabled(bool enabled);
    bool IsHotReloadEnabled() const { return m_hotReloadEnabled; }
    
    // File watching management
    void AddWatchedFile(const std::string& relativePath);
    void RemoveWatchedFile(const std::string& relativePath);
    std::vector<std::string> GetWatchedFiles() const;
    
    // Manual reload trigger
    void ReloadScript(const std::string& relativePath);

    // Thread-safe event processing (called from main thread)
    void ProcessPendingEvents();

    //------------------------------------------------------------------------------------------------
    // Chrome DevTools Support
    //------------------------------------------------------------------------------------------------
    std::string HandleDebuggerGetScriptSource(const std::string& scriptId);
    void        ReplayScriptsToDevTools();
    void        StoreScriptIdMapping(const std::string& scriptId, const std::string& url);
    void        StoreScriptNotificationForReplay(const std::string& notification);

    // DevTools Panel Event Generation
    void SendPerformanceTimelineEvent(const std::string& eventType, const std::string& name, double timestamp);
    void SendNetworkRequestEvent(const std::string& url, const std::string& method, int statusCode);
    void SendMemoryHeapSnapshot();

private:
    //----------------------------------------------------------------------------------------------------
    /// @brief Internal implementation struct for v8
    /// @see https://www.geeksforgeeks.org/cpp/pimpl-idiom-in-c-with-examples/
    struct V8Implementation;

    //------------------------------------------------------------------------------------------------
    // V8 Private methods
    //------------------------------------------------------------------------------------------------
    bool ExecuteScriptWithOrigin(String const& script, String const& scriptName);

    // Chrome DevTools Script Management (Private)
    std::string ConvertToDevToolsURL(const std::string& scriptPath);
    void        StoreScriptSource(const std::string& url, const std::string& source);
    std::string GetScriptSourceByURL(const std::string& url);
    void        ForwardConsoleMessageToDevTools(const std::string& message);

    // V8 Engine lifecycle
    bool InitializeV8Engine() const;
    void ShutdownV8Engine();

    // V8 bindings setup
    void SetupV8Bindings();

    // Create single object/function bindings (incremental binding)
    void CreateSingleObjectBinding(String const&                             objectName,
                                   std::shared_ptr<IScriptableObject> const& object);
    void CreateSingleFunctionBinding(String const&         functionName,
                                     ScriptFunction const& function);

    // Set up built-in JavaScript objects and functions
    void SetupBuiltinObjects();

    // Error handling
    void HandleV8Error(String const& error);

    // Convert between C++ std::any and V8 values
    void* ConvertToV8Value(const std::any& value);
    std::any ConvertFromV8Value(void* v8Value);

    // Validate script file path
    std::string ValidateScriptPath(std::string const& filename) const;

    //------------------------------------------------------------------------------------------------
    // Hot-reload private methods
    //------------------------------------------------------------------------------------------------

    // Hot-reload callbacks
    void OnFileChanged(const std::string& filePath);
    void OnReloadComplete(bool success, const std::string& error);

    // Helper method to construct absolute paths (same logic as FileWatcher)
    std::string GetAbsoluteScriptPath(const std::string& relativePath) const;

    //------------------------------------------------------------------------------------------------
    // Member variables
    //------------------------------------------------------------------------------------------------

    // Pointer to the internal V8 implementation
    std::unique_ptr<V8Implementation> m_impl;

    sScriptSubsystemConfig m_config;

    // Registered script objects
    std::unordered_map<String, std::shared_ptr<IScriptableObject>> m_scriptableObjects;

    // Registered global functions
    std::unordered_map<String, ScriptFunction> m_globalFunctions;

    // Status tracking
    bool        m_isInitialized = false;
    bool        m_hasError      = false;
    std::string m_lastError;
    std::string m_lastResult;

    // Execution statistics
    ExecutionStats m_stats;

    // Binding tracking (prevent duplicate binding)
    std::set<String> m_boundObjects;    // Bound objects
    std::set<String> m_boundFunctions;  // Bound functions

    // Chrome DevTools Integration
    std::unique_ptr<ChromeDevToolsWebSocketSubsystem> m_devToolsServer;

    // Script Source Storage for DevTools
    std::unordered_map<std::string, std::string> m_scriptSources; // URL -> Source Code
    std::unordered_map<std::string, std::string> m_scriptIdToURL; // Script ID -> URL

    // SCRIPT REGISTRY: Selective Chrome DevTools integration
    std::unordered_set<std::string>              m_registeredScripts; // Scripts that should appear in DevTools
    std::unordered_map<std::string, std::string> m_scriptRegistry; // Name -> Source code

    // Priority-based script notification storage for better Chrome DevTools experience
    std::vector<std::string> m_priorityScriptNotifications; // High-priority scripts (JSEngine.js, JSGame.js)
    std::vector<std::string> m_scriptNotifications;         // Regular script notifications

    // Callback data storage (avoid memory leaks)
    std::vector<std::unique_ptr<MethodCallbackData>>   m_methodCallbacks;
    std::vector<std::unique_ptr<PropertyCallbackData>> m_propertyCallbacks;
    std::vector<std::unique_ptr<ScriptFunction>>       m_functionCallbacks;

    //------------------------------------------------------------------------------------------------
    // Hot-reload components and state
    //------------------------------------------------------------------------------------------------
    
    // Core hot-reload components
    std::unique_ptr<FileWatcher>    m_fileWatcher;
    std::unique_ptr<ScriptReloader> m_scriptReloader;
    
    // Hot-reload configuration
    bool        m_hotReloadEnabled{false};
    std::string m_projectRoot;
    
    // Thread-safe event queue for main thread processing
    std::queue<std::string> m_pendingFileChanges;
    mutable std::mutex      m_fileChangeQueueMutex;
};