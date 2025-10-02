//----------------------------------------------------------------------------------------------------
// FileWatcher.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Scripting/FileWatcher.hpp"
//----------------------------------------------------------------------------------------------------
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/LogSubsystem.hpp"

FileWatcher::FileWatcher()
{
    // Constructor - initialize member variables to default state
}

FileWatcher::~FileWatcher()
{
    // Ensure clean shutdown
    Shutdown();
}

bool FileWatcher::Initialize(const std::string& projectRoot)
{
    try {
        // Validate and store project root path
        if (projectRoot.empty()) {
            DAEMON_LOG(LogScript, eLogVerbosity::Error, StringFormat("FileWatcher: Project root path cannot be empty"));
            return false;
        }

        // Ensure path exists and is a directory
        std::filesystem::path rootPath(projectRoot);
        if (!std::filesystem::exists(rootPath) || !std::filesystem::is_directory(rootPath)) {
            DAEMON_LOG(LogScript, eLogVerbosity::Error, StringFormat("FileWatcher: Invalid project root path: {}", projectRoot));
            return false;
        }

        // Store normalized project root path
        m_projectRoot = std::filesystem::absolute(rootPath).string();

        // Ensure trailing slash for consistent path joining
        if (m_projectRoot.back() != '\\' && m_projectRoot.back() != '/') {
            m_projectRoot += "\\";
        }

        DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("FileWatcher: Initialized with project root: {}", m_projectRoot));
        return true;
    }
    catch (const std::exception& e) {
        DAEMON_LOG(LogScript, eLogVerbosity::Error, StringFormat("FileWatcher: Initialization failed: {}", e.what()));
        return false;
    }
}

void FileWatcher::Shutdown()
{
    try {
        // Stop watching if currently active
        StopWatching();
        
        // Clear all state
        {
            std::lock_guard<std::mutex> lock(m_watchedFilesMutex);
            m_watchedFiles.clear();
            m_lastWriteTimes.clear();
        }
        
        {
            std::lock_guard<std::mutex> lock(m_changesMutex);
            m_pendingChanges.clear();
            m_hasPendingChanges = false;
        }
        
        m_changeCallback = nullptr;
        
        DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("FileWatcher: Shutdown completed"));
    }
    catch (const std::exception& e) {
        DAEMON_LOG(LogScript, eLogVerbosity::Error, StringFormat("FileWatcher: Shutdown error: {}", e.what()));
    }
}

void FileWatcher::AddWatchedFile(const std::string& relativePath)
{
    try {
        std::lock_guard<std::mutex> lock(m_watchedFilesMutex);
        
        // Check if already watching this file
        auto it = std::find(m_watchedFiles.begin(), m_watchedFiles.end(), relativePath);
        if (it != m_watchedFiles.end()) {
            DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("FileWatcher: Already watching file: {}", relativePath));
            return;
        }
        
        // Verify file exists
        std::string fullPath = GetFullPath(relativePath);
        if (!std::filesystem::exists(fullPath)) {
            DAEMON_LOG(LogScript, eLogVerbosity::Error, StringFormat("FileWatcher: Cannot watch non-existent file: {}", fullPath));
            return;
        }
        
        // Add to watched files and record initial timestamp
        m_watchedFiles.push_back(relativePath);
        m_lastWriteTimes[relativePath] = std::filesystem::last_write_time(fullPath);
        
        DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("FileWatcher: Added watched file: {}", relativePath));
    }
    catch (const std::exception& e) {
        DAEMON_LOG(LogScript, eLogVerbosity::Error, StringFormat("FileWatcher: Failed to add watched file {}: {}", relativePath, e.what()));
    }
}

void FileWatcher::RemoveWatchedFile(const std::string& relativePath)
{
    try {
        std::lock_guard<std::mutex> lock(m_watchedFilesMutex);
        
        // Remove from watched files vector
        auto it = std::find(m_watchedFiles.begin(), m_watchedFiles.end(), relativePath);
        if (it != m_watchedFiles.end()) {
            m_watchedFiles.erase(it);
            m_lastWriteTimes.erase(relativePath);
            DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("FileWatcher: Removed watched file: {}", relativePath));
        }
        else {
            DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("FileWatcher: File not being watched: {}", relativePath));
        }
    }
    catch (const std::exception& e) {
        DAEMON_LOG(LogScript, eLogVerbosity::Error, StringFormat("FileWatcher: Failed to remove watched file {}: {}", relativePath, e.what()));
    }
}

void FileWatcher::SetChangeCallback(FileChangeCallback callback)
{
    m_changeCallback = callback;
    DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("FileWatcher: Change callback {}", (callback ? "set" : "cleared")));
}

void FileWatcher::StartWatching()
{
    try {
        if (m_isWatching) {
            DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("FileWatcher: Already watching files"));
            return;
        }
        
        if (m_watchedFiles.empty()) {
            DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("FileWatcher: No files to watch"));
            return;
        }
        
        if (!m_changeCallback) {
            DAEMON_LOG(LogScript, eLogVerbosity::Error, StringFormat("FileWatcher: No change callback set"));
            return;
        }
        
        // Reset control flags
        m_shouldStop = false;
        m_isWatching = true;
        
        // Start watching thread
        m_watchingThread = std::make_unique<std::thread>(&FileWatcher::WatchingThreadFunction, this);
        
        DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("FileWatcher: Started watching %zu files", m_watchedFiles.size()));
    }
    catch (const std::exception& e) {
        DAEMON_LOG(LogScript, eLogVerbosity::Error, StringFormat("FileWatcher: Failed to start watching: {}", e.what()));
        m_isWatching = false;
    }
}

void FileWatcher::StopWatching()
{
    try {
        if (!m_isWatching) {
            return;
        }
        
        // Signal thread to stop
        m_shouldStop = true;
        m_isWatching = false;
        
        // Wait for thread to finish
        if (m_watchingThread && m_watchingThread->joinable()) {
            m_watchingThread->join();
            m_watchingThread.reset();
        }
        
        DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("FileWatcher: Stopped watching files"));
    }
    catch (const std::exception& e) {
        DAEMON_LOG(LogScript, eLogVerbosity::Error, StringFormat("FileWatcher: Error stopping file watching: {}", e.what()));
    }
}

void FileWatcher::SetPollingInterval(std::chrono::milliseconds interval)
{
    if (interval.count() < 50) {
        DAEMON_LOG(LogScript, eLogVerbosity::Warning, StringFormat("FileWatcher: Polling interval too small, using minimum 50ms"));
        m_pollingInterval = std::chrono::milliseconds(50);
    }
    else {
        m_pollingInterval = interval;
    }
    
    DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("FileWatcher: Polling interval set to %lldms", m_pollingInterval.count()));
}

void FileWatcher::SetBatchDelay(std::chrono::milliseconds delay)
{
    m_batchDelay = delay;
    DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("FileWatcher: Batch delay set to %lldms", m_batchDelay.count()));
}

std::vector<std::string> FileWatcher::GetWatchedFiles() const
{
    std::lock_guard<std::mutex> lock(m_watchedFilesMutex);
    return m_watchedFiles; // Return copy
}

void FileWatcher::WatchingThreadFunction()
{
    try {
        DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("FileWatcher: Watching thread started"));
        
        while (!m_shouldStop) {
            // Check for file changes
            CheckFileChanges();
            
            // Process any pending batched changes
            FlushPendingChanges();
            
            // Sleep for polling interval
            std::this_thread::sleep_for(m_pollingInterval);
        }
        
        DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("FileWatcher: Watching thread stopped"));
    }
    catch (const std::exception& e) {
        DAEMON_LOG(LogScript, eLogVerbosity::Error, StringFormat("FileWatcher: Watching thread error: {}", e.what()));
    }
}

void FileWatcher::CheckFileChanges()
{
    try {
        std::lock_guard<std::mutex> lock(m_watchedFilesMutex);
        
        for (const auto& relativePath : m_watchedFiles) {
            if (HasFileChanged(relativePath)) {
                ProcessFileChange(relativePath);
            }
        }
    }
    catch (const std::exception& e) {
        DAEMON_LOG(LogScript, eLogVerbosity::Error, StringFormat("FileWatcher: Error checking file changes: {}", e.what()));
    }
}

bool FileWatcher::HasFileChanged(const std::string& relativePath)
{
    try {
        std::string fullPath = GetFullPath(relativePath);
        
        if (!std::filesystem::exists(fullPath)) {
            DAEMON_LOG(LogScript, eLogVerbosity::Warning, StringFormat("FileWatcher: Watched file no longer exists: {}", fullPath));
            return false;
        }
        
        auto currentWriteTime = std::filesystem::last_write_time(fullPath);
        auto lastWriteTime = m_lastWriteTimes[relativePath];
        
        if (currentWriteTime != lastWriteTime) {
            // Update stored time
            m_lastWriteTimes[relativePath] = currentWriteTime;
            return true;
        }
        
        return false;
    }
    catch (const std::exception& e) {
        DAEMON_LOG(LogScript, eLogVerbosity::Error, StringFormat("FileWatcher: Error checking file change for {}: {}", relativePath, e.what()));
        return false;
    }
}

std::string FileWatcher::GetFullPath(const std::string& relativePath) const
{
    // Join project root with relative path
    std::filesystem::path fullPath = std::filesystem::path(m_projectRoot) / "Run" / relativePath;
    return fullPath.string();
}

void FileWatcher::ProcessFileChange(const std::string& filePath)
{
    try {
        std::lock_guard<std::mutex> lock(m_changesMutex);
        
        // Add to pending changes for batching
        auto it = std::find(m_pendingChanges.begin(), m_pendingChanges.end(), filePath);
        if (it == m_pendingChanges.end()) {
            m_pendingChanges.push_back(filePath);
        }
        
        // Update timing for batching
        m_lastChangeTime = std::chrono::steady_clock::now();
        m_hasPendingChanges = true;
        
        DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("FileWatcher: Detected change in file: {}", filePath));
    }
    catch (const std::exception& e) {
        DAEMON_LOG(LogScript, eLogVerbosity::Error, StringFormat("FileWatcher: Error processing file change for {}: {}", filePath, e.what()));
    }
}

void FileWatcher::FlushPendingChanges()
{
    try {
        std::lock_guard<std::mutex> lock(m_changesMutex);
        
        if (!m_hasPendingChanges || m_pendingChanges.empty()) {
            return;
        }
        
        // Check if enough time has passed for batching
        auto now = std::chrono::steady_clock::now();
        auto timeSinceLastChange = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastChangeTime);
        
        if (timeSinceLastChange >= m_batchDelay) {
            // Process all pending changes
            DAEMON_LOG(LogScript, eLogVerbosity::Log, StringFormat("FileWatcher: Flushing %zu pending changes", m_pendingChanges.size()));
            
            for (const auto& filePath : m_pendingChanges) {
                if (m_changeCallback) {
                    m_changeCallback(filePath);
                }
            }
            
            // Clear pending changes
            m_pendingChanges.clear();
            m_hasPendingChanges = false;
        }
    }
    catch (const std::exception& e) {
        DAEMON_LOG(LogScript, eLogVerbosity::Error, StringFormat("FileWatcher: Error flushing pending changes: {}", e.what()));
    }
}