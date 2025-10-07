//----------------------------------------------------------------------------------------------------
// ScriptReloader.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Script/ScriptReloader.hpp"
//----------------------------------------------------------------------------------------------------
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/LogSubsystem.hpp"
#include "Engine/Script/ModuleLoader.hpp"
#include "Engine/Script/ScriptSubsystem.hpp"

//----------------------------------------------------------------------------------------------------
ScriptReloader::ScriptReloader()
{
    // Constructor - initialize to default state
}

//----------------------------------------------------------------------------------------------------
ScriptReloader::~ScriptReloader()
{
    Shutdown();
}

bool ScriptReloader::Initialize(ScriptSubsystem* scriptSystem, ModuleLoader* moduleLoader)
{
    if (!scriptSystem)
    {
        SetError("ScriptSubsystem pointer is null");
        return false;
    }

    m_scriptSystem      = scriptSystem;
    m_moduleLoader      = moduleLoader;
    m_reloadCount       = 0;
    m_successfulReloads = 0;
    m_failedReloads     = 0;
    m_lastError.clear();

    LogReloadEvent("ScriptReloader initialized");
    if (m_moduleLoader)
    {
        LogReloadEvent("ScriptReloader: ES6 module support enabled");
    }
    return true;
}

void ScriptReloader::Shutdown()
{
    if (m_isReloading)
    {
        LogReloadEvent("Warning: Shutting down while reload in progress");
    }

    ClearPreservedState();
    m_scriptSystem           = nullptr;
    m_reloadCompleteCallback = nullptr;

    LogReloadEvent("ScriptReloader shutdown completed");
}

bool ScriptReloader::ReloadScript(String const& scriptPath)
{
    std::vector<std::string> paths = {scriptPath};
    return ReloadScripts(paths);
}

bool ScriptReloader::ReloadScripts(const std::vector<std::string>& scriptPaths)
{
    if (m_isReloading)
    {
        SetError("Reload already in progress");
        return false;
    }

    if (!m_scriptSystem)
    {
        SetError("ScriptSubsystem not initialized");
        return false;
    }

    if (scriptPaths.empty())
    {
        SetError("No script paths provided");
        return false;
    }

    LogReloadEvent("Starting reload of " + std::to_string(scriptPaths.size()) + " scripts");

    m_isReloading = true;
    m_reloadCount++;

    bool success = PerformReload(scriptPaths);

    m_isReloading = false;

    if (success)
    {
        m_successfulReloads++;
        LogReloadEvent("Reload completed successfully");
    }
    else
    {
        m_failedReloads++;
        LogReloadEvent("Reload failed: " + m_lastError);
    }

    // Notify completion
    if (m_reloadCompleteCallback)
    {
        m_reloadCompleteCallback(success, m_lastError);
    }

    return success;
}

void ScriptReloader::SetReloadCompleteCallback(ReloadCompleteCallback callback)
{
    m_reloadCompleteCallback = callback;
}

bool ScriptReloader::PreserveJavaScriptState()
{
    if (!m_statePreservationEnabled)
    {
        LogReloadEvent("State preservation disabled, skipping");
        return true;
    }

    try
    {
        LogReloadEvent("Preserving JavaScript state...");

        // Create state preservation script
        std::string preservationScript = CreateStatePreservationScript();

        // Execute preservation script in V8
        if (m_scriptSystem->ExecuteScript(preservationScript))
        {
            // Note: ExecuteScript doesn't return the result, so we'll use a simpler approach
            // For now, we'll assume preservation succeeded if execution succeeded
            m_preservedState = "state_preserved";
            LogReloadEvent("JavaScript state preservation executed successfully");
            return true;
        }
        else
        {
            SetError("Failed to execute state preservation script");
            return false;
        }
    }
    catch (const std::exception& e)
    {
        SetError("State preservation exception: " + std::string(e.what()));
        return false;
    }
}

bool ScriptReloader::RestoreJavaScriptState()
{
    if (!m_statePreservationEnabled || m_preservedState.empty())
    {
        LogReloadEvent("No state to restore or preservation disabled");
        return true;
    }

    try
    {
        LogReloadEvent("Restoring JavaScript state...");

        // Create state restoration script
        std::string restorationScript = CreateStateRestorationScript();

        // Execute restoration script in V8
        if (m_scriptSystem->ExecuteScript(restorationScript))
        {
            LogReloadEvent("JavaScript state restored successfully");
            return true;
        }
        else
        {
            SetError("Failed to execute state restoration script");
            return false;
        }
    }
    catch (const std::exception& e)
    {
        SetError("State restoration exception: " + std::string(e.what()));
        return false;
    }
}

void ScriptReloader::ClearPreservedState()
{
    m_preservedState.clear();
}

bool ScriptReloader::PerformReload(std::vector<std::string> const& scriptPaths)
{
    try
    {
        // Phase 1: Preserve current JavaScript state
        if (!PreserveJavaScriptState())
        {
            return false;
        }

        // Phase 2: Reload all scripts
        LogReloadEvent("Reloading scripts...");
        for (const auto& scriptPath : scriptPaths)
        {
            if (!ExecuteScript(scriptPath))
            {
                // Attempt to restore state on failure
                // RestoreJavaScriptState();
                return false;
            }
        }

        // Phase 3: Restore preserved state
        // if (!RestoreJavaScriptState()) {
        //     LogReloadEvent("Warning: State restoration failed, but scripts were reloaded");
        //     // Don't fail the entire reload for state restoration issues
        // }

        // Phase 4: Clear preserved state
        ClearPreservedState();

        return true;
    }
    catch (std::exception const& e)
    {
        SetError("Reload exception: " + std::string(e.what()));
        return false;
    }
}

bool ScriptReloader::ExecuteScript(String const& scriptPath)
{
    try
    {
        LogReloadEvent("Executing script: " + scriptPath);

        // PHASE 5: Check if this is an ES6 module (.js file)
        if (IsES6Module(scriptPath))
        {
            LogReloadEvent("Detected ES6 module, routing to ModuleLoader: " + scriptPath);
            return ReloadES6Module(scriptPath);
        }

        // Classic script handling (.js files)
        LogReloadEvent("Processing as classic script: " + scriptPath);

        // Read script file content
        std::string scriptContent;
        if (!ReadScriptFile(scriptPath, scriptContent))
        {
            return false;
        }

        // For InputSystem.js, use special reloading strategy to avoid class re-declaration
        if (scriptPath.find("InputSystem.js") != std::string::npos)
        {
            return ReloadInputSystemScript(scriptContent);
        }

        // For other scripts, use the original approach
        if (m_scriptSystem->ExecuteScript(scriptContent))
        {
            LogReloadEvent("Script executed successfully: " + scriptPath);
            return true;
        }
        else
        {
            SetError("Failed to execute script: " + scriptPath);
            return false;
        }
    }
    catch (const std::exception& e)
    {
        SetError("Script execution exception for " + scriptPath + ": " + std::string(e.what()));
        return false;
    }
}

bool ScriptReloader::ReloadInputSystemScript(String const& scriptContent)
{
    try
    {
        LogReloadEvent("Reloading InputSystem.js with class replacement strategy");

        // Create a script that replaces the InputSystem class without re-declaring it
        std::string reloadScript = R"(
(function() {
    try {
        // Save old InputSystem reference
        var oldInputSystem = globalThis.InputSystem;

        // Clear the InputSystem from global scope temporarily
        delete globalThis.InputSystem;

        // Execute the new InputSystem code
)" + scriptContent + R"(

        // Force version update to trigger hot-reload detection
        if (typeof InputSystem !== 'undefined') {
            InputSystem.version = Date.now();
            console.log('ScriptReloader: InputSystem hot-reloaded, new version:', InputSystem.version);

            // CRITICAL FIX: Update existing instances with new methods
            // Find all existing InputSystem instances and replace their methods
            console.log('ScriptReloader: Checking for existing InputSystem instances...');
            if (typeof globalThis.jsGameInstance !== 'undefined' &&
                globalThis.jsGameInstance &&
                globalThis.jsGameInstance.inputSystem) {

                console.log('ScriptReloader: Found existing InputSystem instance, replacing with new version');
                var oldInstance = globalThis.jsGameInstance.inputSystem;
                var savedState = {
                    lastF1State: oldInstance.lastF1State || false
                };

                // Create new instance with saved state
                var newInstance = new InputSystem();
                newInstance.lastF1State = savedState.lastF1State;

                // Replace the instance in JSGame
                globalThis.jsGameInstance.inputSystem = newInstance;

                console.log('ScriptReloader: Updated existing InputSystem instance with new methods');
            } else {
                console.log('ScriptReloader: No existing InputSystem instance found or jsGameInstance not available');
            }
        }

        console.log('ScriptReloader: InputSystem.js reloaded successfully');
        return { success: true, message: 'InputSystem reloaded successfully' };
    } catch (e) {
        // Restore old InputSystem if reload failed
        if (typeof oldInputSystem !== 'undefined') {
            globalThis.InputSystem = oldInputSystem;
        }
        console.log('ScriptReloader: InputSystem reload failed:', e.message);
        return { success: false, error: e.message, stack: e.stack };
    }
})();
)";

        // Execute the reload script in V8
        if (m_scriptSystem->ExecuteScript(reloadScript))
        {
            LogReloadEvent("InputSystem.js reloaded successfully");
            return true;
        }
        else
        {
            SetError("Failed to reload InputSystem.js");
            return false;
        }
    }
    catch (const std::exception& e)
    {
        SetError("InputSystem reload exception: " + std::string(e.what()));
        return false;
    }
}

bool ScriptReloader::ReadScriptFile(String const& scriptPath, std::string& content)
{
    try
    {
        // Use the script path directly (now receiving absolute paths from GameScriptInterface)
        std::filesystem::path fullPath(scriptPath);

        if (!std::filesystem::exists(fullPath))
        {
            SetError("Script file does not exist: " + fullPath.string());
            return false;
        }

        // Read file content with UTF-8 handling
        std::ifstream file(fullPath, std::ios::binary);
        if (!file.is_open())
        {
            SetError("Failed to open script file: " + fullPath.string());
            return false;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        content = buffer.str();
        file.close();

        LogReloadEvent("Read " + std::to_string(content.size()) + " bytes from: " + scriptPath);

        // Debug: Log a snippet of the handleInput method to verify we're reading the updated file
        size_t handleInputPos = content.find("handleInput(deltaTime)");
        if (handleInputPos != std::string::npos)
        {
            // Find the next console.log line after handleInput
            size_t logPos = content.find("console.log", handleInputPos);
            if (logPos != std::string::npos)
            {
                size_t lineStart = content.rfind('\n', logPos) + 1;
                size_t lineEnd   = content.find('\n', logPos);
                if (lineEnd != std::string::npos)
                {
                    std::string logLine = content.substr(lineStart, lineEnd - lineStart);
                    LogReloadEvent("First console.log in handleInput: " + logLine);
                }
            }
        }

        return true;
    }
    catch (const std::exception& e)
    {
        SetError("File reading exception for " + scriptPath + ": " + std::string(e.what()));
        return false;
    }
}

bool ScriptReloader::PreserveSpecificObjects()
{
    // This could be expanded to preserve specific game objects
    // For now, we rely on the general state preservation
    return true;
}

bool ScriptReloader::RestoreSpecificObjects()
{
    // This could be expanded to restore specific game objects
    // For now, we rely on the general state restoration
    return true;
}

std::string ScriptReloader::CreateStatePreservationScript()
{
    return R"(
        (function() {
            try {
                // Preserve critical game state
                var preservedState = {
                    // Preserve InputSystem state if it exists
                    inputSystemVersion: (typeof globalThis.jsGameInstance !== 'undefined' &&
                                       globalThis.jsGameInstance.inputSystemVersion) || 0,

                    // Preserve shouldRender flag
                    shouldRender: (typeof globalThis.shouldRender !== 'undefined') ?
                                  globalThis.shouldRender : true,

                    // Preserve JSGame state
                    gameFrameCount: (typeof globalThis.jsGameInstance !== 'undefined' &&
                                   globalThis.jsGameInstance.frameCount) || 0,

                    // Add more state preservation as needed
                    timestamp: Date.now()
                };

                // Store preserved state globally for restoration
                globalThis._hotReloadPreservedState = preservedState;

                return JSON.stringify(preservedState);
            } catch (e) {
                return '{"error": "' + e.message + '"}';
            }
        })();
    )";
}

std::string ScriptReloader::CreateStateRestorationScript()
{
    return R"(
        (function() {
            try {
                // Retrieve preserved state
                var preservedState = globalThis._hotReloadPreservedState;
                if (!preservedState) {
                    return '{"result": "No preserved state found"}';
                }

                // Restore shouldRender flag
                if (typeof preservedState.shouldRender !== 'undefined') {
                    globalThis.shouldRender = preservedState.shouldRender;
                }

                // Restore JSGame frame count if JSGame exists
                if (typeof globalThis.jsGameInstance !== 'undefined' &&
                    typeof preservedState.gameFrameCount !== 'undefined') {
                    globalThis.jsGameInstance.frameCount = preservedState.gameFrameCount;
                }

                // Force InputSystem version reset to trigger reload detection
                if (typeof globalThis.jsGameInstance !== 'undefined') {
                    globalThis.jsGameInstance.inputSystemVersion = 0;
                }

                // Clean up preserved state
                delete globalThis._hotReloadPreservedState;

                return '{"result": "State restored successfully"}';
            } catch (e) {
                return '{"error": "' + e.message + '"}';
            }
        })();
    )";
}

void ScriptReloader::SetError(String const& error)
{
    m_lastError = error;
    DAEMON_LOG(LogScript, eLogVerbosity::Error, StringFormat("ScriptReloader Error: {}", error));
}

void ScriptReloader::LogReloadEvent(String const& message)
{
    DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("ScriptReloader: {}", message));
}

//----------------------------------------------------------------------------------------------------
// ES6 Module Hot-Reload Support (Phase 5)
//----------------------------------------------------------------------------------------------------

bool ScriptReloader::IsES6Module(String const& filePath) const
{
    // All .js files are treated as ES6 modules
    // This allows us to use a single .js extension for all JavaScript files
    if (filePath.length() >= 3)
    {
        std::string extension = filePath.substr(filePath.length() - 3);
        if (extension == ".js")
        {
            return true;
        }
    }

    return false;
}

bool ScriptReloader::ReloadES6Module(String const& modulePath)
{
    if (!m_moduleLoader)
    {
        SetError("ModuleLoader not initialized - cannot reload ES6 modules");
        return false;
    }

    LogReloadEvent("Reloading ES6 module: " + modulePath);

    // Use ModuleLoader::ReloadModule() which handles:
    // 1. InvalidateModuleTree() to clear dependent modules
    // 2. Reload from main.js to trigger full import chain
    // 3. Enhanced logging for debugging
    bool success = m_moduleLoader->ReloadModule(modulePath);

    if (success)
    {
        LogReloadEvent("ES6 module reloaded successfully: " + modulePath);
    }
    else
    {
        SetError("Failed to reload ES6 module: " + modulePath);
    }

    return success;
}
