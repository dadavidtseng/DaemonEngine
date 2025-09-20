//----------------------------------------------------------------------------------------------------
// V8Subsystem.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Scripting/V8Subsystem.hpp"

#include <chrono>
#include <fstream>
#include <sstream>

#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/LogSubsystem.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/Time.hpp"

//----------------------------------------------------------------------------------------------------
// Any changes that you made to the warning state between push and pop are undone.
//----------------------------------------------------------------------------------------------------
#pragma warning(push)           // stores the current warning state for every warning

#pragma warning(disable: 4100)  // 'identifier' : unreferenced formal parameter
#pragma warning(disable: 4127)  // conditional expression is constant
#pragma warning(disable: 4324)  // 'structname': structure was padded due to alignment specifier

#include "v8.h"
#include "v8-inspector.h"
#include "libplatform/libplatform.h"

// Chrome DevTools Server
#include "Engine/Scripting/ChromeDevToolsServer.hpp"

#pragma warning(pop)            // pops the last warning state pushed onto the stack
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
// Chrome DevTools Inspector Implementation
//----------------------------------------------------------------------------------------------------

// Channel implementation for Chrome DevTools communication
class V8InspectorChannelImpl : public v8_inspector::V8Inspector::Channel
{
public:
    explicit V8InspectorChannelImpl(V8Subsystem* v8Subsystem, ChromeDevToolsServer* devToolsServer)
        : m_v8Subsystem(v8Subsystem), m_devToolsServer(devToolsServer)
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
    void SetDevToolsServer(ChromeDevToolsServer* devToolsServer)
    {
        m_devToolsServer = devToolsServer;
    }

    // Store script parsed notifications for replay
    void StoreScriptNotification(const std::string& notification)
    {
        // Extract script ID and URL from the notification for script ID mapping
        std::string scriptId = ExtractJsonString(notification, "scriptId");
        std::string url      = ExtractJsonString(notification, "url");

        if (!scriptId.empty() && !url.empty() && m_v8Subsystem)
        {
            // Store the script ID to URL mapping for Debugger.getScriptSource
            m_v8Subsystem->StoreScriptIdMapping(scriptId, url);
        }

        // Store the complete notification for replay
        m_v8Subsystem->StoreScriptNotificationForReplay(notification);
    }

private:
    V8Subsystem*          m_v8Subsystem;
    ChromeDevToolsServer* m_devToolsServer;

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
    explicit V8InspectorClientImpl(V8Subsystem* v8Subsystem)
    {
        UNUSED(v8Subsystem); // Suppress unreferenced parameter warning
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
struct V8Subsystem::V8Implementation
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
    static constexpr int                              kContextGroupId = 1;        // Context group ID for debugging
};

//----------------------------------------------------------------------------------------------------
V8Subsystem::V8Subsystem(sV8SubsystemConfig config)
    : m_impl(std::make_unique<V8Implementation>()),
      m_config(std::move(config))
{
}

//----------------------------------------------------------------------------------------------------
V8Subsystem::~V8Subsystem() = default;

//----------------------------------------------------------------------------------------------------
void V8Subsystem::Startup()
{
    DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("(V8Subsystem::Startup)(start)"));

    if (m_isInitialized)
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Warning, StringFormat("(V8Subsystem::Startup)(V8Subsystem has already initialized, skip...)"));
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
        sChromeDevToolsConfig devToolsConfig;
        devToolsConfig.enabled     = true;
        devToolsConfig.host        = m_config.inspectorHost;
        devToolsConfig.port        = m_config.inspectorPort;
        devToolsConfig.contextName = "FirstV8 JavaScript Context";

        m_devToolsServer = std::make_unique<ChromeDevToolsServer>(devToolsConfig, this);

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
                static_cast<V8InspectorChannelImpl*>(m_impl->inspectorChannel.get())->SetDevToolsServer(m_devToolsServer.get());
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

    DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("(V8Subsystem::Startup)(end)"));
}

//----------------------------------------------------------------------------------------------------
void V8Subsystem::Shutdown()
{
    DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("(V8Subsystem::Shutdown)(start)"));

    if (!m_isInitialized)
    {
        return;
    }

    // Shutdown Chrome DevTools server first
    if (m_devToolsServer)
    {
        m_devToolsServer->Stop();
        m_devToolsServer.reset();
    }

    // 清理回呼資料
    m_methodCallbacks.clear();
    m_functionCallbacks.clear();

    // 清理註冊的物件和函式
    m_scriptableObjects.clear();
    m_globalFunctions.clear();

    // Clear tracking sets
    m_boundObjects.clear();
    m_boundFunctions.clear();

    ShutdownV8Engine();

    m_isInitialized = false;

    DAEMON_LOG(LogScript, eLogVerbosity::Display, StringFormat("(V8Subsystem::Shutdown)(end)"));
}

//----------------------------------------------------------------------------------------------------
void V8Subsystem::Update()
{
    if (!m_isInitialized)
    {
        return;
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
            SendNetworkRequestEvent("file:///FirstV8/Scripts/JSEngine.js", "GET", 200);
        }

        // Generate Memory heap snapshots every 300 frames (~5 seconds)  
        if (updateCounter % 300 == 0)
        {
            DAEMON_LOG(LogScript, eLogVerbosity::Display, 
                       StringFormat("DEVTOOLS DEBUG: Triggering Memory snapshot (frame {})", updateCounter));
            SendMemoryHeapSnapshot();
        }
    }

    // 這裡可以添加定期的 V8 維護工作
    // 例如：垃圾回收、統計更新等
}

//----------------------------------------------------------------------------------------------------
bool V8Subsystem::ExecuteScript(String const& script)
{
    // SCRIPT REGISTRY APPROACH: Use unregistered execution for performance
    // This prevents Chrome DevTools overhead for high-frequency script calls
    // Use ExecuteRegisteredScript() explicitly for scripts that need debugging

    return ExecuteUnregisteredScript(script);
}

//----------------------------------------------------------------------------------------------------
bool V8Subsystem::ExecuteScriptFile(String const& scriptFilename)
{
    if (!m_isInitialized) ERROR_AND_DIE(StringFormat("(V8Subsystem::ExecuteScriptFile)(V8Subsystem is not initialized)"))

    DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("(V8Subsystem::ExecuteScriptFile)(start)({})", scriptFilename));

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

    DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("(V8Subsystem::ExecuteScriptFile)(end)({})", scriptFilename));

    // SCRIPT REGISTRY: ExecuteScriptFile always registers scripts for Chrome DevTools debugging
    // This ensures JSEngine.js, JSGame.js, and other script files are visible in DevTools Sources panel
    m_registeredScripts.insert(scriptFilename);
    m_scriptRegistry[scriptFilename] = scriptContent;

    DAEMON_LOG(LogScript, eLogVerbosity::Display,
               StringFormat("SCRIPT REGISTRY: Registered file '{}' for Chrome DevTools debugging", scriptFilename));

    return ExecuteScriptWithOrigin(scriptContent, scriptFilename);
}

//----------------------------------------------------------------------------------------------------
bool V8Subsystem::ExecuteRegisteredScript(String const& script, String const& scriptName)
{
    // Execute script with full Chrome DevTools integration for debugging
    // This method is used for important scripts that should be visible in DevTools Sources panel

    if (!m_isInitialized) ERROR_AND_DIE(StringFormat("(V8Subsystem::ExecuteRegisteredScript)(V8Subsystem is not initialized)"))

    // Register this script for Chrome DevTools debugging
    m_registeredScripts.insert(scriptName);
    m_scriptRegistry[scriptName] = script;

    DAEMON_LOG(LogScript, eLogVerbosity::Display,
               StringFormat("SCRIPT REGISTRY: Registered '{}' for Chrome DevTools debugging", scriptName));

    // Execute with full Chrome DevTools integration
    return ExecuteScriptWithOrigin(script, scriptName);
}

//----------------------------------------------------------------------------------------------------
bool V8Subsystem::ExecuteUnregisteredScript(String const& script)
{
    // High-performance script execution without Chrome DevTools registration
    // Used for high-frequency calls to prevent performance overhead

    if (!m_isInitialized) ERROR_AND_DIE(StringFormat("(V8Subsystem::ExecuteUnregisteredScript)(V8Subsystem is not initialized)"))

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

//----------------------------------------------------------------------------------------------------
bool V8Subsystem::ExecuteScriptWithOrigin(String const& script, String const& scriptName)
{
    if (!m_isInitialized) ERROR_AND_DIE(StringFormat("(V8Subsystem::ExecuteScriptWithOrigin)(V8Subsystem is not initialized)"))

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

//----------------------------------------------------------------------------------------------------
// Chrome DevTools Script Management Functions
//----------------------------------------------------------------------------------------------------

std::string V8Subsystem::ConvertToDevToolsURL(const std::string& scriptPath)
{
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

//----------------------------------------------------------------------------------------------------
void V8Subsystem::StoreScriptSource(const std::string& url, const std::string& source)
{
    m_scriptSources[url] = source;
    DAEMON_LOG(LogScript, eLogVerbosity::Log,
               StringFormat("Stored script source for URL: {} ({} bytes)", url, source.length()));
}

//----------------------------------------------------------------------------------------------------
std::string V8Subsystem::GetScriptSourceByURL(const std::string& url)
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

//----------------------------------------------------------------------------------------------------
void V8Subsystem::ForwardConsoleMessageToDevTools(const std::string& message)
{
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
// Chrome DevTools Integration Support Functions
//----------------------------------------------------------------------------------------------------

std::string V8Subsystem::HandleDebuggerGetScriptSource(const std::string& scriptId)
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

//----------------------------------------------------------------------------------------------------
void V8Subsystem::ReplayScriptsToDevTools()
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

//----------------------------------------------------------------------------------------------------
void V8Subsystem::StoreScriptIdMapping(const std::string& scriptId, const std::string& url)
{
    m_scriptIdToURL[scriptId] = url;
}

//----------------------------------------------------------------------------------------------------
void V8Subsystem::StoreScriptNotificationForReplay(const std::string& notification)
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
std::any V8Subsystem::ExecuteScriptWithResult(String const& script)
{
    if (ExecuteScript(script))
    {
        return std::any(m_lastResult);
    }

    return std::any{};
}

//----------------------------------------------------------------------------------------------------
bool V8Subsystem::HasError() const
{
    return m_hasError;
}

//----------------------------------------------------------------------------------------------------
String V8Subsystem::GetLastError() const
{
    return m_lastError;
}

//----------------------------------------------------------------------------------------------------
String V8Subsystem::GetLastResult() const
{
    return m_lastResult;
}

//----------------------------------------------------------------------------------------------------
bool V8Subsystem::IsInitialized() const
{
    return m_isInitialized;
}

//----------------------------------------------------------------------------------------------------
void V8Subsystem::ClearError()
{
    m_hasError = false;
    m_lastError.clear();
}

//----------------------------------------------------------------------------------------------------
/// @brief 註冊可腳本化的物件
/// @param name 在 JavaScript 中的物件名稱
/// @param object 實作 IScriptableObject 介面的物件
void V8Subsystem::RegisterScriptableObject(String const&                             name,
                                           std::shared_ptr<IScriptableObject> const& object)
{
    if (object == nullptr)
    {
        HandleV8Error(StringFormat("Script object is null: {}", name));
        return;
    }

    DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("(V8Subsystem::RegisterScriptableObject)({})(start)", name));

    // Handle object replacement
    if (m_scriptableObjects.contains(name))
    {
        DebuggerPrintf("V8Subsystem: 替換現有的腳本物件: %s\n", name.c_str());
        m_boundObjects.erase(name);  // Allow rebinding
    }

    m_scriptableObjects[name] = object;

    // Create binding for this specific object only
    if (m_isInitialized)
    {
        CreateSingleObjectBinding(name, object);  // ← Only bind this one object
    }

    DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("(V8Subsystem::RegisterScriptableObject)({})(end)", name));
}

// 取消註冊腳本化物件
void V8Subsystem::UnregisterScriptableObject(String const& name)
{
    auto it = m_scriptableObjects.find(name);
    if (it != m_scriptableObjects.end())
    {
        DebuggerPrintf("V8Subsystem: 取消註冊腳本物件: %s\n", name.c_str());
        m_scriptableObjects.erase(it);
        m_boundObjects.erase(name);  // Remove from bound tracking

        // Note: V8 global object cleanup would require more complex implementation
        // For now, the object remains in V8 global scope but is no longer updated
    }
}

//----------------------------------------------------------------------------------------------------
/// @brief 註冊全域 JavaScript 函式
/// @param name 在 JavaScript 中的函式名稱
/// @param function C++ 函式實作
void V8Subsystem::RegisterGlobalFunction(String const& name, ScriptFunction const& function)
{
    if (!function)
    {
        HandleV8Error(StringFormat("Attempting to register empty global function: {}", name));
        return;
    }

    DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("(V8Subsystem::RegisterGlobalFunction)({})(start)", name));

    // Handle function replacement
    if (m_globalFunctions.contains(name))
    {
        DebuggerPrintf("V8Subsystem: 替換現有的全域函式: %s\n", name.c_str());
        m_boundFunctions.erase(name);  // Allow rebinding
    }

    m_globalFunctions[name] = function;

    // Create binding for this specific function only
    if (m_isInitialized)
    {
        CreateSingleFunctionBinding(name, function);  // ← Only bind this one function
    }

    DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("(V8Subsystem::RegisterGlobalFunction)({})(end)", name));
}

//----------------------------------------------------------------------------------------------------
// 取消註冊全域函式
void V8Subsystem::UnregisterGlobalFunction(String const& name)
{
    auto it = m_globalFunctions.find(name);
    if (it != m_globalFunctions.end())
    {
        DebuggerPrintf("V8Subsystem: 取消註冊全域函式: %s\n", name.c_str());
        m_globalFunctions.erase(it);
        m_boundFunctions.erase(name);  // Remove from bound tracking
    }
}

//----------------------------------------------------------------------------------------------------
bool V8Subsystem::HasRegisteredObject(const String& name) const
{
    return m_scriptableObjects.contains(name);
}

//----------------------------------------------------------------------------------------------------
bool V8Subsystem::HasRegisteredFunction(const String& name) const
{
    return m_globalFunctions.contains(name);
}

//----------------------------------------------------------------------------------------------------
StringList V8Subsystem::GetRegisteredObjectNames() const
{
    std::vector<String> names;
    names.reserve(m_scriptableObjects.size());

    for (const auto& pair : m_scriptableObjects)
    {
        names.push_back(pair.first);
    }

    return names;
}

//----------------------------------------------------------------------------------------------------
StringList V8Subsystem::GetRegisteredFunctionNames() const
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
void V8Subsystem::SetDebugOutput(bool const enabled)
{
    m_config.enableConsoleOutput = enabled;
}

//----------------------------------------------------------------------------------------------------
V8Subsystem::ExecutionStats V8Subsystem::GetExecutionStats() const
{
    return m_stats;
}

//----------------------------------------------------------------------------------------------------
void V8Subsystem::ResetExecutionStats()
{
    m_stats = ExecutionStats{};
}

//----------------------------------------------------------------------------------------------------
void V8Subsystem::ForceGarbageCollection()
{
    if (m_isInitialized && m_impl->isolate)
    {
        m_impl->isolate->RequestGarbageCollectionForTesting(v8::Isolate::kFullGarbageCollection);
        DebuggerPrintf("V8Subsystem: 強制執行垃圾回收\n");
    }
}

//----------------------------------------------------------------------------------------------------
V8Subsystem::MemoryUsage V8Subsystem::GetMemoryUsage() const
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
// 私有方法實作
//----------------------------------------------------------------------------------------------------

bool V8Subsystem::InitializeV8Engine() const
{
    DAEMON_LOG(LogScript, eLogVerbosity::Display, StringFormat("(V8Subsystem::InitializeV8Engine)(start)"));

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
        DAEMON_LOG(LogScript, eLogVerbosity::Error, StringFormat("(V8Subsystem::InitializeV8Engine)(failed to create V8 Isolate!)"));
        return false;
    }

    // Create Context
    v8::Isolate::Scope           isolateScope(m_impl->isolate);                     // Thread safety, make this isolate active for the current thread
    v8::HandleScope              handleScope(m_impl->isolate);                      // Memory safety, manage temporary V8 object handles automatically
    v8::Local<v8::Context> const localContext = v8::Context::New(m_impl->isolate);  // Create new JavaScript execution context
    m_impl->globalContext.Reset(m_impl->isolate, localContext);                     // Convert local context to persistent handle for long-term storage
    m_impl->isInitialized = true;

    // Initialize Chrome DevTools Inspector if enabled
    if (m_config.enableInspector)
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Display, StringFormat("Initializing Chrome DevTools Inspector on {}:{}", m_config.inspectorHost, m_config.inspectorPort));

        // Create inspector client and inspector
        m_impl->inspectorClient = std::make_unique<V8InspectorClientImpl>(const_cast<V8Subsystem*>(this));
        m_impl->inspector       = v8_inspector::V8Inspector::create(m_impl->isolate, m_impl->inspectorClient.get());

        // Register the JavaScript context with the inspector
        v8_inspector::StringView contextName = v8_inspector::StringView(
            reinterpret_cast<const uint8_t*>("FirstV8 JavaScript Context"), 26
        );

        v8_inspector::V8ContextInfo contextInfo(localContext, m_impl->kContextGroupId, contextName);
        contextInfo.hasMemoryOnConsole = true;
        m_impl->inspector->contextCreated(contextInfo);

        // Create inspector channel and session for Chrome DevTools communication
        m_impl->inspectorChannel = std::make_unique<V8InspectorChannelImpl>(const_cast<V8Subsystem*>(this), nullptr);
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

    DAEMON_LOG(LogScript, eLogVerbosity::Display, StringFormat("(V8Subsystem::InitializeV8Engine)(end)"));
    return true;
}

//----------------------------------------------------------------------------------------------------
void V8Subsystem::ShutdownV8Engine()
{
    if (!m_impl->isInitialized) return;

    DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("(V8Subsystem::ShutdownV8Engine)(start)"));

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

    // 清理 Isolate
    if (m_impl->isolate)
    {
        m_impl->isolate->Dispose();
        m_impl->isolate = nullptr;
    }

    // 清理 Allocator
    if (m_impl->allocator != nullptr)
    {
        m_impl->allocator = nullptr;
    }

    // 清理平台
    v8::V8::Dispose();
    v8::V8::DisposePlatform();

    m_impl->isInitialized = false;
    DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("(V8Subsystem::ShutdownV8Engine)(end)"));
}

//----------------------------------------------------------------------------------------------------
void V8Subsystem::SetupV8Bindings()
{
    if (!m_isInitialized) ERROR_AND_DIE(StringFormat("(V8Subsystem::SetupV8Bindings)(V8Subsystem is not initialized)"))

    DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("(V8Subsystem::SetupV8Bindings)(start)"));

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

    DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("(V8Subsystem::SetupV8Bindings)(end)"));
}

//----------------------------------------------------------------------------------------------------
void V8Subsystem::CreateSingleObjectBinding(String const&                             objectName,
                                            std::shared_ptr<IScriptableObject> const& object)
{
    if (!m_impl->isolate) return;

    // Check if already bound to prevent duplicates
    if (m_boundObjects.contains(objectName))
    {
        DebuggerPrintf("V8Subsystem: 物件 %s 已綁定，跳過重複綁定\n", objectName.c_str());
        return;
    }

    v8::Isolate::Scope           isolateScope(m_impl->isolate);
    v8::HandleScope              handleScope(m_impl->isolate);
    v8::Local<v8::Context> const localContext = m_impl->globalContext.Get(m_impl->isolate);
    v8::Context::Scope           contextScope(localContext);
    v8::Local<v8::Object>        global = localContext->Global();

    DebuggerPrintf("V8Subsystem: 創建 V8 綁定 - 物件: %s\n", objectName.c_str());

    // Create JavaScript object for this specific object only
    v8::Local<v8::Object> jsObject = v8::Object::New(m_impl->isolate);

    // Get object's available methods
    auto methods = object->GetAvailableMethods();

    for (const auto& method : methods)
    {
        DebuggerPrintf("V8Subsystem: 綁定方法 %s.%s\n", objectName.c_str(), method.name.c_str());

        // Create V8 function callback for each method (same as original implementation)
        auto methodCallback = [](const v8::FunctionCallbackInfo<v8::Value>& args) {
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
                        args.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, str.c_str()).ToLocalChecked());
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

    // Bind object to global scope
    global->Set(localContext,
                v8::String::NewFromUtf8(m_impl->isolate, objectName.c_str()).ToLocalChecked(),
                jsObject).Check();

    // Mark as bound
    m_boundObjects.insert(objectName);

    DebuggerPrintf("V8Subsystem: 物件 %s 已綁定到 JavaScript 全域範圍\n", objectName.c_str());
}

//----------------------------------------------------------------------------------------------------
void V8Subsystem::CreateSingleFunctionBinding(const String&         functionName,
                                              const ScriptFunction& function)
{
    if (!m_impl->isolate) return;

    // Check if already bound to prevent duplicates
    if (m_boundFunctions.contains(functionName))
    {
        DebuggerPrintf("V8Subsystem: 函式 %s 已綁定，跳過重複綁定\n", functionName.c_str());
        return;
    }

    v8::Isolate::Scope           isolateScope(m_impl->isolate);
    v8::HandleScope              handleScope(m_impl->isolate);
    v8::Local<v8::Context> const localContext = m_impl->globalContext.Get(m_impl->isolate);
    v8::Context::Scope           contextScope(localContext);
    v8::Local<v8::Object>        global = localContext->Global();

    DebuggerPrintf("V8Subsystem: 綁定全域函式: %s\n", functionName.c_str());

    // Create function callback (same as original implementation)
    auto functionCallback = [](const v8::FunctionCallbackInfo<v8::Value>& args) {
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

//----------------------------------------------------------------------------------------------------
// TODO: integrate this with LogSubsystem or more console-related function, such as console.warn and console.error, etc...
void V8Subsystem::SetupBuiltinObjects()
{
    DAEMON_LOG(LogScript, eLogVerbosity::Display, StringFormat("(V8Subsystem::SetupBuiltinObjects)(start)"));

    if (m_impl->isolate == nullptr) ERROR_AND_DIE(StringFormat("(V8Subsystem::SetupBuiltinObjects)(v8::Isolate* is nullptr)"));

    v8::Isolate::Scope           isolateScope(m_impl->isolate);
    v8::HandleScope              handleScope(m_impl->isolate);
    v8::Local<v8::Context> const localContext = m_impl->globalContext.Get(m_impl->isolate);
    v8::Context::Scope           contextScope(localContext);

    if (m_config.enableConsoleOutput)
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Display, StringFormat("(V8Subsystem::SetupBuiltinObjects)(start)(enableConsoleOutput)"));

        // create console object
        v8::Local<v8::Object> const console = v8::Object::New(m_impl->isolate);

        // create console.log callback with Chrome DevTools integration
        static auto consoleLogCallback = [](v8::FunctionCallbackInfo<v8::Value> const& args) {
            v8::Isolate*           isolate = args.GetIsolate();
            v8::HandleScope        scope(isolate);
            v8::Local<v8::Context> context = isolate->GetCurrentContext();

            // Get V8Subsystem instance from the data parameter
            v8::Local<v8::External> external    = v8::Local<v8::External>::Cast(args.Data());
            V8Subsystem*            v8Subsystem = static_cast<V8Subsystem*>(external->Value());

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

            // Log to C++ logging system
            DAEMON_LOG(LogScript, eLogVerbosity::Display, StringFormat("{}", output));

            // Forward to Chrome DevTools Console if Inspector is enabled
            if (v8Subsystem->m_impl->inspector && v8Subsystem->m_impl->inspectorSession)
            {
                v8Subsystem->ForwardConsoleMessageToDevTools(consoleMessage);
            }
        };

        // Create external wrapper for 'this' pointer
        v8::Local<v8::External> external = v8::External::New(m_impl->isolate, this);

        // 修正：直接創建函式
        v8::Local<v8::Function> const logFunction = v8::Function::New(localContext, consoleLogCallback, external).ToLocalChecked();
        console->Set(localContext,
                     v8::String::NewFromUtf8(m_impl->isolate, "log").ToLocalChecked(),
                     logFunction).Check();

        // 將 console 物件綁定到全域範圍
        v8::Local<v8::Object> const global = localContext->Global();
        global->Set(localContext,
                    v8::String::NewFromUtf8(m_impl->isolate, "console").ToLocalChecked(),
                    console).Check();
    }
    else
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Display, StringFormat("(V8Subsystem::SetupBuiltinObjects)(enableConsoleOutput is false, skip...)"));
    }
}

//----------------------------------------------------------------------------------------------------
void V8Subsystem::HandleV8Error(String const& error)
{
    m_hasError  = true;
    m_lastError = error;
    m_stats.errorsEncountered++;

    DAEMON_LOG(LogScript, eLogVerbosity::Error, StringFormat("(V8Subsystem::HandleV8Error)({})", error));
}

//----------------------------------------------------------------------------------------------------
void* V8Subsystem::ConvertToV8Value(const std::any& value)
{
    // 實作 C++ std::any 到 V8 值的轉換
    // 這是一個複雜的函式，需要處理各種類型
    UNUSED(value)
    return nullptr; // 占位符
}

//----------------------------------------------------------------------------------------------------
std::any V8Subsystem::ConvertFromV8Value(void* v8Value)
{
    // 實作 V8 值到 C++ std::any 的轉換
    UNUSED(v8Value)
    return std::any{}; // 占位符
}

//----------------------------------------------------------------------------------------------------
String V8Subsystem::ValidateScriptPath(const String& filename) const
{
    String fullPath;

    // 檢查是否已經是完整路徑
    if (filename.find(':') != String::npos || filename[0] == '/' || filename[0] == '\\')
    {
        fullPath = filename;
    }
    else
    {
        // 檢查檔案名是否已經包含路徑前綴
        if (filename.find(m_config.scriptPath) == 0)
        {
            // 已經包含路徑前綴，直接使用
            fullPath = filename;
        }
        else
        {
            // 沒有路徑前綴，添加它
            fullPath = m_config.scriptPath + filename;
        }
    }

    // 確保副檔名是 .js
    if (fullPath.length() < 3 || fullPath.substr(fullPath.length() - 3) != ".js")
    {
        fullPath += ".js";
    }

    return fullPath;
}

//----------------------------------------------------------------------------------------------------
// DevTools Panel Event Generation Methods
//----------------------------------------------------------------------------------------------------

void V8Subsystem::SendPerformanceTimelineEvent(const std::string& eventType, const std::string& name, double timestamp)
{
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

void V8Subsystem::SendNetworkRequestEvent(const std::string& url, const std::string& method, int statusCode)
{
    if (!m_isInitialized || !m_devToolsServer) return;

    // Create Network request event notification  
    // This populates the Network panel with request data
    std::string requestId = "req_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    double timestamp = static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());

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

void V8Subsystem::SendMemoryHeapSnapshot()
{
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
        while ((pos = escapedSnapshot.find("\"", pos)) != std::string::npos) {
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
