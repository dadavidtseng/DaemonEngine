#pragma once

#include <atomic>
#include <chrono>
#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

/**
 * FileWatcher - C++ File System Monitoring for Hot-Reload
 * 
 * Monitors JavaScript files for changes and triggers hot-reload operations.
 * Uses std::filesystem for cross-platform file monitoring with efficient polling.
 * 
 * Features:
 * - std::filesystem-based file change detection
 * - Configurable polling interval
 * - Callback-based change notifications
 * - Thread-safe operation
 * - Batch change detection to avoid rapid fire reloads
 */
class FileWatcher
{
public:
    using FileChangeCallback = std::function<void(const std::string& filePath)>;
    using FileTimeMap = std::unordered_map<std::string, std::filesystem::file_time_type>;

    FileWatcher();
    ~FileWatcher();

    // Core functionality
    bool Initialize(const std::string& projectRoot);
    void Shutdown();
    
    // File monitoring
    void AddWatchedFile(const std::string& relativePath);
    void RemoveWatchedFile(const std::string& relativePath);
    void SetChangeCallback(FileChangeCallback callback);
    
    // Control operations
    void StartWatching();
    void StopWatching();
    bool IsWatching() const { return m_isWatching; }
    
    // Configuration
    void SetPollingInterval(std::chrono::milliseconds interval);
    void SetBatchDelay(std::chrono::milliseconds delay);
    
    // Status and debugging
    std::vector<std::string> GetWatchedFiles() const;
    size_t GetWatchedFileCount() const { return m_watchedFiles.size(); }
    std::chrono::milliseconds GetPollingInterval() const { return m_pollingInterval; }

private:
    // Internal monitoring logic
    void WatchingThreadFunction();
    void CheckFileChanges();
    bool HasFileChanged(const std::string& filePath);
    std::string GetFullPath(const std::string& relativePath) const;
    
    // Change detection and batching
    void ProcessFileChange(const std::string& filePath);
    void FlushPendingChanges();

private:
    // Configuration
    std::string m_projectRoot;
    std::chrono::milliseconds m_pollingInterval{500}; // Default 500ms polling
    std::chrono::milliseconds m_batchDelay{100};      // Default 100ms batch delay
    
    // File monitoring state
    std::vector<std::string> m_watchedFiles;
    FileTimeMap m_lastWriteTimes;
    FileChangeCallback m_changeCallback;
    
    // Threading and control
    std::unique_ptr<std::thread> m_watchingThread;
    std::atomic<bool> m_isWatching{false};
    std::atomic<bool> m_shouldStop{false};
    
    // Batching for rapid changes
    std::vector<std::string> m_pendingChanges;
    std::chrono::steady_clock::time_point m_lastChangeTime;
    bool m_hasPendingChanges{false};
    
    // Thread safety
    mutable std::mutex m_watchedFilesMutex;
    mutable std::mutex m_changesMutex;
};