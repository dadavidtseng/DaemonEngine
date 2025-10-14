//----------------------------------------------------------------------------------------------------
// ScriptSubsystem.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Script/ScriptSubsystem.hpp"
#include "Game/EngineBuildPreferences.hpp"
#if !defined( ENGINE_DISABLE_SCRIPT )
//----------------------------------------------------------------------------------------------------
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/LogSubsystem.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Network/ChromeDevToolsWebSocketSubsystem.hpp"
#include "Engine/Script/FileWatcher.hpp"
#include "Engine/Script/ModuleLoader.hpp"
#include "Engine/Script/ScriptReloader.hpp"
#include "ThirdParty/json/json.hpp"
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
// Any changes that you made to the warning state between push and pop are undone.
//----------------------------------------------------------------------------------------------------
#pragma warning(push)           // stores the current warning state for every warning

#pragma warning(disable: 4100)  // 'identifier' : unreferenced formal parameter
#pragma warning(disable: 4127)  // conditional expression is constant
#pragma warning(disable: 4324)  // 'structname': structure was padded due to alignment specifier

#include "v8-inspector.h"
#include "v8.h"
#include "libplatform/libplatform.h"

#pragma warning(pop)            // pops the last warning state pushed onto the stack
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
// Chrome DevTools Inspector Implementation
//----------------------------------------------------------------------------------------------------

// Channel implementation for Chrome DevTools communication
class V8InspectorChannelImpl : public v8_inspector::V8Inspector::Channel
{
public:
    explicit V8InspectorChannelImpl(ScriptSubsystem* scriptSubsystem, ChromeDevToolsWebSocketSubsystem* devToolsServer)
        : m_scriptSubsystem(scriptSubsystem), m_devToolsServer(devToolsServer)
    {
    }

    void sendResponse(int callId, std::unique_ptr<v8_inspector::StringBuffer> message) override
    {
        (void)callId; // Suppress unused parameter warning
        std::string response = StringViewToStdString(message->string());

        // Send response to Chrome DevTools client
        if (m_devToolsServer)
        {
            m_devToolsServer->SendToDevTools(response);
        }
        else
        {
            DAEMON_LOG(LogScript, eLogVerbosity::Error, StringFormat("Cannot send response: Chrome DevTools server is null"));
        }
    }

    void sendNotification(std::unique_ptr<v8_inspector::StringBuffer> message) override
    {
        std::string notification = StringViewToStdString(message->string());

        // Parse and store script information for replay when new DevTools connects
        if (notification.find("\"method\":\"Debugger.scriptParsed\"") != std::string::npos)
        {
            StoreScriptNotification(notification);
        }

        // Send notification to Chrome DevTools client
        if (m_devToolsServer)
        {
            m_devToolsServer->SendToDevTools(notification);
        }
    }

    void flushProtocolNotifications() override
    {
        // Protocol notifications are sent immediately, no buffering needed
    }

    // Set the Chrome DevTools server after it's created
    void SetDevToolsServer(ChromeDevToolsWebSocketSubsystem* devToolsServer)
    {
        m_devToolsServer = devToolsServer;
    }

    // Store script parsed notifications for replay
    void StoreScriptNotification(const std::string& notification)
    {
        // Extract script ID and URL from the notification for script ID mapping
        std::string scriptId = ExtractJsonString(notification, "scriptId");
        std::string url      = ExtractJsonString(notification, "url");

        if (!scriptId.empty() && !url.empty() && m_scriptSubsystem)
        {
            // Store the script ID to URL mapping for Debugger.getScriptSource
            m_scriptSubsystem->StoreScriptIdMapping(scriptId, url);
        }

        // Store the complete notification for replay
        m_scriptSubsystem->StoreScriptNotificationForReplay(notification);
    }

private:
    ScriptSubsystem*                  m_scriptSubsystem;
    ChromeDevToolsWebSocketSubsystem* m_devToolsServer;

    std::string StringViewToStdString(const v8_inspector::StringView& view)
    {
        if (view.is8Bit())
        {
            return std::string(reinterpret_cast<const char*>(view.characters8()), view.length());
        }
        else
        {
            // Convert UTF-16 to UTF-8
            std::string     result;
            const uint16_t* chars = view.characters16();
            for (size_t i = 0; i < view.length(); ++i)
            {
                if (chars[i] < 128)
                {
                    result += static_cast<char>(chars[i]);
                }
                else
                {
                    result += '?'; // Simple fallback for non-ASCII characters
                }
            }
            return result;
        }
    }

    // Simple JSON string extractor
    std::string ExtractJsonString(const std::string& json, const std::string& key)
    {
        std::string searchKey = "\"" + key + "\":";
        size_t      keyPos    = json.find(searchKey);
        if (keyPos == std::string::npos) return "";

        size_t valueStart = json.find("\"", keyPos + searchKey.length());
        if (valueStart == std::string::npos) return "";
        valueStart++; // Skip opening quote

        size_t valueEnd = json.find("\"", valueStart);
        if (valueEnd == std::string::npos) return "";

        return json.substr(valueStart, valueEnd - valueStart);
    }
};

// V8InspectorClient implementation
class V8InspectorClientImpl : public v8_inspector::V8InspectorClient
{
public:
    explicit V8InspectorClientImpl(ScriptSubsystem* scriptSubsystem)
    {
        UNUSED(scriptSubsystem); // Suppress unreferenced parameter warning
    }

    void runMessageLoopOnPause(int contextGroupId) override
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("Chrome DevTools: Paused on context group {}", contextGroupId));
        // Message loop handling would go here for breakpoint debugging
    }

    void quitMessageLoopOnPause() override
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("Chrome DevTools: Quit message loop on pause"));
    }

    void runIfWaitingForDebugger(int contextGroupId) override
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("Chrome DevTools: Run if waiting for debugger on context group {}", contextGroupId));
    }

    void consoleAPIMessage(int                             contextGroupId,
                           v8::Isolate::MessageErrorLevel  level,
                           const v8_inspector::StringView& message,
                           const v8_inspector::StringView& url,
                           unsigned                        lineNumber,
                           unsigned                        columnNumber,
                           v8_inspector::V8StackTrace*) override
    {
        (void)contextGroupId; // Suppress unused parameter warning
        std::string msg    = StringViewToStdString(message);
        std::string urlStr = StringViewToStdString(url);

        const char* levelStr = "Unknown";
        switch (level)
        {
        case v8::Isolate::kMessageLog: levelStr = "Log";
            break;
        case v8::Isolate::kMessageDebug: levelStr = "Debug";
            break;
        case v8::Isolate::kMessageInfo: levelStr = "Info";
            break;
        case v8::Isolate::kMessageError: levelStr = "Error";
            break;
        case v8::Isolate::kMessageWarning: levelStr = "Warning";
            break;
        }

        DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("JS Console [{}]: {} ({}:{}:{})",
                                                               levelStr, msg, urlStr, lineNumber, columnNumber));
    }

    double currentTimeMS() override
    {
        return GetCurrentTimeSeconds() * 1000.0;
    }

private:
    std::string StringViewToStdString(const v8_inspector::StringView& view)
    {
        if (view.is8Bit())
        {
            return std::string(reinterpret_cast<const char*>(view.characters8()), view.length());
        }
        else
        {
            // Convert UTF-16 to UTF-8
            std::string     result;
            const uint16_t* chars = view.characters16();
            for (size_t i = 0; i < view.length(); ++i)
            {
                if (chars[i] < 128)
                {
                    result += static_cast<char>(chars[i]);
                }
                else
                {
                    result += '?'; // Simple fallback for non-ASCII characters
                }
            }
            return result;
        }
    }
};

//----------------------------------------------------------------------------------------------------
struct ScriptSubsystem::V8Implementation
{
    std::unique_ptr<v8::Platform>               platform;
    v8::Isolate::CreateParams                   createParams;
    v8::Isolate*                                isolate = nullptr;
    v8::Global<v8::Context>                     globalContext;              // v8 persistent handle
    std::unique_ptr<v8::ArrayBuffer::Allocator> allocator;
    bool                                        isInitialized      = false;
    double                                      lastExecutionStart = 0.0;

    // Chrome DevTools Inspector Components
    std::unique_ptr<V8InspectorClientImpl>            inspectorClient;
    std::unique_ptr<v8_inspector::V8Inspector>        inspector;
    std::unique_ptr<v8_inspector::V8InspectorSession> inspectorSession;
    std::unique_ptr<V8InspectorChannelImpl>           inspectorChannel;
    static int constexpr                              kContextGroupId = 1;        // Context group ID for debugging
};

//----------------------------------------------------------------------------------------------------
ScriptSubsystem::ScriptSubsystem(sScriptSubsystemConfig config)
    : m_impl(std::make_unique<V8Implementation>()),
      m_config(std::move(config)),
      m_fileWatcher(std::make_unique<FileWatcher>()),
      m_scriptReloader(std::make_unique<ScriptReloader>())
{
    DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("ScriptSubsystem: Created"));
}

//----------------------------------------------------------------------------------------------------
ScriptSubsystem::~ScriptSubsystem()
{
    Shutdown();
    DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("ScriptSubsystem: Destroyed"));
}

//----------------------------------------------------------------------------------------------------
void ScriptSubsystem::Startup()
{
    DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("(ScriptSubsystem::Startup)(start)"));

    if (m_isInitialized)
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Warning, StringFormat("(ScriptSubsystem::Startup)(ScriptSubsystem has already initialized, skip...)"));
        return;
    }

    if (!InitializeV8Engine())
    {
        HandleV8Error(StringFormat("Failed to initialize V8 engine"));
        return;
    }

    m_isInitialized = true;

    SetupV8Bindings();

    // Initialize Chrome DevTools server if inspector is enabled
    if (m_config.enableInspector)
    {
        // Load Chrome DevTools configuration from JSON file
        sChromeDevToolsConfig devToolsConfig;

        try
        {
            std::ifstream configFile("Data/Config/WebSocketConfig.json");
            if (configFile.is_open())
            {
                nlohmann::json jsonConfig;
                configFile >> jsonConfig;

                if (jsonConfig.contains("chromeDevTools"))
                {
                    devToolsConfig = sChromeDevToolsConfig::FromJSON(jsonConfig["chromeDevTools"]);
                    DAEMON_LOG(LogScript, eLogVerbosity::Log,
                               StringFormat("Loaded Chrome DevTools config from JSON: {}:{}",
                                            devToolsConfig.host, devToolsConfig.port));
                }
                else
                {
                    DAEMON_LOG(LogScript, eLogVerbosity::Warning,
                               StringFormat("chromeDevTools section not found in WebSocketConfig.json, using defaults"));
                    // Use defaults from m_config
                    devToolsConfig.enabled     = true;
                    devToolsConfig.host        = m_config.inspectorHost;
                    devToolsConfig.port        = m_config.inspectorPort;
                    devToolsConfig.contextName = "ProtogameJS3D JavaScript Context";
                }
            }
            else
            {
                DAEMON_LOG(LogScript, eLogVerbosity::Warning,
                           StringFormat("WebSocketConfig.json not found, using defaults from sScriptSubsystemConfig"));
                // Fallback to existing hardcoded values
                devToolsConfig.enabled     = true;
                devToolsConfig.host        = m_config.inspectorHost;
                devToolsConfig.port        = m_config.inspectorPort;
                devToolsConfig.contextName = "ProtogameJS3D JavaScript Context";
            }
        }
        catch (nlohmann::json::exception const& e)
        {
            DAEMON_LOG(LogScript, eLogVerbosity::Error,
                       StringFormat("JSON parsing error in WebSocketConfig.json: {}", e.what()));
            // Fallback to existing hardcoded values
            devToolsConfig.enabled     = true;
            devToolsConfig.host        = m_config.inspectorHost;
            devToolsConfig.port        = m_config.inspectorPort;
            devToolsConfig.contextName = "ProtogameJS3D JavaScript Context";
        }

        m_devToolsServer = std::make_unique<ChromeDevToolsWebSocketSubsystem>(devToolsConfig, this);

        if (m_devToolsServer->Start())
        {
            // Connect the DevTools server to the V8 Inspector
            if (m_impl->inspector && m_impl->inspectorSession)
            {
                m_devToolsServer->SetInspector(m_impl->inspector.get(), m_impl->inspectorSession.get());
            }

            // Update the inspector channel to use the DevTools server
            if (m_impl->inspectorChannel)
            {
                m_impl->inspectorChannel.get()->SetDevToolsServer(m_devToolsServer.get());
            }

            DAEMON_LOG(LogScript, eLogVerbosity::Display,
                       StringFormat("Chrome DevTools server started successfully on {}:{}",
                                    devToolsConfig.host, devToolsConfig.port));
        }
        else
        {
            DAEMON_LOG(LogScript, eLogVerbosity::Error,
                       StringFormat("Failed to start Chrome DevTools server on {}:{}",
                                    devToolsConfig.host, devToolsConfig.port));
        }
    }

    // Initialize ES6 module loader if enabled
    if (m_config.enableModules)
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("ScriptSubsystem: Initializing ES6 module loader..."));

        m_moduleLoader = std::make_unique<ModuleLoader>(this, m_config.scriptPath);

        if (m_moduleLoader)
        {
            DAEMON_LOG(LogScript, eLogVerbosity::Display,
                       StringFormat("ES6 module system initialized with base path: {}", m_config.scriptPath));
        }
        else
        {
            DAEMON_LOG(LogScript, eLogVerbosity::Error, StringFormat("Failed to initialize ES6 module loader"));
        }
    }

    DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("(ScriptSubsystem::Startup)(end)"));
}

//----------------------------------------------------------------------------------------------------
void ScriptSubsystem::Shutdown()
{
    DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("(ScriptSubsystem::Shutdown)(start)"));

    if (!m_isInitialized)
    {
        return;
    }

    // Shutdown hot-reload first
    if (m_hotReloadEnabled)
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("ScriptSubsystem: Shutting down hot-reload system..."));

        m_hotReloadEnabled = false;

        if (m_fileWatcher)
        {
            m_fileWatcher->Shutdown();
        }

        if (m_scriptReloader)
        {
            m_scriptReloader->Shutdown();
        }

        // Clear any pending events
        {
            std::lock_guard<std::mutex> lock(m_fileChangeQueueMutex);
            while (!m_pendingFileChanges.empty())
            {
                m_pendingFileChanges.pop();
            }
        }

        DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("ScriptSubsystem: Hot-reload system shutdown complete"));
    }

    // Shutdown Chrome DevTools server
    if (m_devToolsServer)
    {
        m_devToolsServer->Stop();
        m_devToolsServer.reset();
    }

    // Clean up callback data
    m_methodCallbacks.clear();
    m_functionCallbacks.clear();

    // Clean up registered objects and functions
    m_scriptableObjects.clear();
    m_globalFunctions.clear();

    // Clear tracking sets
    m_boundObjects.clear();
    m_boundFunctions.clear();

    ShutdownV8Engine();

    m_isInitialized = false;

    DAEMON_LOG(LogScript, eLogVerbosity::Display, StringFormat("(ScriptSubsystem::Shutdown)(end)"));
}

//----------------------------------------------------------------------------------------------------
void ScriptSubsystem::Update()
{
    if (!m_isInitialized)
    {
        return;
    }

    // Update hot-reload if enabled
    if (m_hotReloadEnabled)
    {
        // Process pending file changes on main thread
        ProcessPendingEvents();
    }

    // Update Chrome DevTools server if it's running
    if (m_devToolsServer && m_devToolsServer->IsRunning())
    {
        m_devToolsServer->Update();

        // THREAD SAFETY FIX: Process queued V8 Inspector messages on main thread
        m_devToolsServer->ProcessQueuedMessages();

        // DEVTOOLS PANEL POPULATION: Generate sample events for panels
        static int updateCounter = 0;
        updateCounter++;

        // Generate Performance timeline events every 60 frames (~1 second at 60fps)
        if (updateCounter % 60 == 0)
        {
            double timestamp = static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count());
            DAEMON_LOG(LogScript, eLogVerbosity::Display,
                       StringFormat("DEVTOOLS DEBUG: Triggering Performance event (frame {})", updateCounter));
            SendPerformanceTimelineEvent("ScriptUpdate", "JSEngine.update", timestamp);
        }

        // Generate Network request events every 120 frames (~2 seconds)
        if (updateCounter % 120 == 0)
        {
            DAEMON_LOG(LogScript, eLogVerbosity::Display,
                       StringFormat("DEVTOOLS DEBUG: Triggering Network event (frame {})", updateCounter));
            SendNetworkRequestEvent("file:///FirstV8/Scripts/main.js", "GET", 200);
        }

        // Generate Memory heap snapshots every 300 frames (~5 seconds)
        if (updateCounter % 300 == 0)
        {
            DAEMON_LOG(LogScript, eLogVerbosity::Display,
                       StringFormat("DEVTOOLS DEBUG: Triggering Memory snapshot (frame {})", updateCounter));
            SendMemoryHeapSnapshot();
        }
    }
}

//----------------------------------------------------------------------------------------------------
// Initialize hot-reload system with project root path
bool ScriptSubsystem::InitializeHotReload(const std::string& projectRoot)
{
    if (!m_config.enableHotReload)
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("ScriptSubsystem: Hot-reload disabled in configuration"));
        return true; // Not an error, just disabled
    }

    try
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("ScriptSubsystem: Initializing hot-reload system..."));

        // Store project root for path construction
        m_projectRoot = projectRoot;

        // Initialize FileWatcher
        if (!m_fileWatcher->Initialize(projectRoot))
        {
            DAEMON_LOG(LogScript, eLogVerbosity::Error, StringFormat("ScriptSubsystem: Failed to initialize FileWatcher"));
            return false;
        }

        // Initialize ScriptReloader with ModuleLoader support (Phase 5)
        if (!m_scriptReloader->Initialize(this, m_moduleLoader.get()))
        {
            DAEMON_LOG(LogScript, eLogVerbosity::Error, StringFormat("ScriptSubsystem: Failed to initialize ScriptReloader"));
            return false;
        }

        // Set up callbacks
        m_fileWatcher->SetChangeCallback([this](const std::string& filePath)
        {
            OnFileChanged(filePath);
        });

        m_scriptReloader->SetReloadCompleteCallback([this](bool success, const std::string& error)
        {
            OnReloadComplete(success, error);
        });

        // Add default watched files - Phase 4 ES6 Module System
        // Watch all ES6 module files for hot-reload support
        m_fileWatcher->AddWatchedFile("Data/Scripts/main.js");
        m_fileWatcher->AddWatchedFile("Data/Scripts/InputSystemCommon.js");
        m_fileWatcher->AddWatchedFile("Data/Scripts/JSEngine.js");
        m_fileWatcher->AddWatchedFile("Data/Scripts/JSGame.js");
        m_fileWatcher->AddWatchedFile("Data/Scripts/core/Subsystem.js");
        m_fileWatcher->AddWatchedFile("Data/Scripts/components/CppBridgeSystem.js");
        m_fileWatcher->AddWatchedFile("Data/Scripts/components/InputSystem.js");
        m_fileWatcher->AddWatchedFile("Data/Scripts/components/AudioSystem.js");

        // Start the file watching thread
        m_fileWatcher->StartWatching();

        m_hotReloadEnabled = true;
        DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("ScriptSubsystem: Hot-reload system initialized successfully"));

        return true;
    }
    catch (const std::exception& e)
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Error, StringFormat("ScriptSubsystem: Initialize hot-reload failed with exception: {}", e.what()));
        return false;
    }
}

//----------------------------------------------------------------------------------------------------
// Hot-reload functionality
//----------------------------------------------------------------------------------------------------

void ScriptSubsystem::SetHotReloadEnabled(bool enabled)
{
    m_hotReloadEnabled = enabled;
    DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("ScriptSubsystem: Hot-reload {}", enabled ? "enabled" : "disabled"));
}

void ScriptSubsystem::AddWatchedFile(const std::string& relativePath)
{
    if (m_fileWatcher)
    {
        m_fileWatcher->AddWatchedFile(relativePath);
        DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("ScriptSubsystem: Added watched file: {}", relativePath));
    }
}

void ScriptSubsystem::RemoveWatchedFile(const std::string& relativePath)
{
    if (m_fileWatcher)
    {
        m_fileWatcher->RemoveWatchedFile(relativePath);
        DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("ScriptSubsystem: Removed watched file: {}", relativePath));
    }
}

std::vector<std::string> ScriptSubsystem::GetWatchedFiles() const
{
    if (m_fileWatcher)
    {
        return m_fileWatcher->GetWatchedFiles();
    }
    return {};
}

void ScriptSubsystem::ReloadScript(const std::string& relativePath)
{
    if (m_scriptReloader && m_hotReloadEnabled)
    {
        std::string absolutePath = GetAbsoluteScriptPath(relativePath);
        m_scriptReloader->ReloadScript(absolutePath);
        DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("ScriptSubsystem: Manual reload triggered for: {}", relativePath));
    }
}

void ScriptSubsystem::ProcessPendingEvents()
{
    try
    {
        // PHASE 5: Add V8 scope safety (Solution B from Q3)
        // Get V8 isolate first (this is safe without scopes)
        void* isolatePtr = GetV8Isolate();

        if (isolatePtr)
        {
            v8::Isolate* isolate = static_cast<v8::Isolate*>(isolatePtr);

            // CRITICAL: Enter isolate scope BEFORE accessing context
            v8::Isolate::Scope isolateScope(isolate);
            v8::HandleScope    handleScope(isolate);

            // Now safe to get context (requires isolate scope to be active)
            void* contextPtr = GetV8Context();

            if (contextPtr)
            {
                v8::Local<v8::Context> context = *static_cast<v8::Local<v8::Context>*>(contextPtr);
                v8::Context::Scope     contextScope(context);

                // Process all pending file changes on the main thread (V8-safe)
                std::queue<std::string> filesToProcess;

                // Get all pending changes under lock
                {
                    std::lock_guard<std::mutex> lock(m_fileChangeQueueMutex);
                    filesToProcess.swap(m_pendingFileChanges); // Efficiently move all items
                }

                // Process all file changes outside the lock
                while (!filesToProcess.empty())
                {
                    const std::string& filePath = filesToProcess.front();

                    DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("ScriptSubsystem: Processing file change on main thread: {}", filePath));

                    // Convert relative path to absolute path for ScriptReloader
                    std::string absolutePath = GetAbsoluteScriptPath(filePath);

                    // Now safe to call V8 from main thread with proper scopes
                    if (m_scriptReloader && m_hotReloadEnabled)
                    {
                        m_scriptReloader->ReloadScript(absolutePath);
                    }

                    filesToProcess.pop();
                }
            }
            else
            {
                DAEMON_LOG(LogScript, eLogVerbosity::Warning,
                           StringFormat("ScriptSubsystem: V8 context not available, skipping hot-reload processing"));
            }
        }
        else
        {
            DAEMON_LOG(LogScript, eLogVerbosity::Warning,
                       StringFormat("ScriptSubsystem: V8 isolate not available, skipping hot-reload processing"));
        }
    }
    catch (const std::exception& e)
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Error, StringFormat("ScriptSubsystem: Error processing pending hot-reload events: {}", e.what()));
    }
}

void ScriptSubsystem::OnFileChanged(const std::string& filePath)
{
    try
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("ScriptSubsystem: File changed (queuing for main thread): {}", filePath));

        // Queue the file change for main thread processing (thread-safe)
        if (m_hotReloadEnabled)
        {
            std::lock_guard<std::mutex> lock(m_fileChangeQueueMutex);
            m_pendingFileChanges.push(filePath);
        }
    }
    catch (const std::exception& e)
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Error, StringFormat("ScriptSubsystem: Error handling file change: {}", e.what()));
    }
}

void ScriptSubsystem::OnReloadComplete(bool success, const std::string& error)
{
    if (success)
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("ScriptSubsystem: Script reload completed successfully"));
    }
    else
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Error, StringFormat("ScriptSubsystem: Script reload failed: {}", error));
    }
}

std::string ScriptSubsystem::GetAbsoluteScriptPath(const std::string& relativePath) const
{
    // Same logic as FileWatcher::GetFullPath()
    std::filesystem::path fullPath = std::filesystem::path(m_projectRoot) / "Run" / relativePath;
    return fullPath.string();
}

//----------------------------------------------------------------------------------------------------
// Script execution methods (from V8Subsystem)
//----------------------------------------------------------------------------------------------------

bool ScriptSubsystem::ExecuteScript(String const& script)
{
    // SCRIPT REGISTRY APPROACH: Use unregistered execution for performance
    // This prevents Chrome DevTools overhead for high-frequency script calls
    // Use ExecuteRegisteredScript() explicitly for scripts that need debugging

    return ExecuteUnregisteredScript(script);
}

bool ScriptSubsystem::ExecuteScriptFile(String const& scriptFilename)
{
    if (!m_isInitialized) ERROR_AND_DIE(StringFormat("(ScriptSubsystem::ExecuteScriptFile)(ScriptSubsystem is not initialized)"))

    DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("(ScriptSubsystem::ExecuteScriptFile)(start)({})", scriptFilename));

    String const scriptFullPath = ValidateScriptPath(scriptFilename);

    // Read the script file.
    std::ifstream const file(scriptFullPath);

    if (!file.is_open())
    {
        HandleV8Error(StringFormat("Cannot open script file: {}", scriptFullPath));
        return false;
    }

    std::stringstream buffer;

    buffer << file.rdbuf();

    String const scriptContent = buffer.str();

    if (scriptContent.empty())
    {
        HandleV8Error(StringFormat("Script file is empty: {}", scriptFullPath));
        return false;
    }

    DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("(ScriptSubsystem::ExecuteScriptFile)(end)({})", scriptFilename));

    // SCRIPT REGISTRY: ExecuteScriptFile always registers scripts for Chrome DevTools debugging
    // This ensures JSEngine.js, JSGame.js, and other script files are visible in DevTools Sources panel
    m_registeredScripts.insert(scriptFilename);
    m_scriptRegistry[scriptFilename] = scriptContent;

    DAEMON_LOG(LogScript, eLogVerbosity::Display,
               StringFormat("SCRIPT REGISTRY: Registered file '{}' for Chrome DevTools debugging", scriptFilename));

    return ExecuteScriptWithOrigin(scriptContent, scriptFilename);
}

bool ScriptSubsystem::ExecuteRegisteredScript(String const& script, String const& scriptName)
{
    // Execute script with full Chrome DevTools integration for debugging
    // This method is used for important scripts that should be visible in DevTools Sources panel

    if (!m_isInitialized) ERROR_AND_DIE(StringFormat("(ScriptSubsystem::ExecuteRegisteredScript)(ScriptSubsystem is not initialized)"))

    // Register this script for Chrome DevTools debugging
    m_registeredScripts.insert(scriptName);
    m_scriptRegistry[scriptName] = script;

    DAEMON_LOG(LogScript, eLogVerbosity::Display,
               StringFormat("SCRIPT REGISTRY: Registered '{}' for Chrome DevTools debugging", scriptName));

    // Execute with full Chrome DevTools integration
    return ExecuteScriptWithOrigin(script, scriptName);
}

bool ScriptSubsystem::ExecuteUnregisteredScript(String const& script)
{
    // High-performance script execution without Chrome DevTools registration
    // Used for high-frequency calls to prevent performance overhead

    if (!m_isInitialized) ERROR_AND_DIE(StringFormat("(ScriptSubsystem::ExecuteUnregisteredScript)(ScriptSubsystem is not initialized)"))

    if (script.empty())
    {
        HandleV8Error(StringFormat("Script is empty"));
        return false;
    }

    ClearError();

    m_impl->lastExecutionStart = GetCurrentTimeSeconds();

    v8::Isolate::Scope           isolateScope(m_impl->isolate);
    v8::HandleScope              handleScope(m_impl->isolate);
    v8::Local<v8::Context> const localContext = m_impl->globalContext.Get(m_impl->isolate);
    v8::Context::Scope           contextScope(localContext);

    v8::TryCatch const tryCatch(m_impl->isolate);

    // Simple script compilation WITHOUT Chrome DevTools origin information
    v8::Local<v8::String> const source = v8::String::NewFromUtf8(m_impl->isolate, script.c_str()).ToLocalChecked();

    v8::Local<v8::Script> compiledScript;
    if (!v8::Script::Compile(localContext, source).ToLocal(&compiledScript))
    {
        // Compile error
        v8::String::Utf8Value error(m_impl->isolate, tryCatch.Exception());
        HandleV8Error(StringFormat("Script compilation error: {}", String(*error)));
        return false;
    }

    // Execute script
    v8::Local<v8::Value> result;
    if (!compiledScript->Run(localContext).ToLocal(&result))
    {
        // Runtime error
        v8::String::Utf8Value error(m_impl->isolate, tryCatch.Exception());
        HandleV8Error(StringFormat("Script runtime error: {}", String(*error)));
        return false;
    }

    // Store result
    if (!result->IsUndefined())
    {
        v8::String::Utf8Value utf8Result(m_impl->isolate, result);
        m_lastResult = String(*utf8Result);
    }
    else
    {
        m_lastResult.clear();
    }

    // Update execution statistics
    double executionTime = GetCurrentTimeSeconds() - m_impl->lastExecutionStart;
    m_stats.scriptsExecuted++;
    m_stats.totalExecutionTime += static_cast<size_t>(executionTime * 1000); // Convert to milliseconds

    return true;
}

bool ScriptSubsystem::ExecuteScriptWithOrigin(String const& script, String const& scriptName)
{
    if (!m_isInitialized) ERROR_AND_DIE(StringFormat("(ScriptSubsystem::ExecuteScriptWithOrigin)(ScriptSubsystem is not initialized)"))

    if (script.empty())
    {
        HandleV8Error(StringFormat("Script is empty"));
        return false;
    }

    ClearError();

    m_impl->lastExecutionStart = GetCurrentTimeSeconds();

    v8::Isolate::Scope           isolateScope(m_impl->isolate);
    v8::HandleScope              handleScope(m_impl->isolate);
    v8::Local<v8::Context> const localContext = m_impl->globalContext.Get(m_impl->isolate);
    v8::Context::Scope           contextScope(localContext);

    v8::TryCatch const tryCatch(m_impl->isolate);

    // Compile script with origin information for Chrome DevTools
    v8::Local<v8::String> const source = v8::String::NewFromUtf8(m_impl->isolate, script.c_str()).ToLocalChecked();

    // Convert script name to DevTools-friendly URL
    std::string                 devToolsURL  = ConvertToDevToolsURL(scriptName);
    v8::Local<v8::String> const resourceName = v8::String::NewFromUtf8(m_impl->isolate, devToolsURL.c_str()).ToLocalChecked();

    // Store script source for DevTools Debugger.getScriptSource support
    StoreScriptSource(devToolsURL, script);

    // Create script origin for Chrome DevTools visibility
    v8::ScriptOrigin origin(resourceName);

    v8::Local<v8::Script> compiledScript;
    if (!v8::Script::Compile(localContext, source, &origin).ToLocal(&compiledScript))
    {
        // Compile error
        v8::String::Utf8Value error(m_impl->isolate, tryCatch.Exception());
        HandleV8Error(StringFormat("Script compilation error: {}", String(*error)));
        return false;
    }

    // Execute script
    v8::Local<v8::Value> result;
    if (!compiledScript->Run(localContext).ToLocal(&result))
    {
        // Execute error
        v8::String::Utf8Value error(m_impl->isolate, tryCatch.Exception());
        HandleV8Error(StringFormat("Script execution error: {}", String(*error)));
        return false;
    }

    // Save execute result
    if (!result->IsUndefined())
    {
        v8::String::Utf8Value resultStr(m_impl->isolate, result);
        m_lastResult = *resultStr;
    }
    else
    {
        m_lastResult.clear();
    }

    // Update ExecutionStats
    double const executionTime = GetCurrentTimeSeconds() - m_impl->lastExecutionStart;
    m_stats.scriptsExecuted++;
    m_stats.totalExecutionTime += static_cast<long long>(executionTime * 1000.0);

    // DEVTOOLS EVENTS: Generate events for script execution to populate panels
    if (m_devToolsServer && m_devToolsServer->IsRunning())
    {
        double timestamp = static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());

        // Performance timeline event for script execution
        SendPerformanceTimelineEvent("ScriptExecution", scriptName, timestamp);

        // Network event for script loading (simulate file request)
        std::string scriptURL = "file:///FirstV8/Scripts/" + scriptName;
        SendNetworkRequestEvent(scriptURL, "GET", 200);
    }

    return true;
}

std::any ScriptSubsystem::ExecuteScriptWithResult(String const& script)
{
    if (ExecuteScript(script))
    {
        return std::any(m_lastResult);
    }

    return std::any{};
}

//----------------------------------------------------------------------------------------------------
// Error handling and status methods
//----------------------------------------------------------------------------------------------------

bool ScriptSubsystem::HasError() const
{
    return m_hasError;
}

String ScriptSubsystem::GetLastError() const
{
    return m_lastError;
}

String ScriptSubsystem::GetLastResult() const
{
    return m_lastResult;
}

bool ScriptSubsystem::IsInitialized() const
{
    return m_isInitialized;
}

void ScriptSubsystem::ClearError()
{
    m_hasError = false;
    m_lastError.clear();
}

//----------------------------------------------------------------------------------------------------
// Object and function registration methods
//----------------------------------------------------------------------------------------------------

void ScriptSubsystem::RegisterScriptableObject(String const&                             name,
                                               std::shared_ptr<IScriptableObject> const& object)
{
    if (object == nullptr)
    {
        HandleV8Error(StringFormat("Script object is null: {}", name));
        return;
    }

    DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("(ScriptSubsystem::RegisterScriptableObject)({})(start)", name));

    // Handle object replacement
    if (m_scriptableObjects.contains(name))
    {
        DebuggerPrintf("ScriptSubsystem: 替換現有的腳本物件: %s\n", name.c_str());
        m_boundObjects.erase(name);  // Allow rebinding
    }

    m_scriptableObjects[name] = object;

    // Create binding for this specific object only
    if (m_isInitialized)
    {
        CreateSingleObjectBinding(name, object);  // ← Only bind this one object
    }

    DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("(ScriptSubsystem::RegisterScriptableObject)({})(end)", name));
}

void ScriptSubsystem::UnregisterScriptableObject(String const& name)
{
    auto it = m_scriptableObjects.find(name);
    if (it != m_scriptableObjects.end())
    {
        DebuggerPrintf("ScriptSubsystem: 取消註冊腳本物件: %s\n", name.c_str());
        m_scriptableObjects.erase(it);
        m_boundObjects.erase(name);  // Remove from bound tracking

        // Note: V8 global object cleanup would require more complex implementation
        // For now, the object remains in V8 global scope but is no longer updated
    }
}

void ScriptSubsystem::RegisterGlobalFunction(String const& name, ScriptFunction const& function)
{
    if (!function)
    {
        HandleV8Error(StringFormat("Attempting to register empty global function: {}", name));
        return;
    }

    DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("(ScriptSubsystem::RegisterGlobalFunction)({})(start)", name));

    // Handle function replacement
    if (m_globalFunctions.contains(name))
    {
        DebuggerPrintf("ScriptSubsystem: 替換現有的全域函式: %s\n", name.c_str());
        m_boundFunctions.erase(name);  // Allow rebinding
    }

    m_globalFunctions[name] = function;

    // Create binding for this specific function only
    if (m_isInitialized)
    {
        CreateSingleFunctionBinding(name, function);  // ← Only bind this one function
    }

    DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("(ScriptSubsystem::RegisterGlobalFunction)({})(end)", name));
}

void ScriptSubsystem::UnregisterGlobalFunction(String const& name)
{
    auto it = m_globalFunctions.find(name);
    if (it != m_globalFunctions.end())
    {
        DebuggerPrintf("ScriptSubsystem: 取消註冊全域函式: %s\n", name.c_str());
        m_globalFunctions.erase(it);
        m_boundFunctions.erase(name);  // Remove from bound tracking
    }
}

bool ScriptSubsystem::HasRegisteredObject(const String& name) const
{
    return m_scriptableObjects.contains(name);
}

bool ScriptSubsystem::HasRegisteredFunction(const String& name) const
{
    return m_globalFunctions.contains(name);
}

StringList ScriptSubsystem::GetRegisteredObjectNames() const
{
    std::vector<String> names;
    names.reserve(m_scriptableObjects.size());

    for (const auto& pair : m_scriptableObjects)
    {
        names.push_back(pair.first);
    }

    return names;
}

StringList ScriptSubsystem::GetRegisteredFunctionNames() const
{
    std::vector<String> names;
    names.reserve(m_globalFunctions.size());

    for (const auto& pair : m_globalFunctions)
    {
        names.push_back(pair.first);
    }

    return names;
}

//----------------------------------------------------------------------------------------------------
// Debug and statistics methods
//----------------------------------------------------------------------------------------------------

void ScriptSubsystem::SetDebugOutput(bool const enabled)
{
    m_config.enableConsoleOutput = enabled;
}

ScriptSubsystem::ExecutionStats ScriptSubsystem::GetExecutionStats() const
{
    return m_stats;
}

void ScriptSubsystem::ResetExecutionStats()
{
    m_stats = ExecutionStats{};
}

//----------------------------------------------------------------------------------------------------
// Memory management methods
//----------------------------------------------------------------------------------------------------

void ScriptSubsystem::ForceGarbageCollection()
{
    if (m_isInitialized && m_impl->isolate)
    {
        m_impl->isolate->RequestGarbageCollectionForTesting(v8::Isolate::kFullGarbageCollection);
        DebuggerPrintf("ScriptSubsystem: 強制執行垃圾回收\n");
    }
}

ScriptSubsystem::MemoryUsage ScriptSubsystem::GetMemoryUsage() const
{
    MemoryUsage usage;
    if (m_isInitialized && m_impl->isolate)
    {
        v8::HeapStatistics stats;
        m_impl->isolate->GetHeapStatistics(&stats);

        usage.usedHeapSize  = stats.used_heap_size();
        usage.totalHeapSize = stats.total_heap_size();
        usage.heapSizeLimit = stats.heap_size_limit();

        if (usage.heapSizeLimit > 0)
        {
            usage.usagePercentage = (double)usage.usedHeapSize / usage.heapSizeLimit * 100.0;
        }
    }

    return usage;
}

//----------------------------------------------------------------------------------------------------
// Chrome DevTools methods
//----------------------------------------------------------------------------------------------------

std::string ScriptSubsystem::HandleDebuggerGetScriptSource(const std::string& scriptId)
{
    // Find URL by script ID
    auto idIt = m_scriptIdToURL.find(scriptId);
    if (idIt == m_scriptIdToURL.end())
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Warning,
                   StringFormat("Script ID not found: {}", scriptId));
        return "";
    }

    std::string url = idIt->second;
    return GetScriptSourceByURL(url);
}

void ScriptSubsystem::ReplayScriptsToDevTools()
{
    if (!m_devToolsServer || !m_devToolsServer->IsRunning())
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Warning,
                   "Cannot replay scripts: DevTools server not running");
        return;
    }

    size_t totalScripts = m_priorityScriptNotifications.size() + m_scriptNotifications.size();
    DAEMON_LOG(LogScript, eLogVerbosity::Display,
               StringFormat("Replaying {} script notifications ({} priority, {} regular) to newly connected DevTools",
                            totalScripts, m_priorityScriptNotifications.size(), m_scriptNotifications.size()));

    // First, replay high-priority scripts (JSEngine.js, JSGame.js) to ensure they appear first
    for (const std::string& notification : m_priorityScriptNotifications)
    {
        m_devToolsServer->SendToDevTools(notification);
        DAEMON_LOG(LogScript, eLogVerbosity::Log,
                   StringFormat("Replayed PRIORITY script: {}", notification.substr(0, 100) + "..."));
    }

    // Then replay regular script notifications
    for (const std::string& notification : m_scriptNotifications)
    {
        m_devToolsServer->SendToDevTools(notification);
        DAEMON_LOG(LogScript, eLogVerbosity::Log,
                   StringFormat("Replayed script notification: {}", notification.substr(0, 100) + "..."));
    }

    DAEMON_LOG(LogScript, eLogVerbosity::Display,
               "Script notification replay completed");
}

void ScriptSubsystem::StoreScriptIdMapping(const std::string& scriptId, const std::string& url)
{
    m_scriptIdToURL[scriptId] = url;
}

void ScriptSubsystem::StoreScriptNotificationForReplay(const std::string& notification)
{
    // Check if this is a high-priority script (JSEngine.js, JSGame.js)
    bool isHighPriority = (notification.find("JSEngine.js") != std::string::npos ||
        notification.find("JSGame.js") != std::string::npos);

    if (isHighPriority)
    {
        m_priorityScriptNotifications.push_back(notification);
    }
    else
    {
        // For dynamic scripts, limit storage to prevent Chrome DevTools clutter
        if (m_scriptNotifications.size() < 50) // Reasonable limit
        {
            m_scriptNotifications.push_back(notification);
        }
    }
}

//----------------------------------------------------------------------------------------------------
// DevTools Panel Event Generation Methods (continued from V8Subsystem.cpp with same implementation)
//----------------------------------------------------------------------------------------------------

void ScriptSubsystem::SendPerformanceTimelineEvent(const std::string& eventType, const std::string& name, double timestamp)
{
    // [Same implementation as V8Subsystem.cpp:1725-1786]
    if (!m_isInitialized || !m_devToolsServer) return;

    // Create proper Profiler.consoleProfileStarted event for Performance panel
    // This is the correct Chrome DevTools Protocol event for Performance timeline
    std::string notification = std::string("{") +
        "\"method\": \"Profiler.consoleProfileStarted\"," +
        "\"params\": {" +
        "\"id\": \"" + std::to_string(static_cast<long long>(timestamp)) + "\"," +
        "\"location\": {" +
        "\"scriptId\": \"1\"," +
        "\"lineNumber\": 0" +
        "}," +
        "\"title\": \"" + eventType + ": " + name + "\"" +
        "}" +
        "}";

    // Send via DevTools server
    if (m_devToolsServer && m_devToolsServer->IsRunning())
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Display,
                   StringFormat("DEVTOOLS DEBUG: Sending Performance event: {} - {}", eventType, name));
        m_devToolsServer->SendToDevTools(notification);

        // Also send the corresponding finished event after a short delay
        std::string finishedNotification = std::string("{") +
            "\"method\": \"Profiler.consoleProfileFinished\"," +
            "\"params\": {" +
            "\"id\": \"" + std::to_string(static_cast<long long>(timestamp)) + "\"," +
            "\"location\": {" +
            "\"scriptId\": \"1\"," +
            "\"lineNumber\": 0" +
            "}," +
            "\"title\": \"" + eventType + ": " + name + "\"," +
            "\"profile\": {" +
            "\"nodes\": [" +
            "{" +
            "\"id\": 1," +
            "\"callFrame\": {" +
            "\"functionName\": \"" + name + "\"," +
            "\"scriptId\": \"1\"," +
            "\"url\": \"file:///FirstV8/Scripts/" + name + ".js\"," +
            "\"lineNumber\": 0," +
            "\"columnNumber\": 0" +
            "}," +
            "\"hitCount\": 1" +
            "}" +
            "]," +
            "\"startTime\": " + std::to_string(timestamp) + "," +
            "\"endTime\": " + std::to_string(timestamp + 10) + "," +
            "\"samples\": [1]," +
            "\"timeDeltas\": [10]" +
            "}" +
            "}" +
            "}";

        m_devToolsServer->SendToDevTools(finishedNotification);
        DAEMON_LOG(LogScript, eLogVerbosity::Display,
                   StringFormat("DEVTOOLS DEBUG: Sent Performance finished event for: {}", eventType));
    }
}

void ScriptSubsystem::SendNetworkRequestEvent(const std::string& url, const std::string& method, int statusCode)
{
    // [Same implementation as V8Subsystem.cpp:1788-1848]
    if (!m_isInitialized || !m_devToolsServer) return;

    // Create Network request event notification
    // This populates the Network panel with request data
    std::string requestId = "req_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    double      timestamp = static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());

    std::string notification = R"({
        "method": "Network.requestWillBeSent",
        "params": {
            "requestId": ")" + requestId + R"(",
            "loaderId": "loader1",
            "documentURL": "file://FirstV8",
            "request": {
                "url": ")" + url + R"(",
                "method": ")" + method + R"(",
                "headers": {
                    "User-Agent": "FirstV8/1.0"
                }
            },
            "timestamp": )" + std::to_string(timestamp) + R"(,
            "wallTime": )" + std::to_string(timestamp) + R"(,
            "initiator": {
                "type": "script"
            }
        }
    })";

    if (m_devToolsServer && m_devToolsServer->IsRunning())
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Display,
                   StringFormat("DEVTOOLS DEBUG: Sending Network request: {} {} ({})", method, url, statusCode));
        m_devToolsServer->SendToDevTools(notification);

        // Send response event
        std::string responseNotification = R"({
            "method": "Network.responseReceived",
            "params": {
                "requestId": ")" + requestId + R"(",
                "loaderId": "loader1",
                "timestamp": )" + std::to_string(timestamp + 10) + R"(,
                "type": "Script",
                "response": {
                    "url": ")" + url + R"(",
                    "status": )" + std::to_string(statusCode) + R"(,
                    "statusText": "OK",
                    "headers": {
                        "Content-Type": "application/javascript"
                    },
                    "mimeType": "application/javascript"
                }
            }
        })";

        m_devToolsServer->SendToDevTools(responseNotification);
        DAEMON_LOG(LogScript, eLogVerbosity::Display,
                   StringFormat("DEVTOOLS DEBUG: Sent Network response for: {}", url));
    }
}

void ScriptSubsystem::SendMemoryHeapSnapshot()
{
    // [Same implementation as V8Subsystem.cpp:1850-1924]
    if (!m_isInitialized || !m_devToolsServer) return;

    // Get current memory usage
    MemoryUsage usage = GetMemoryUsage();

    // STEP 1: First send HeapProfiler.takeHeapSnapshot command to initiate snapshot
    std::string takeSnapshotCommand = R"({
        "method": "HeapProfiler.takeHeapSnapshot",
        "params": {
            "reportProgress": true,
            "captureNumericValue": true
        }
    })";

    if (m_devToolsServer && m_devToolsServer->IsRunning())
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Display,
                   StringFormat("DEVTOOLS DEBUG: Sending Memory heap snapshot ({} bytes used)", usage.usedHeapSize));
        m_devToolsServer->SendToDevTools(takeSnapshotCommand);

        // STEP 2: Send reportHeapSnapshotProgress event
        std::string progressNotification = R"({
            "method": "HeapProfiler.reportHeapSnapshotProgress",
            "params": {
                "done": 100,
                "total": 100,
                "finished": true
            }
        })";

        m_devToolsServer->SendToDevTools(progressNotification);

        // STEP 3: Send the actual heap snapshot data chunk
        // Using a simplified but valid V8 heap snapshot format
        std::string snapshotData = std::string("{") +
            "\"snapshot\": {" +
            "\"meta\": {" +
            "\"node_fields\": [\"type\", \"name\", \"id\", \"self_size\", \"edge_count\", \"trace_node_id\"]," +
            "\"node_types\": [[\"hidden\", \"array\", \"string\", \"object\", \"code\", \"closure\", \"regexp\", \"number\", \"native\", \"synthetic\", \"concatenated string\", \"sliced string\"]]," +
            "\"edge_fields\": [\"type\", \"name_or_index\", \"to_node\"]," +
            "\"edge_types\": [[\"context\", \"element\", \"property\", \"internal\", \"hidden\", \"shortcut\", \"weak\"]]" +
            "}," +
            "\"node_count\": 3," +
            "\"edge_count\": 2" +
            "}," +
            "\"nodes\": [9, 0, 1, " + std::to_string(usage.usedHeapSize / 3) + ", 1, 0, 9, 1, 2, " + std::to_string(usage.usedHeapSize / 3) + ", 1, 0, 9, 2, 3, " + std::to_string(usage.usedHeapSize / 3) + ", 0, 0]," +
            "\"edges\": [1, 1, 2, 1, 2, 3]," +
            "\"strings\": [\"FirstV8\", \"JSEngine\", \"V8Context\"]" +
            "}";

        // Escape the JSON for embedding in the notification
        std::string escapedSnapshot = snapshotData;
        // Replace quotes with escaped quotes
        size_t pos = 0;
        while ((pos = escapedSnapshot.find("\"", pos)) != std::string::npos)
        {
            escapedSnapshot.replace(pos, 1, "\\\"");
            pos += 2;
        }

        std::string chunkNotification = R"({
            "method": "HeapProfiler.addHeapSnapshotChunk",
            "params": {
                "chunk": ")" + escapedSnapshot + R"("
            }
        })";

        m_devToolsServer->SendToDevTools(chunkNotification);
    }

    DAEMON_LOG(LogScript, eLogVerbosity::Log,
               StringFormat("Sent memory heap snapshot: {} bytes used", usage.usedHeapSize));
}

//----------------------------------------------------------------------------------------------------
// Private implementation methods (V8 engine)
//----------------------------------------------------------------------------------------------------

std::string ScriptSubsystem::ConvertToDevToolsURL(const std::string& scriptPath)
{
    // [Same implementation as V8Subsystem.cpp:652-681]
    // Convert relative script paths to Chrome DevTools-friendly URLs
    // Transform "Data/Scripts/JSEngine.js" → "file:///FirstV8/Scripts/JSEngine.js"

    std::string url;
    if (scriptPath.find("Data/Scripts/") == 0)
    {
        // Extract filename from Data/Scripts/ path
        std::string filename = scriptPath.substr(13); // Remove "Data/Scripts/"
        url                  = "file:///FirstV8/Scripts/" + filename;
    }
    else if (scriptPath.find("/") != std::string::npos || scriptPath.find("\\") != std::string::npos)
    {
        // Handle other paths - use the basename
        size_t      lastSlash = scriptPath.find_last_of("/\\");
        std::string filename  = (lastSlash != std::string::npos) ? scriptPath.substr(lastSlash + 1) : scriptPath;
        url                   = "file:///FirstV8/Scripts/" + filename;
    }
    else
    {
        // Simple filename
        url = "file:///FirstV8/Scripts/" + scriptPath;
    }

    DAEMON_LOG(LogScript, eLogVerbosity::Display,
               StringFormat("Script URL mapping: '{}' → '{}'", scriptPath, url));

    return url;
}

void ScriptSubsystem::StoreScriptSource(const std::string& url, const std::string& source)
{
    m_scriptSources[url] = source;
    DAEMON_LOG(LogScript, eLogVerbosity::Log,
               StringFormat("Stored script source for URL: {} ({} bytes)", url, source.length()));
}

std::string ScriptSubsystem::GetScriptSourceByURL(const std::string& url)
{
    auto it = m_scriptSources.find(url);
    if (it != m_scriptSources.end())
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Log,
                   StringFormat("Retrieved script source for URL: {} ({} bytes)", url, it->second.length()));
        return it->second;
    }

    DAEMON_LOG(LogScript, eLogVerbosity::Warning,
               StringFormat("Script source not found for URL: {}", url));
    return "";
}

void ScriptSubsystem::ForwardConsoleMessageToDevTools(const std::string& message)
{
    // [Same implementation as V8Subsystem.cpp:708-751]
    // Only forward if Chrome DevTools Inspector is enabled and connected
    if (!m_config.enableInspector || !m_impl->inspector || !m_impl->inspectorSession)
    {
        return;
    }

    // Create Chrome DevTools Runtime.consoleAPICalled notification
    // This follows the Chrome DevTools Protocol specification for console messages
    std::string notification = R"({
        "method": "Runtime.consoleAPICalled",
        "params": {
            "type": "log",
            "args": [
                {
                    "type": "string",
                    "value": ")" + message + R"("
                }
            ],
            "executionContextId": 1,
            "timestamp": )" + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()) + R"(
        }
    })";

    // Send the notification through the Inspector Channel
    if (m_impl->inspectorChannel)
    {
        // Create StringBuffer for the notification
        std::unique_ptr<v8_inspector::StringBuffer> buffer =
            v8_inspector::StringBuffer::create(v8_inspector::StringView(
                reinterpret_cast<const uint8_t*>(notification.c_str()),
                notification.length()
            ));

        // Send notification directly through the channel
        m_impl->inspectorChannel->sendNotification(std::move(buffer));
    }
    else
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Warning,
                   StringFormat("Cannot forward console message: Inspector channel not available"));
    }
}

//----------------------------------------------------------------------------------------------------
// V8 Engine lifecycle methods
//----------------------------------------------------------------------------------------------------

bool ScriptSubsystem::InitializeV8Engine() const
{
    // [Most of the implementation from V8Subsystem.cpp:1064-1254, with ScriptSubsystem adaptations]
    DAEMON_LOG(LogScript, eLogVerbosity::Display, StringFormat("(ScriptSubsystem::InitializeV8Engine)(start)"));

    // Initialize V8 platform
    v8::V8::InitializeICUDefaultLocation("");
    v8::V8::InitializeExternalStartupData("");
    m_impl->platform = v8::platform::NewDefaultPlatform();
    v8::V8::InitializePlatform(m_impl->platform.get());
    v8::V8::Initialize();

    // Create Isolate
    v8::ArrayBuffer::Allocator* allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();
    m_impl->allocator.reset(allocator);
    m_impl->createParams.array_buffer_allocator = m_impl->allocator.get();

    // Set heap size limits (MEMORY SAFETY FIX)
    if (m_config.heapSizeLimit > 0)
    {
        // Convert MB to bytes and set both old generation and young generation limits
        size_t heapSizeBytes = m_config.heapSizeLimit * 1024 * 1024;

        // Set old generation (long-lived objects) to 80% of total heap
        m_impl->createParams.constraints.set_max_old_generation_size_in_bytes(static_cast<size_t>(heapSizeBytes * 0.8));

        // Set young generation (short-lived objects) to 20% of total heap
        m_impl->createParams.constraints.set_max_young_generation_size_in_bytes(static_cast<size_t>(heapSizeBytes * 0.2));

        DAEMON_LOG(LogScript, eLogVerbosity::Display,
                   StringFormat("V8 heap limits set: Total {}MB, Old Gen {}MB, Young Gen {}MB",
                                m_config.heapSizeLimit,
                                (heapSizeBytes * 0.8) / (1024 * 1024),
                                (heapSizeBytes * 0.2) / (1024 * 1024)));
    }

    m_impl->isolate = v8::Isolate::New(m_impl->createParams);

    if (!m_impl->isolate)
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Error, StringFormat("(ScriptSubsystem::InitializeV8Engine)(failed to create V8 Isolate!)"));
        return false;
    }

    // Create Context
    v8::Isolate::Scope           isolateScope(m_impl->isolate);                     // Thread safety, make this isolate active for the current thread
    v8::HandleScope              handleScope(m_impl->isolate);                      // Memory safety, manage temporary V8 object handles automatically
    v8::Local<v8::Context> const localContext = v8::Context::New(m_impl->isolate);  // Create new JavaScript execution context
    m_impl->globalContext.Reset(m_impl->isolate, localContext);                     // Convert local context to persistent handle for long-term storage
    m_impl->isInitialized = true;

    // PHASE 3: Register dynamic import callback for import() function support
    if (m_config.enableModules)
    {
        m_impl->isolate->SetHostImportModuleDynamicallyCallback(ModuleLoader::HostImportModuleDynamicallyCallback);
        DAEMON_LOG(LogScript, eLogVerbosity::Display, StringFormat("Phase 3: Dynamic import() callback registered"));
    }

    // Initialize Chrome DevTools Inspector if enabled
    if (m_config.enableInspector)
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Display, StringFormat("Initializing Chrome DevTools Inspector on {}:{}", m_config.inspectorHost, m_config.inspectorPort));

        // Create inspector client and inspector
        m_impl->inspectorClient = std::make_unique<V8InspectorClientImpl>(const_cast<ScriptSubsystem*>(this));
        m_impl->inspector       = v8_inspector::V8Inspector::create(m_impl->isolate, m_impl->inspectorClient.get());

        // Register the JavaScript context with the inspector
        v8_inspector::StringView contextName = v8_inspector::StringView(
            reinterpret_cast<const uint8_t*>("FirstV8 JavaScript Context"), 26
        );

        v8_inspector::V8ContextInfo contextInfo(localContext, m_impl->kContextGroupId, contextName);
        contextInfo.hasMemoryOnConsole = true;
        m_impl->inspector->contextCreated(contextInfo);

        // Create inspector channel and session for Chrome DevTools communication
        m_impl->inspectorChannel = std::make_unique<V8InspectorChannelImpl>(const_cast<ScriptSubsystem*>(this), nullptr);
        m_impl->inspectorSession = m_impl->inspector->connect(
            m_impl->kContextGroupId,
            m_impl->inspectorChannel.get(),
            v8_inspector::StringView(),
            v8_inspector::V8Inspector::kFullyTrusted
        );

        // Enable essential Chrome DevTools Protocol domains for proper functionality
        if (m_impl->inspectorSession)
        {
            // Enable Runtime domain for console.log and script evaluation
            std::string              enableRuntime = "{\"id\":1,\"method\":\"Runtime.enable\"}";
            v8_inspector::StringView runtimeMessage(
                reinterpret_cast<const uint8_t*>(enableRuntime.c_str()),
                enableRuntime.length()
            );
            m_impl->inspectorSession->dispatchProtocolMessage(runtimeMessage);

            // Enable Console domain for console message handling
            std::string              enableConsole = "{\"id\":2,\"method\":\"Console.enable\"}";
            v8_inspector::StringView consoleMessage(
                reinterpret_cast<const uint8_t*>(enableConsole.c_str()),
                enableConsole.length()
            );
            m_impl->inspectorSession->dispatchProtocolMessage(consoleMessage);

            // Enable Debugger domain for source visibility and breakpoints
            std::string              enableDebugger = "{\"id\":3,\"method\":\"Debugger.enable\"}";
            v8_inspector::StringView debuggerMessage(
                reinterpret_cast<const uint8_t*>(enableDebugger.c_str()),
                enableDebugger.length()
            );
            m_impl->inspectorSession->dispatchProtocolMessage(debuggerMessage);

            // Enable HeapProfiler domain for Memory Panel visibility
            // This is ESSENTIAL for Chrome DevTools Memory Panel to detect the JavaScript VM instance
            std::string              enableHeapProfiler = "{\"id\":4,\"method\":\"HeapProfiler.enable\"}";
            v8_inspector::StringView heapProfilerMessage(
                reinterpret_cast<const uint8_t*>(enableHeapProfiler.c_str()),
                enableHeapProfiler.length()
            );
            m_impl->inspectorSession->dispatchProtocolMessage(heapProfilerMessage);

            // Enable Profiler domain for CPU profiling support
            std::string              enableProfiler = "{\"id\":5,\"method\":\"Profiler.enable\"}";
            v8_inspector::StringView profilerMessage(
                reinterpret_cast<const uint8_t*>(enableProfiler.c_str()),
                enableProfiler.length()
            );
            m_impl->inspectorSession->dispatchProtocolMessage(profilerMessage);

            // Enable Network domain for Network panel functionality
            std::string              enableNetwork = "{\"id\":6,\"method\":\"Network.enable\"}";
            v8_inspector::StringView networkMessage(
                reinterpret_cast<const uint8_t*>(enableNetwork.c_str()),
                enableNetwork.length()
            );
            m_impl->inspectorSession->dispatchProtocolMessage(networkMessage);

            // Enable Page domain for resource loading and navigation tracking
            std::string              enablePage = "{\"id\":7,\"method\":\"Page.enable\"}";
            v8_inspector::StringView pageMessage(
                reinterpret_cast<const uint8_t*>(enablePage.c_str()),
                enablePage.length()
            );
            m_impl->inspectorSession->dispatchProtocolMessage(pageMessage);

            // Enable DOM domain for DOM tree inspection
            std::string              enableDOM = "{\"id\":8,\"method\":\"DOM.enable\"}";
            v8_inspector::StringView domMessage(
                reinterpret_cast<const uint8_t*>(enableDOM.c_str()),
                enableDOM.length()
            );
            m_impl->inspectorSession->dispatchProtocolMessage(domMessage);

            DAEMON_LOG(LogScript, eLogVerbosity::Display, StringFormat("Chrome DevTools domains enabled: Runtime, Console, Debugger, HeapProfiler, Profiler, Network, Page, DOM"));

            // CRITICAL: Send Runtime.executionContextCreated event for proper initialization
            // This is essential for DevTools panels to recognize the JavaScript context
            if (m_devToolsServer && m_devToolsServer->IsRunning())
            {
                std::string contextCreatedNotification = R"({
                    "method": "Runtime.executionContextCreated",
                    "params": {
                        "context": {
                            "id": 1,
                            "origin": "file://FirstV8",
                            "name": "FirstV8 JavaScript Context",
                            "auxData": {
                                "isDefault": true,
                                "type": "default",
                                "frameId": "frame1"
                            }
                        }
                    }
                })";

                m_devToolsServer->SendToDevTools(contextCreatedNotification);

                DAEMON_LOG(LogScript, eLogVerbosity::Display,
                           StringFormat("DEVTOOLS DEBUG: Sent Runtime.executionContextCreated event to DevTools"));
            }
        }

        // If configured to wait for debugger, pause execution
        if (m_config.waitForDebugger)
        {
            DAEMON_LOG(LogScript, eLogVerbosity::Display, StringFormat("Waiting for Chrome DevTools debugger connection..."));
            v8_inspector::StringView reason = v8_inspector::StringView(
                reinterpret_cast<const uint8_t*>("Waiting for debugger"), 21
            );
            m_impl->inspectorSession->schedulePauseOnNextStatement(reason, v8_inspector::StringView());
        }

        DAEMON_LOG(LogScript, eLogVerbosity::Display, StringFormat("Chrome DevTools Inspector initialized successfully"));
        DAEMON_LOG(LogScript, eLogVerbosity::Display, StringFormat("Connect Chrome DevTools to: chrome://inspect or devtools://devtools/bundled/js_app.html?experiments=true&ws={}:{}", m_config.inspectorHost, m_config.inspectorPort));
    }

    DAEMON_LOG(LogScript, eLogVerbosity::Display, StringFormat("(ScriptSubsystem::InitializeV8Engine)(end)"));
    return true;
}

void ScriptSubsystem::ShutdownV8Engine()
{
    // [Same implementation as V8Subsystem.cpp:1257-1314 with ScriptSubsystem adaptations]
    if (!m_impl->isInitialized) return;

    DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("(ScriptSubsystem::ShutdownV8Engine)(start)"));

    // Cleanup Chrome DevTools Inspector if it was enabled
    if (m_config.enableInspector && m_impl->inspector)
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Display, StringFormat("Shutting down Chrome DevTools Inspector..."));

        // Stop inspector session
        if (m_impl->inspectorSession)
        {
            m_impl->inspectorSession->stop();
            m_impl->inspectorSession.reset();
        }

        // Notify inspector about context destruction
        if (!m_impl->globalContext.IsEmpty())
        {
            v8::Isolate::Scope     isolateScope(m_impl->isolate);
            v8::HandleScope        handleScope(m_impl->isolate);
            v8::Local<v8::Context> localContext = m_impl->globalContext.Get(m_impl->isolate);
            m_impl->inspector->contextDestroyed(localContext);
        }

        // Cleanup inspector components
        m_impl->inspectorChannel.reset();
        m_impl->inspector.reset();
        m_impl->inspectorClient.reset();

        DAEMON_LOG(LogScript, eLogVerbosity::Display, StringFormat("Chrome DevTools Inspector shutdown complete"));
    }

    // Context cleanup
    m_impl->globalContext.Reset();

    // Clean up Isolate
    if (m_impl->isolate)
    {
        m_impl->isolate->Dispose();
        m_impl->isolate = nullptr;
    }

    // Clean up Allocator
    if (m_impl->allocator != nullptr)
    {
        m_impl->allocator = nullptr;
    }

    // Clean up platform
    v8::V8::Dispose();
    v8::V8::DisposePlatform();

    m_impl->isInitialized = false;
    DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("(ScriptSubsystem::ShutdownV8Engine)(end)"));
}

void ScriptSubsystem::SetupV8Bindings()
{
    // [Same implementation as V8Subsystem.cpp:1317-1338]
    if (!m_isInitialized) ERROR_AND_DIE(StringFormat("(ScriptSubsystem::SetupV8Bindings)(ScriptSubsystem is not initialized)"))

    DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("(ScriptSubsystem::SetupV8Bindings)(start)"));

    SetupBuiltinObjects();

    // Create bindings for all currently registered objects (initial bulk binding)
    for (auto const& [name, object] : m_scriptableObjects)
    {
        CreateSingleObjectBinding(name, object);
    }

    // Create bindings for all currently registered functions (initial bulk binding)
    for (auto const& [name, function] : m_globalFunctions)
    {
        CreateSingleFunctionBinding(name, function);
    }

    DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("(ScriptSubsystem::SetupV8Bindings)(end)"));
}

//----------------------------------------------------------------------------------------------------
// V8 Property Accessor Callbacks
//----------------------------------------------------------------------------------------------------

// Property getter callback - called when JavaScript reads object.property
void PropertyGetterCallback(v8::Local<v8::Name> property, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    v8::Isolate*    isolate = info.GetIsolate();
    v8::HandleScope scope(isolate);

    // Get property callback data
    v8::Local<v8::External> external     = v8::Local<v8::External>::Cast(info.Data());
    auto*                   callbackData = static_cast<PropertyCallbackData*>(external->Value());

    v8::String::Utf8Value propertyName(isolate, property);
    std::string           propName(*propertyName);

    try
    {
        // Call C++ GetProperty method
        std::any result = callbackData->object->GetProperty(callbackData->propertyName);

        // Convert result to V8 value based on type
        if (result.type() == typeid(std::string))
        {
            std::string str = std::any_cast<std::string>(result);
            info.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, str.c_str()).ToLocalChecked());
        }
        else if (result.type() == typeid(String))
        {
            String str = std::any_cast<String>(result);
            info.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, str.c_str()).ToLocalChecked());
        }
        else if (result.type() == typeid(int))
        {
            int value = std::any_cast<int>(result);
            info.GetReturnValue().Set(v8::Integer::New(isolate, value));
        }
        else if (result.type() == typeid(double))
        {
            double value = std::any_cast<double>(result);
            info.GetReturnValue().Set(v8::Number::New(isolate, value));
        }
        else if (result.type() == typeid(bool))
        {
            bool value = std::any_cast<bool>(result);
            info.GetReturnValue().Set(v8::Boolean::New(isolate, value));
        }
        else
        {
            DAEMON_LOG(LogScript, eLogVerbosity::Warning, StringFormat("PropertyGetterCallback: Unknown type returned from GetProperty, using undefined"));
            info.GetReturnValue().Set(v8::Undefined(isolate));
        }
    }
    catch (const std::exception& e)
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Error, StringFormat("PropertyGetterCallback: Exception in GetProperty: {}", e.what()));
        info.GetReturnValue().Set(v8::Undefined(isolate));
    }
}

// Property setter callback - called when JavaScript writes object.property = value
void PropertySetterCallback(v8::Local<v8::Name> property, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
    v8::Isolate*    isolate = info.GetIsolate();
    v8::HandleScope scope(isolate);

    // Get property callback data
    v8::Local<v8::External> external     = v8::Local<v8::External>::Cast(info.Data());
    auto*                   callbackData = static_cast<PropertyCallbackData*>(external->Value());

    v8::String::Utf8Value propertyName(isolate, property);
    std::string           propName(*propertyName);

    try
    {
        std::any cppValue;

        // Convert V8 value to C++ std::any based on JavaScript type
        if (value->IsString())
        {
            v8::String::Utf8Value str(isolate, value);
            cppValue = std::string(*str);
            DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("PropertySetterCallback: Setting string value: '{}'", *str));
        }
        else if (value->IsNumber())
        {
            double num = value->NumberValue(isolate->GetCurrentContext()).ToChecked();
            // Check if it's an integer
            if (num == floor(num))
            {
                cppValue = static_cast<int>(num);
                DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("PropertySetterCallback: Setting int value: {}", static_cast<int>(num)));
            }
            else
            {
                cppValue = num;
                DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("PropertySetterCallback: Setting double value: {}", num));
            }
        }
        else if (value->IsBoolean())
        {
            bool boolVal = value->BooleanValue(isolate);
            cppValue     = boolVal;
            DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("PropertySetterCallback: Setting bool value: {}", boolVal));
        }
        else
        {
            DAEMON_LOG(LogScript, eLogVerbosity::Warning, StringFormat("PropertySetterCallback: Unsupported value type for property '{}'", propName));
            return;
        }

        // Call C++ SetProperty method
        bool success = callbackData->object->SetProperty(callbackData->propertyName, cppValue);
        if (success)
        {
            DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("PropertySetterCallback: Successfully set property '{}'", propName));
        }
        else
        {
            DAEMON_LOG(LogScript, eLogVerbosity::Warning, StringFormat("PropertySetterCallback: Failed to set property '{}'", propName));
        }
    }
    catch (const std::exception& e)
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Error, StringFormat("PropertySetterCallback: Exception in SetProperty: {}", e.what()));
    }
}

void ScriptSubsystem::CreateSingleObjectBinding(String const&                             objectName,
                                                std::shared_ptr<IScriptableObject> const& object)
{
    // [Same implementation as V8Subsystem.cpp:1341-1469 with ScriptSubsystem adaptations]
    if (!m_impl->isolate) return;

    // Check if already bound to prevent duplicates
    if (m_boundObjects.contains(objectName))
    {
        DebuggerPrintf("ScriptSubsystem: 物件 %s 已綁定，跳過重複綁定\n", objectName.c_str());
        return;
    }

    v8::Isolate::Scope           isolateScope(m_impl->isolate);
    v8::HandleScope              handleScope(m_impl->isolate);
    v8::Local<v8::Context> const localContext = m_impl->globalContext.Get(m_impl->isolate);
    v8::Context::Scope           contextScope(localContext);
    v8::Local<v8::Object>        global = localContext->Global();

    DebuggerPrintf("ScriptSubsystem: 創建 V8 綁定 - 物件: %s\n", objectName.c_str());

    // Create JavaScript object for this specific object only
    v8::Local<v8::Object> jsObject = v8::Object::New(m_impl->isolate);

    // Get object's available methods
    auto methods = object->GetAvailableMethods();

    for (const auto& method : methods)
    {
        DebuggerPrintf("ScriptSubsystem: 綁定方法 %s.%s\n", objectName.c_str(), method.name.c_str());

        // Create V8 function callback for each method (same as original implementation)
        auto methodCallback = [](const v8::FunctionCallbackInfo<v8::Value>& args)
        {
            v8::Isolate*    isolate = args.GetIsolate();
            v8::HandleScope scope(isolate);

            // Get object and method name from function's internal field
            v8::Local<v8::External> objectExternal = v8::Local<v8::External>::Cast(args.Data());
            auto*                   callbackData   = static_cast<MethodCallbackData*>(objectExternal->Value());

            // Convert arguments
            std::vector<std::any> cppArgs;
            for (int i = 0; i < args.Length(); i++)
            {
                v8::Local<v8::Value> arg = args[i];
                if (arg->IsNumber())
                {
                    double num = arg->NumberValue(isolate->GetCurrentContext()).ToChecked();
                    cppArgs.push_back(num);
                }
                else if (arg->IsString())
                {
                    v8::String::Utf8Value str(isolate, arg);
                    cppArgs.push_back(String(*str));
                }
                else if (arg->IsBoolean())
                {
                    cppArgs.push_back(arg->BooleanValue(isolate));
                }
                else if (arg->IsFunction())
                {
                    // Phase 2: Store v8::Function for callback registration
                    v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(arg);
                    cppArgs.push_back(func);
                }
                else if (arg->IsArray())
                {
                    v8::Local<v8::Array>  array  = v8::Local<v8::Array>::Cast(arg);
                    uint32_t              length = array->Length();
                    std::vector<std::any> arrayElements;
                    arrayElements.reserve(length);

                    for (uint32_t j = 0; j < length; j++)
                    {
                        v8::Local<v8::Value> element = array->Get(isolate->GetCurrentContext(), j).ToLocalChecked();
                        if (element->IsNumber())
                        {
                            arrayElements.push_back(element->NumberValue(isolate->GetCurrentContext()).ToChecked());
                        }
                        else if (element->IsString())
                        {
                            v8::String::Utf8Value str(isolate, element);
                            arrayElements.push_back(String(*str));
                        }
                        else if (element->IsBoolean())
                        {
                            arrayElements.push_back(element->BooleanValue(isolate));
                        }
                    }
                    cppArgs.push_back(arrayElements);
                }
            }

            // Call C++ method
            ScriptMethodResult result = callbackData->object->CallMethod(callbackData->methodName, cppArgs);

            if (result.success)
            {
                // Convert return value
                try
                {
                    if (result.result.type() == typeid(String))
                    {
                        String str = std::any_cast<String>(result.result);

                        // Try to parse as JSON first - if it looks like JSON and parses successfully,
                        // return a JavaScript object instead of a string
                        if (!str.empty() && (str[0] == '{' || str[0] == '['))
                        {
                            v8::Local<v8::Context> context = isolate->GetCurrentContext();
                            v8::TryCatch tryCatch(isolate);

                            v8::MaybeLocal<v8::Value> jsonResult = v8::JSON::Parse(
                                context,
                                v8::String::NewFromUtf8(isolate, str.c_str()).ToLocalChecked()
                            );

                            if (!jsonResult.IsEmpty())
                            {
                                // Successfully parsed as JSON - return the object
                                args.GetReturnValue().Set(jsonResult.ToLocalChecked());
                            }
                            else
                            {
                                // JSON parsing failed - return as string
                                args.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, str.c_str()).ToLocalChecked());
                            }
                        }
                        else
                        {
                            // Not JSON - return as string
                            args.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, str.c_str()).ToLocalChecked());
                        }
                    }
                    else if (result.result.type() == typeid(bool))
                    {
                        bool value = std::any_cast<bool>(result.result);
                        args.GetReturnValue().Set(v8::Boolean::New(isolate, value));
                    }
                    else if (result.result.type() == typeid(double) || result.result.type() == typeid(float))
                    {
                        double value = std::any_cast<double>(result.result);
                        args.GetReturnValue().Set(v8::Number::New(isolate, value));
                    }
                    else
                    {
                        args.GetReturnValue().Set(v8::Undefined(isolate));
                    }
                }
                catch (const std::bad_any_cast&)
                {
                    args.GetReturnValue().Set(v8::Undefined(isolate));
                }
            }
            else
            {
                // Throw JavaScript error
                isolate->ThrowException(v8::String::NewFromUtf8(isolate, result.errorMessage.c_str()).ToLocalChecked());
            }
        };

        // Create callback data
        auto callbackData        = std::make_unique<MethodCallbackData>();
        callbackData->object     = object;
        callbackData->methodName = method.name;

        v8::Local<v8::External> external = v8::External::New(m_impl->isolate, callbackData.get());

        // Create function directly
        v8::Local<v8::Function> methodFunction = v8::Function::New(localContext, methodCallback, external).ToLocalChecked();

        // Add method to JavaScript object
        jsObject->Set(localContext,
                      v8::String::NewFromUtf8(m_impl->isolate, method.name.c_str()).ToLocalChecked(),
                      methodFunction).Check();

        // Store callback data to prevent destruction
        m_methodCallbacks.push_back(std::move(callbackData));
    }

    // Register V8 property accessors for each property in GetAvailableProperties()
    DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("CreateSingleObjectBinding: Build verification"));
    auto properties = object->GetAvailableProperties();
    DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("CreateSingleObjectBinding: Object '{}' has {} properties", objectName.c_str(), properties.size()));

    for (const auto& propertyName : properties)
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("CreateSingleObjectBinding: Registering property '{}.{}'", objectName.c_str(), propertyName.c_str()));

        // Create property callback data
        auto propertyCallbackData          = std::make_unique<PropertyCallbackData>();
        propertyCallbackData->object       = object;
        propertyCallbackData->propertyName = propertyName;

        v8::Local<v8::External> propertyExternal = v8::External::New(m_impl->isolate, propertyCallbackData.get());

        // Register property accessor with V8
        jsObject->SetNativeDataProperty(
            localContext,
            v8::String::NewFromUtf8(m_impl->isolate, propertyName.c_str()).ToLocalChecked(),
            PropertyGetterCallback,     // Called when JavaScript reads object.property
            PropertySetterCallback,     // Called when JavaScript writes object.property = value
            propertyExternal           // Property callback data
        ).Check();

        // Store property callback data to prevent destruction
        m_propertyCallbacks.push_back(std::move(propertyCallbackData));

        DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("CreateSingleObjectBinding: Successfully registered property accessor for '{}.{}'", objectName.c_str(), propertyName.c_str()));
    }

    // Bind object to global scope
    global->Set(localContext,
                v8::String::NewFromUtf8(m_impl->isolate, objectName.c_str()).ToLocalChecked(),
                jsObject).Check();

    // Mark as bound
    m_boundObjects.insert(objectName);

    DebuggerPrintf("ScriptSubsystem: 物件 %s 已綁定到 JavaScript 全域範圍\n", objectName.c_str());
}

void ScriptSubsystem::CreateSingleFunctionBinding(const String&         functionName,
                                                  const ScriptFunction& function)
{
    // [Same implementation as V8Subsystem.cpp:1472-1559 with ScriptSubsystem adaptations]
    if (!m_impl->isolate) return;

    // Check if already bound to prevent duplicates
    if (m_boundFunctions.contains(functionName))
    {
        DebuggerPrintf("ScriptSubsystem: 函式 %s 已綁定，跳過重複綁定\n", functionName.c_str());
        return;
    }

    v8::Isolate::Scope           isolateScope(m_impl->isolate);
    v8::HandleScope              handleScope(m_impl->isolate);
    v8::Local<v8::Context> const localContext = m_impl->globalContext.Get(m_impl->isolate);
    v8::Context::Scope           contextScope(localContext);
    v8::Local<v8::Object>        global = localContext->Global();

    DebuggerPrintf("ScriptSubsystem: 綁定全域函式: %s\n", functionName.c_str());

    // Create function callback (same as original implementation)
    auto functionCallback = [](const v8::FunctionCallbackInfo<v8::Value>& args)
    {
        v8::Isolate*    isolate = args.GetIsolate();
        v8::HandleScope scope(isolate);

        // Get C++ function from callback data
        v8::Local<v8::External> external = v8::Local<v8::External>::Cast(args.Data());
        auto*                   function = static_cast<ScriptFunction*>(external->Value());

        // Convert arguments
        std::vector<std::any> cppArgs;
        for (int i = 0; i < args.Length(); i++)
        {
            v8::Local<v8::Value> arg = args[i];
            if (arg->IsNumber())
            {
                double num = arg->NumberValue(isolate->GetCurrentContext()).ToChecked();
                cppArgs.push_back(num);
            }
            else if (arg->IsString())
            {
                v8::String::Utf8Value str(isolate, arg);
                cppArgs.push_back(String(*str));
            }
            else if (arg->IsBoolean())
            {
                cppArgs.push_back(arg->BooleanValue(isolate));
            }
        }

        // Call C++ function
        try
        {
            std::any result = (*function)(cppArgs);

            // Convert return value (if any)
            if (result.type() == typeid(String))
            {
                String str = std::any_cast<String>(result);
                args.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, str.c_str()).ToLocalChecked());
            }
            else
            {
                args.GetReturnValue().Set(v8::Undefined(isolate));
            }
        }
        catch (const std::exception& e)
        {
            isolate->ThrowException(v8::String::NewFromUtf8(isolate, e.what()).ToLocalChecked());
        }
    };

    // Create external data to store function pointer
    auto                    functionPtr = std::make_unique<ScriptFunction>(function);
    v8::Local<v8::External> external    = v8::External::New(m_impl->isolate, functionPtr.get());

    // Create function directly
    v8::Local<v8::Function> jsFunction = v8::Function::New(localContext, functionCallback, external).ToLocalChecked();

    // Bind function to global scope
    global->Set(localContext,
                v8::String::NewFromUtf8(m_impl->isolate, functionName.c_str()).ToLocalChecked(),
                jsFunction).Check();

    // Store function pointer to prevent destruction and mark as bound
    m_functionCallbacks.push_back(std::move(functionPtr));
    m_boundFunctions.insert(functionName);
}

void ScriptSubsystem::SetupBuiltinObjects()
{
    // [Same implementation as V8Subsystem.cpp:1563-1658 with ScriptSubsystem adaptations]
    DAEMON_LOG(LogScript, eLogVerbosity::Display, StringFormat("(ScriptSubsystem::SetupBuiltinObjects)(start)"));

    if (m_impl->isolate == nullptr) ERROR_AND_DIE(StringFormat("(ScriptSubsystem::SetupBuiltinObjects)(v8::Isolate* is nullptr)"));

    v8::Isolate::Scope           isolateScope(m_impl->isolate);
    v8::HandleScope              handleScope(m_impl->isolate);
    v8::Local<v8::Context> const localContext = m_impl->globalContext.Get(m_impl->isolate);
    v8::Context::Scope           contextScope(localContext);

    if (m_config.enableConsoleOutput)
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Display, StringFormat("(ScriptSubsystem::SetupBuiltinObjects)(start)(enableConsoleOutput)"));

        // create console object
        v8::Local<v8::Object> const console = v8::Object::New(m_impl->isolate);

        // create console.log callback with Chrome DevTools integration
        static auto consoleLogCallback = [](v8::FunctionCallbackInfo<v8::Value> const& args)
        {
            v8::Isolate*           isolate = args.GetIsolate();
            v8::HandleScope        scope(isolate);
            v8::Local<v8::Context> context = isolate->GetCurrentContext();

            // Get ScriptSubsystem instance from the data parameter
            v8::Local<v8::External> external        = v8::Local<v8::External>::Cast(args.Data());
            ScriptSubsystem*        scriptSubsystem = static_cast<ScriptSubsystem*>(external->Value());

            String output         = "(CONSOLE): ";
            String consoleMessage = ""; // Message for Chrome DevTools (without prefix)

            for (int i = 0; i < args.Length(); i++)
            {
                if (i > 0)
                {
                    output += " ";
                    consoleMessage += " ";
                }

                v8::Local<v8::Value> const arg = args[i];
                String                     argString;

                if (arg->IsString())
                {
                    v8::String::Utf8Value str(isolate, arg);
                    argString = *str;
                }
                else if (arg->IsNumber())
                {
                    double const num = arg->NumberValue(context).ToChecked();
                    argString        = std::to_string(num);
                }
                else if (arg->IsBoolean())
                {
                    bool const val = arg->BooleanValue(isolate);
                    argString      = val ? "true" : "false";
                }
                else
                {
                    argString = "[object]";
                }

                output += argString;
                consoleMessage += argString;
            }

            // TODO: FIX!!!
            // Log to C++ logging system
            //DAEMON_LOG(LogScript, eLogVerbosity::Display, StringFormat("{}", output));

            // Forward to Chrome DevTools Console if Inspector is enabled
            if (scriptSubsystem->m_impl->inspector && scriptSubsystem->m_impl->inspectorSession)
            {
                scriptSubsystem->ForwardConsoleMessageToDevTools(consoleMessage);
            }
        };

        // Create external wrapper for 'this' pointer
        v8::Local<v8::External> external = v8::External::New(m_impl->isolate, this);

        // Create function directly
        v8::Local<v8::Function> const logFunction = v8::Function::New(localContext, consoleLogCallback, external).ToLocalChecked();
        console->Set(localContext,
                     v8::String::NewFromUtf8(m_impl->isolate, "log").ToLocalChecked(),
                     logFunction).Check();

        // Bind console object to global scope
        v8::Local<v8::Object> const global = localContext->Global();
        global->Set(localContext,
                    v8::String::NewFromUtf8(m_impl->isolate, "console").ToLocalChecked(),
                    console).Check();
    }
    else
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Display, StringFormat("(ScriptSubsystem::SetupBuiltinObjects)(enableConsoleOutput is false, skip...)"));
    }
}

void ScriptSubsystem::HandleV8Error(String const& error)
{
    m_hasError  = true;
    m_lastError = error;
    m_stats.errorsEncountered++;

    DAEMON_LOG(LogScript, eLogVerbosity::Error, StringFormat("(ScriptSubsystem::HandleV8Error)({})", error));
}

void* ScriptSubsystem::ConvertToV8Value(const std::any& value)
{
    // Implementation for C++ std::any to V8 value conversion
    // This is a complex function that needs to handle various types
    UNUSED(value)
    return nullptr; // Placeholder
}

std::any ScriptSubsystem::ConvertFromV8Value(void* v8Value)
{
    // Implementation for V8 value to C++ std::any conversion
    UNUSED(v8Value)
    return std::any{}; // Placeholder
}

String ScriptSubsystem::ValidateScriptPath(const String& filename) const
{
    // [Same implementation as V8Subsystem.cpp:1688-1719]
    String fullPath;

    // Check if it's already a full path
    if (filename.find(':') != String::npos || filename[0] == '/' || filename[0] == '\\')
    {
        fullPath = filename;
    }
    else
    {
        // Check if the filename already contains the path prefix
        if (filename.find(m_config.scriptPath) == 0)
        {
            // Already contains path prefix, use directly
            fullPath = filename;
        }
        else
        {
            // No path prefix, add it
            fullPath = m_config.scriptPath + filename;
        }
    }

    // Ensure extension is .js
    if (fullPath.length() < 3 || fullPath.substr(fullPath.length() - 3) != ".js")
    {
        fullPath += ".js";
    }

    return fullPath;
}

//----------------------------------------------------------------------------------------------------
// ES6 Module System Implementation
//----------------------------------------------------------------------------------------------------

bool ScriptSubsystem::ExecuteModule(String const& modulePath)
{
    if (!m_isInitialized)
    {
        HandleV8Error("Cannot execute module: V8 not initialized");
        return false;
    }

    if (!AreModulesEnabled())
    {
        HandleV8Error("Cannot execute module: ES6 modules not enabled");
        return false;
    }

    DAEMON_LOG(LogScript, eLogVerbosity::Log,
               StringFormat("ScriptSubsystem: Executing ES6 module: {}", modulePath));

    // Delegate to ModuleLoader
    bool success = m_moduleLoader->LoadModule(modulePath);

    if (!success)
    {
        std::string error = m_moduleLoader->GetLastError();
        HandleV8Error(StringFormat("Module execution failed: {}", error));
    }

    return success;
}

//----------------------------------------------------------------------------------------------------
// V8 Internal Access Implementation
//----------------------------------------------------------------------------------------------------

void* ScriptSubsystem::GetV8Isolate()
{
    if (!m_impl || !m_impl->isolate)
    {
        return nullptr;
    }
    return m_impl->isolate;
}

//----------------------------------------------------------------------------------------------------
void* ScriptSubsystem::GetV8Context()
{
    if (!m_impl || m_impl->globalContext.IsEmpty())
    {
        return nullptr;
    }

    // Return Local<Context> by creating from Global
    v8::Isolate* isolate = m_impl->isolate;
    if (!isolate)
    {
        return nullptr;
    }

    // CRITICAL: Must be in Isolate scope to safely call Get() on Persistent<Context>
    v8::Isolate::Scope isolateScope(isolate);

    // Create a persistent pointer to the Local<Context>
    // Note: This is a workaround - we'll need to handle this carefully
    static thread_local v8::Local<v8::Context> contextCache;
    contextCache = m_impl->globalContext.Get(isolate);
    return &contextCache;
}

//----------------------------------------------------------------------------------------------------
v8::Isolate* ScriptSubsystem::GetIsolate()
{
    return static_cast<v8::Isolate*>(GetV8Isolate());
}

//----------------------------------------------------------------------------------------------------
bool ScriptSubsystem::ExecuteModuleFromSource(String const& moduleCode, String const& moduleName)
{
    if (!m_isInitialized)
    {
        HandleV8Error("Cannot execute module: V8 not initialized");
        return false;
    }

    if (!AreModulesEnabled())
    {
        HandleV8Error("Cannot execute module: ES6 modules not enabled");
        return false;
    }

    DAEMON_LOG(LogScript, eLogVerbosity::Log,
               StringFormat("ScriptSubsystem: Executing ES6 module from source: {}", moduleName));

    // Delegate to ModuleLoader
    bool success = m_moduleLoader->LoadModuleFromSource(moduleCode, moduleName);

    if (!success)
    {
        std::string error = m_moduleLoader->GetLastError();
        HandleV8Error(StringFormat("Module execution failed: {}", error));
    }

    return success;
}
#endif // !defined( ENGINE_DISABLE_AUDIO )
