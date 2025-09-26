//----------------------------------------------------------------------------------------------------
// HotReloadSubsystem.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include <memory>
#include <queue>
#include <mutex>
#include <string>

//-Forward-Declaration--------------------------------------------------------------------------------
class V8Subsystem;
class FileWatcher;
class ScriptReloader;

//----------------------------------------------------------------------------------------------------
// Hot-reload subsystem for development builds
// Manages file watching and script reloading functionality separate from GameScriptInterface
// Follows the established engine subsystem pattern with global pointer access
//----------------------------------------------------------------------------------------------------
class HotReloadSubsystem
{
public:
    HotReloadSubsystem();
    ~HotReloadSubsystem();

    // Subsystem lifecycle - follows AudioSystem, Renderer pattern
    bool Initialize(V8Subsystem* v8System, const std::string& projectRoot);
    void Update();
    void Shutdown();

    // Configuration and control
    void SetEnabled(bool enabled);
    bool IsEnabled() const { return m_enabled; }
    
    // File watching management
    void AddWatchedFile(const std::string& relativePath);
    void RemoveWatchedFile(const std::string& relativePath);
    std::vector<std::string> GetWatchedFiles() const;
    
    // Manual reload trigger
    void ReloadScript(const std::string& relativePath);

    // Thread-safe event processing (called from main thread)
    void ProcessPendingEvents();

private:
    // Core components
    std::unique_ptr<FileWatcher>    m_fileWatcher;
    std::unique_ptr<ScriptReloader> m_scriptReloader;
    
    // Configuration
    bool        m_enabled{false};
    std::string m_projectRoot;
    
    // Thread-safe event queue for main thread processing
    std::queue<std::string> m_pendingFileChanges;
    mutable std::mutex      m_fileChangeQueueMutex;

    // Hot-reload callbacks
    void OnFileChanged(const std::string& filePath);
    void OnReloadComplete(bool success, const std::string& error);

    // Helper method to construct absolute paths (same logic as FileWatcher)
    std::string GetAbsoluteScriptPath(const std::string& relativePath) const;
};