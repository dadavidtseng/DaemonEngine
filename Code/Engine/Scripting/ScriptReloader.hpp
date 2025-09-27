#pragma once

#include <functional>
#include <string>
#include <vector>

//-Forward-Declaration--------------------------------------------------------------------------------
class ScriptSubsystem;

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
 */
class ScriptReloader
{
public:
    using ReloadCompleteCallback = std::function<void(bool success, const std::string& error)>;

    ScriptReloader();
    ~ScriptReloader();

    // Core functionality
    bool Initialize(ScriptSubsystem* scriptSystem);
    void Shutdown();

    // Reload operations
    bool ReloadScript(const std::string& scriptPath);
    bool ReloadScripts(const std::vector<std::string>& scriptPaths);
    void SetReloadCompleteCallback(ReloadCompleteCallback callback);

    // State management
    bool PreserveJavaScriptState();
    bool RestoreJavaScriptState();
    void ClearPreservedState();

    // Configuration
    void SetStatePreservationEnabled(bool const enabled) { m_statePreservationEnabled = enabled; }
    bool IsStatePreservationEnabled() const { return m_statePreservationEnabled; }

    // Status and debugging
    bool        IsReloading() const { return m_isReloading; }
    std::string GetLastError() const { return m_lastError; }
    size_t      GetReloadCount() const { return m_reloadCount; }

private:
    // Internal reload logic
    bool PerformReload(const std::vector<std::string>& scriptPaths);
    bool ExecuteScript(const std::string& scriptPath);
    bool ReadScriptFile(const std::string& scriptPath, std::string& content);

    // Special reload strategies for different script types
    bool ReloadInputSystemScript(const std::string& scriptContent);

    // State management helpers
    bool        PreserveSpecificObjects();
    bool        RestoreSpecificObjects();
    std::string CreateStatePreservationScript();
    std::string CreateStateRestorationScript();

    // Error handling
    void SetError(const std::string& error);
    void LogReloadEvent(const std::string& message);

private:
    // Script subsystem integration
    ScriptSubsystem* m_scriptSystem{nullptr};

    // Reload state
    bool        m_isReloading{false};
    bool        m_statePreservationEnabled{true};
    std::string m_preservedState;

    // Callback and error handling
    ReloadCompleteCallback m_reloadCompleteCallback;
    std::string            m_lastError;

    // Statistics
    size_t m_reloadCount{0};
    size_t m_successfulReloads{0};
    size_t m_failedReloads{0};
};
