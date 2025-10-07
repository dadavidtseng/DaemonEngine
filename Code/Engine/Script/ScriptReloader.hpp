//----------------------------------------------------------------------------------------------------
// ScriptReloader.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once

//----------------------------------------------------------------------------------------------------
#include "Engine/Core/StringUtils.hpp"
//----------------------------------------------------------------------------------------------------
#include <functional>

//-Forward-Declaration--------------------------------------------------------------------------------
class ScriptSubsystem;
class ModuleLoader;

/**
 * ScriptReloader - V8 Script Hot-Reload Management
 *
 * Handles the complex process of reloading JavaScript files in V8 context while preserving
 * game state. Manages the complete reload lifecycle including state preservation, script
 * re-execution, and state restoration.
 *
 * Features:
 * - Safe V8 script reloading without context recreation
 * - JavaScript state preservation and restoration
 * - Error handling and rollback on reload failures
 * - Integration with existing ScriptSubsystem
 * - ES6 module hot-reload support (.js files)
 * - Classic script hot-reload support (.js files)
 */
class ScriptReloader
{
public:
    using ReloadCompleteCallback = std::function<void(bool success, const String& error)>;

    ScriptReloader();
    ~ScriptReloader();

    // Core functionality
    bool Initialize(ScriptSubsystem* scriptSystem, ModuleLoader* moduleLoader = nullptr);
    void Shutdown();

    // Reload operations
    bool ReloadScript(const String& scriptPath);
    bool ReloadScripts(const std::vector<String>& scriptPaths);
    void SetReloadCompleteCallback(ReloadCompleteCallback callback);

    // State management
    bool PreserveJavaScriptState();
    bool RestoreJavaScriptState();
    void ClearPreservedState();

    // Configuration
    void SetStatePreservationEnabled(bool const enabled) { m_statePreservationEnabled = enabled; }
    bool IsStatePreservationEnabled() const { return m_statePreservationEnabled; }

    // Status and debugging
    bool   IsReloading() const { return m_isReloading; }
    String GetLastError() const { return m_lastError; }
    size_t GetReloadCount() const { return m_reloadCount; }

private:
    // Internal reload logic
    bool PerformReload(StringList const& scriptPaths);
    bool ExecuteScript(String const& scriptPath);
    bool ReadScriptFile(String const& scriptPath, String& content);

    // File type detection and routing
    bool IsES6Module(String const& filePath) const;
    bool ReloadES6Module(String const& modulePath);

    // Special reload strategies for different script types
    bool ReloadInputSystemScript(String const& scriptContent);

    // State management helpers
    bool   PreserveSpecificObjects();
    bool   RestoreSpecificObjects();
    String CreateStatePreservationScript();
    String CreateStateRestorationScript();

    // Error handling
    void SetError(String const& error);
    void LogReloadEvent(String const& message);

private:
    // Script subsystem integration
    ScriptSubsystem* m_scriptSystem{nullptr};
    ModuleLoader*    m_moduleLoader{nullptr};

    // Reload state
    bool   m_isReloading{false};
    bool   m_statePreservationEnabled{true};
    String m_preservedState;

    // Callback and error handling
    ReloadCompleteCallback m_reloadCompleteCallback;
    String                 m_lastError;

    // Statistics
    size_t m_reloadCount{0};
    size_t m_successfulReloads{0};
    size_t m_failedReloads{0};
};
