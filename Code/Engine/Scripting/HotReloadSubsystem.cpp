//----------------------------------------------------------------------------------------------------
// HotReloadSubsystem.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Scripting/HotReloadSubsystem.hpp"

#include <filesystem>
#include <stdexcept>

#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/LogSubsystem.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Scripting/V8Subsystem.hpp"
#include "Engine/Scripting/FileWatcher.hpp"
#include "Engine/Scripting/ScriptReloader.hpp"

//----------------------------------------------------------------------------------------------------
HotReloadSubsystem::HotReloadSubsystem()
    : m_fileWatcher(std::make_unique<FileWatcher>())
    , m_scriptReloader(std::make_unique<ScriptReloader>())
{
    DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("HotReloadSubsystem: Created"));
}

//----------------------------------------------------------------------------------------------------
HotReloadSubsystem::~HotReloadSubsystem()
{
    Shutdown();
    DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("HotReloadSubsystem: Destroyed"));
}

//----------------------------------------------------------------------------------------------------
bool HotReloadSubsystem::Initialize(V8Subsystem* v8System, const std::string& projectRoot)
{
    try
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("HotReloadSubsystem: Initializing hot-reload system..."));

        if (!v8System)
        {
            DAEMON_LOG(LogScript, eLogVerbosity::Error, StringFormat("HotReloadSubsystem: V8Subsystem cannot be null"));
            return false;
        }

        // Store project root for path construction
        m_projectRoot = projectRoot;

        // Initialize FileWatcher
        if (!m_fileWatcher->Initialize(projectRoot))
        {
            DAEMON_LOG(LogScript, eLogVerbosity::Error, StringFormat("HotReloadSubsystem: Failed to initialize FileWatcher"));
            return false;
        }

        // Initialize ScriptReloader
        if (!m_scriptReloader->Initialize(v8System))
        {
            DAEMON_LOG(LogScript, eLogVerbosity::Error, StringFormat("HotReloadSubsystem: Failed to initialize ScriptReloader"));
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

        // Add default watched files
        m_fileWatcher->AddWatchedFile("Data/Scripts/JSEngine.js");
        m_fileWatcher->AddWatchedFile("Data/Scripts/JSGame.js");
        m_fileWatcher->AddWatchedFile("Data/Scripts/InputSystem.js");

        // Start the file watching thread
        m_fileWatcher->StartWatching();

        m_enabled = true;
        DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("HotReloadSubsystem: Hot-reload system initialized successfully"));

        return true;
    }
    catch (const std::exception& e)
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Error, StringFormat("HotReloadSubsystem: Initialize failed with exception: {}", e.what()));
        return false;
    }
}

//----------------------------------------------------------------------------------------------------
void HotReloadSubsystem::Update()
{
    if (!m_enabled)
        return;

    // Process pending file changes on main thread
    ProcessPendingEvents();
}

//----------------------------------------------------------------------------------------------------
void HotReloadSubsystem::Shutdown()
{
    if (m_enabled)
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("HotReloadSubsystem: Shutting down hot-reload system..."));
        
        m_enabled = false;

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

        DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("HotReloadSubsystem: Hot-reload system shutdown complete"));
    }
}

//----------------------------------------------------------------------------------------------------
void HotReloadSubsystem::SetEnabled(bool enabled)
{
    m_enabled = enabled;
    DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("HotReloadSubsystem: Hot-reload {}", enabled ? "enabled" : "disabled"));
}

//----------------------------------------------------------------------------------------------------
void HotReloadSubsystem::AddWatchedFile(const std::string& relativePath)
{
    if (m_fileWatcher)
    {
        m_fileWatcher->AddWatchedFile(relativePath);
        DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("HotReloadSubsystem: Added watched file: {}", relativePath));
    }
}

//----------------------------------------------------------------------------------------------------
void HotReloadSubsystem::RemoveWatchedFile(const std::string& relativePath)
{
    if (m_fileWatcher)
    {
        m_fileWatcher->RemoveWatchedFile(relativePath);
        DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("HotReloadSubsystem: Removed watched file: {}", relativePath));
    }
}

//----------------------------------------------------------------------------------------------------
std::vector<std::string> HotReloadSubsystem::GetWatchedFiles() const
{
    if (m_fileWatcher)
    {
        return m_fileWatcher->GetWatchedFiles();
    }
    return {};
}

//----------------------------------------------------------------------------------------------------
void HotReloadSubsystem::ReloadScript(const std::string& relativePath)
{
    if (m_scriptReloader && m_enabled)
    {
        std::string absolutePath = GetAbsoluteScriptPath(relativePath);
        m_scriptReloader->ReloadScript(absolutePath);
        DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("HotReloadSubsystem: Manual reload triggered for: {}", relativePath));
    }
}

//----------------------------------------------------------------------------------------------------
void HotReloadSubsystem::ProcessPendingEvents()
{
    try
    {
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

            DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("HotReloadSubsystem: Processing file change on main thread: {}", filePath));

            // Convert relative path to absolute path for ScriptReloader
            std::string absolutePath = GetAbsoluteScriptPath(filePath);

            // Now safe to call V8 from main thread
            if (m_scriptReloader && m_enabled)
            {
                m_scriptReloader->ReloadScript(absolutePath);
            }

            filesToProcess.pop();
        }
    }
    catch (const std::exception& e)
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Error, StringFormat("HotReloadSubsystem: Error processing pending hot-reload events: {}", e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
void HotReloadSubsystem::OnFileChanged(const std::string& filePath)
{
    try
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("HotReloadSubsystem: File changed (queuing for main thread): {}", filePath));

        // Queue the file change for main thread processing (thread-safe)
        if (m_enabled)
        {
            std::lock_guard<std::mutex> lock(m_fileChangeQueueMutex);
            m_pendingFileChanges.push(filePath);
        }
    }
    catch (const std::exception& e)
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Error, StringFormat("HotReloadSubsystem: Error handling file change: {}", e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
void HotReloadSubsystem::OnReloadComplete(bool success, const std::string& error)
{
    if (success)
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("HotReloadSubsystem: Script reload completed successfully"));
    }
    else
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Error, StringFormat("HotReloadSubsystem: Script reload failed: {}", error));
    }
}

//----------------------------------------------------------------------------------------------------
std::string HotReloadSubsystem::GetAbsoluteScriptPath(const std::string& relativePath) const
{
    // Same logic as FileWatcher::GetFullPath()
    std::filesystem::path fullPath = std::filesystem::path(m_projectRoot) / "Run" / relativePath;
    return fullPath.string();
}