//----------------------------------------------------------------------------------------------------
// LogSubsystem.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Core/LogSubsystem.hpp"
//----------------------------------------------------------------------------------------------------
#include "Engine/Core/ConsoleOutputDevice.hpp"
#include "Engine/Core/DebugOutputDevice.hpp"
#include "Engine/Core/DevConsoleOutputDevice.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/FileOutputDevice.hpp"
#include "Engine/Core/OnScreenOutputDevice.hpp"
//----------------------------------------------------------------------------------------------------
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "ErrorWarningAssert.hpp"

// 外部全域變數聲明
extern HANDLE g_consoleHandle;

LogSubsystem* g_logSubsystem = nullptr;

//----------------------------------------------------------------------------------------------------
// LogEntry 實作
//----------------------------------------------------------------------------------------------------
LogEntry::LogEntry(String const&       category,
                   eLogVerbosity const verbosity,
                   String const&       message,
                   String const&       functionName,
                   String const&       fileName,
                   int const           lineNum)
    : m_category(category),
      m_verbosity(verbosity),
      m_message(message),
      m_functionName(functionName),
      m_fileName(fileName),
      m_lineNum(lineNum)
{
    if (g_logSubsystem)
    {
        m_timestamp = g_logSubsystem->GetCurrentTimestamp();
        m_threadId  = g_logSubsystem->GetCurrentThreadId();
    }
}

//----------------------------------------------------------------------------------------------------
LogSubsystem::LogSubsystem(sLogSubsystemConfig config)
    : m_config(std::move(config)),
      m_smartFileDevice(nullptr),
      m_isSmartRotationInitialized(false)
{
}



//----------------------------------------------------------------------------------------------------
void LogSubsystem::Startup()
{
    RegisterCategory("LogTemp");
    RegisterCategory("LogLog");
    RegisterCategory("LogEvent");
    RegisterCategory("LogCore");
    RegisterCategory("LogRenderer");
    RegisterCategory("LogAudio");
    RegisterCategory("LogInput");
    RegisterCategory("LogNetwork");
    RegisterCategory("LogResource");
    RegisterCategory("LogMath");
    RegisterCategory("LogPlatform");
    RegisterCategory("LogScript");
    RegisterCategory("LogGame");
    RegisterCategory("LogApp");

    LogMessage("LogLog", eLogVerbosity::Display, "LogSubsystem::Startup() start");

    // 建立輸出裝置
    if (m_config.enableConsole)
    {
        AddOutputDevice(std::make_unique<ConsoleOutputDevice>());
    }

    // Initialize smart rotation if enabled
    if (m_config.enableSmartRotation && m_config.enableFile)
    {
        try
        {
            // Load rotation configuration from file if specified
            if (!m_config.rotationConfigPath.empty())
            {
                LoadRotationConfigFromFile(m_config.rotationConfigPath);
            }

            // Create SmartFileOutputDevice with Minecraft-style rotation
            std::unique_ptr<SmartFileOutputDevice> smartDevice = std::make_unique<SmartFileOutputDevice>(
                m_config.smartRotationConfig.logDirectory,
                m_config.smartRotationConfig
            );

            // Keep a pointer for direct access
            m_smartFileDevice = smartDevice.get();
            AddOutputDevice(std::move(smartDevice));

            m_isSmartRotationInitialized = true;
            // Note: Don't log here as it might cause circular dependency during startup
        }
        catch (std::exception const& e)
        {
            UNUSED(e)
            // Fallback to regular file output
            if (m_config.enableFile)
            {
                AddOutputDevice(std::make_unique<FileOutputDevice>(m_config.logFilePath));
            }
            m_smartFileDevice            = nullptr;
            m_isSmartRotationInitialized = false;
        }
    }
    else if (m_config.enableFile)
    {
        // Use regular file output device
        AddOutputDevice(std::make_unique<FileOutputDevice>(m_config.logFilePath));
    }

    if (m_config.enableDebugOut)
    {
        AddOutputDevice(std::make_unique<DebugOutputDevice>());
    }

    if (m_config.enableOnScreen)
    {
        AddOutputDevice(std::make_unique<OnScreenOutputDevice>());
    }

    if (m_config.enableDevConsole)
    {
        AddOutputDevice(std::make_unique<DevConsoleOutputDevice>());
    }

    // 啟動非同步日誌執行緒
    if (m_config.asyncLogging)
    {
        m_shouldExit = false;
        m_logThread  = std::thread(&LogSubsystem::ProcessLogQueue, this);
    }

    // 記錄啟動訊息
    LogMessage("LogCore", eLogVerbosity::Display, "LogSubsystem::Startup() finish");

    // Log smart rotation status after full initialization
    if (m_isSmartRotationInitialized && m_smartFileDevice)
    {
        LogMessage("LogRotation", eLogVerbosity::Display, "Smart log rotation active - Minecraft-style file management enabled");
    }

    // 恢復原有的控制台顏色設定
#ifdef _WIN32
    if (g_consoleHandle && g_consoleHandle != INVALID_HANDLE_VALUE)
    {
        SetConsoleTextAttribute(g_consoleHandle, BACKGROUND_BLUE | FOREGROUND_INTENSITY);
    }
#endif
}

void LogSubsystem::Shutdown()
{
    // 記錄關閉訊息
    LogMessage("LogCore", eLogVerbosity::Display, "LogSubsystem::Shutdown() start");

    // CRITICAL: Disable async logging BEFORE stopping worker thread
    // This prevents new logs from being queued during shutdown
    bool wasAsyncLogging = m_config.asyncLogging;
    m_config.asyncLogging = false;

    // 停止非同步日誌執行緒
    if (m_logThread.joinable())
    {
        // Set stop flag first, then notify to wake the thread
        m_shouldExit.store(true, std::memory_order_release);  // Use explicit memory ordering

        m_logCondition.notify_all();  // Wake up the worker thread immediately

        m_logThread.join();
    }

    // 清理輸出裝置
    FlushAllOutputs();
    m_outputDevices.clear();

    // 清理日誌歷史
    ClearLogHistory();
    m_categories.clear();

    // This log will be written synchronously since async logging is disabled
    LogMessage("LogCore", eLogVerbosity::Display, "LogSubsystem::Shutdown() finish");

    // Restore original setting (not strictly necessary since we're shutting down)
    m_config.asyncLogging = wasAsyncLogging;
}

void LogSubsystem::BeginFrame()
{
    // 每幀開始時的處理
}

void LogSubsystem::EndFrame()
{
    // 每幀結束時的處理
    if (m_config.autoFlush)
    {
        FlushAllOutputs();
    }
}

void LogSubsystem::Update(float const deltaSeconds)
{
    // 更新螢幕輸出裝置
    for (auto& device : m_outputDevices)
    {
        if (OnScreenOutputDevice* onScreenDevice = dynamic_cast<OnScreenOutputDevice*>(device.get()))
        {
            onScreenDevice->Update(deltaSeconds);
        }
    }
}

void LogSubsystem::RegisterCategory(String const& categoryName,
                                    eLogVerbosity defaultVerbosity,
                                    eLogVerbosity compileTimeVerbosity,
                                    eLogOutput    outputTargets)
{
    LogCategory category(categoryName, defaultVerbosity, compileTimeVerbosity, outputTargets);
    m_categories[categoryName] = category;
}

void LogSubsystem::SetCategoryVerbosity(const String& categoryName, eLogVerbosity verbosity)
{
    auto it = m_categories.find(categoryName);
    if (it != m_categories.end())
    {
        it->second.defaultVerbosity = verbosity;
    }
}

LogCategory* LogSubsystem::GetCategory(String const& categoryName)
{
    auto const it = m_categories.find(categoryName);
    return (it != m_categories.end()) ? &it->second : nullptr;
}

bool LogSubsystem::IsCategoryRegistered(const String& categoryName) const
{
    return m_categories.contains(categoryName);
}

void LogSubsystem::LogMessage(String const&       categoryName,
                              eLogVerbosity const verbosity,
                              String const&       message,
                              String const&       functionName,
                              String const&       fileName,
                              int const           lineNumber)
{
    if (!ShouldLog(categoryName, verbosity))
    {
        return;
    }

    LogEntry entry(categoryName, verbosity, message, functionName, fileName, lineNumber);

    if (m_config.asyncLogging)
    {
        // 非同步日誌：加入佇列
        {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            m_logQueue.push(entry);
        }

        // Notify the worker thread that a log entry is available
        m_logCondition.notify_one();
    }
    else
    {
        // 同步日誌：直接輸出
        WriteToOutputDevices(entry);
    }

    // 加入日誌歷史 (限制數量)
    {
        std::lock_guard<std::mutex> lock(m_historyMutex);
        m_logHistory.push_back(entry);

        if (m_logHistory.size() > static_cast<size_t>(m_config.maxLogEntries))
        {
            m_logHistory.erase(m_logHistory.begin());
        }
    }
}

void LogSubsystem::LogMessageIf(bool          condition, const String& categoryName, eLogVerbosity verbosity,
                                const String& message, const String&   functionName,
                                const String& fileName, int            lineNumber)
{
    if (condition)
    {
        LogMessage(categoryName, verbosity, message, functionName, fileName, lineNumber);
    }
}

void LogSubsystem::AddOnScreenMessage(const String& message, float displayTime,
                                      const Rgba8&  color, int     uniqueId)
{
    for (auto& device : m_outputDevices)
    {
        auto* onScreenDevice = dynamic_cast<OnScreenOutputDevice*>(device.get());
        if (onScreenDevice)
        {
            onScreenDevice->AddMessage(message, displayTime, color, uniqueId);
        }
    }
}

std::vector<LogEntry> LogSubsystem::GetLogHistory(const String& categoryFilter,
                                                  eLogVerbosity minVerbosity) const
{
    std::lock_guard<std::mutex> lock(m_historyMutex);
    std::vector<LogEntry>       filteredHistory;

    for (const auto& entry : m_logHistory)
    {
        // 分類篩選
        if (!categoryFilter.empty() && entry.m_category != categoryFilter)
        {
            continue;
        }

        // 詳細程度篩選
        if (entry.m_verbosity > minVerbosity)
        {
            continue;
        }

        filteredHistory.push_back(entry);
    }

    return filteredHistory;
}

void LogSubsystem::ClearLogHistory()
{
    std::lock_guard<std::mutex> lock(m_historyMutex);
    m_logHistory.clear();
}

void LogSubsystem::AddOutputDevice(std::unique_ptr<ILogOutputDevice> device)
{
    if (device && device->IsAvailable())
    {
        m_outputDevices.push_back(std::move(device));
    }
}

void LogSubsystem::FlushAllOutputs()
{
    for (auto& device : m_outputDevices)
    {
        device->Flush();
    }
}

String LogSubsystem::GetCurrentTimestamp() const
{
    if (!m_config.timestampEnabled)
    {
        return "";
    }

    auto now    = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms     = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::stringstream ss;
    struct tm         timeinfo;
#ifdef _WIN32
    localtime_s(&timeinfo, &time_t);
#else
    localtime_r(&time_t, &timeinfo);
#endif
    ss << std::put_time(&timeinfo, "%H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();

    return ss.str();
}

String LogSubsystem::GetCurrentThreadId() const
{
    if (!m_config.threadIdEnabled)
    {
        return "";
    }

    std::stringstream ss;
    ss << std::this_thread::get_id();
    return ss.str();
}

void LogSubsystem::ProcessLogQueue()
{
    while (!m_shouldExit.load(std::memory_order_acquire))  // Use explicit memory ordering
    {
        // CRITICAL: Use the SAME mutex for condition variable and queue access
        // This prevents data race when predicate checks m_logQueue.empty()
        std::unique_lock<std::mutex> lock(m_queueMutex);

        // Wait for logs to be available or shutdown signal
        m_logCondition.wait_for(
            lock,
            std::chrono::milliseconds(10),  // Timeout after 10ms to check m_shouldExit
            [this]
            {
                // Predicate: wake up if stopping or if there are logs available
                // Safe to access m_logQueue here because we hold m_queueMutex
                return m_shouldExit.load(std::memory_order_acquire) || !m_logQueue.empty();
            }
        );

        // Check m_shouldExit again after waking up
        if (m_shouldExit.load(std::memory_order_acquire))
        {
            break;
        }

        // Process one log entry if available
        if (!m_logQueue.empty())
        {
            LogEntry entry = m_logQueue.front();
            m_logQueue.pop();
            lock.unlock();  // Unlock before expensive WriteToOutputDevices

            WriteToOutputDevices(entry);
        }
        else
        {
            lock.unlock();
        }
    }

    // Process remaining log entries after shutdown signal
    std::lock_guard<std::mutex> lock(m_queueMutex);
    while (!m_logQueue.empty())
    {
        LogEntry entry = m_logQueue.front();
        m_logQueue.pop();
        WriteToOutputDevices(entry);
    }
}

void LogSubsystem::WriteToOutputDevices(const LogEntry& entry)
{
    LogCategory* category = GetCategory(entry.m_category);
    if (!category)
    {
        return;
    }

    // 檢查輸出目標
    for (auto& device : m_outputDevices)
    {
        bool shouldOutput = false;

        // 判斷裝置類型並檢查是否應該輸出
        if (dynamic_cast<ConsoleOutputDevice*>(device.get()))
        {
            shouldOutput = (category->outputTargets & eLogOutput::Console) != eLogOutput::None;
        }
        else if (dynamic_cast<SmartFileOutputDevice*>(device.get()))
        {
            // SmartFileOutputDevice should receive file output
            shouldOutput = (category->outputTargets & eLogOutput::File) != eLogOutput::None;
        }
        else if (dynamic_cast<FileOutputDevice*>(device.get()))
        {
            shouldOutput = (category->outputTargets & eLogOutput::File) != eLogOutput::None;
        }
        else if (dynamic_cast<DebugOutputDevice*>(device.get()))
        {
            shouldOutput = (category->outputTargets & eLogOutput::DebugOutput) != eLogOutput::None;
        }
        else if (dynamic_cast<OnScreenOutputDevice*>(device.get()))
        {
            shouldOutput = (category->outputTargets & eLogOutput::OnScreen) != eLogOutput::None;
        }
        else if (dynamic_cast<DevConsoleOutputDevice*>(device.get()))
        {
            shouldOutput = (category->outputTargets & eLogOutput::DevConsole) != eLogOutput::None;
        }

        if (shouldOutput && device->IsAvailable())
        {
            device->WriteLog(entry);
        }
    }
}

bool LogSubsystem::ShouldLog(const String& categoryName, eLogVerbosity verbosity) const
{
    auto it = m_categories.find(categoryName);
    if (it == m_categories.end())
    {
        return false;
    }

    const LogCategory& category = it->second;

    // 檢查編譯時詳細程度 (編譯時就決定是否包含)
    if (verbosity > category.compileTimeVerbosity)
    {
        return false;
    }

    // 檢查執行時詳細程度
    if (verbosity > category.defaultVerbosity)
    {
        return false;
    }

    return true;
}


//----------------------------------------------------------------------------------------------------
// 預定義日誌分類
//----------------------------------------------------------------------------------------------------
DAEMON_LOG_CATEGORY(LogTemp, eLogVerbosity::Log, eLogVerbosity::All)

DAEMON_LOG_CATEGORY(LogCore, eLogVerbosity::Log, eLogVerbosity::All)

DAEMON_LOG_CATEGORY(LogRenderer, eLogVerbosity::Log, eLogVerbosity::All)

DAEMON_LOG_CATEGORY(LogAudio, eLogVerbosity::Log, eLogVerbosity::All)

DAEMON_LOG_CATEGORY(LogInput, eLogVerbosity::Log, eLogVerbosity::All)

DAEMON_LOG_CATEGORY(LogNetwork, eLogVerbosity::Log, eLogVerbosity::All)

DAEMON_LOG_CATEGORY(LogResource, eLogVerbosity::Log, eLogVerbosity::All)

DAEMON_LOG_CATEGORY(LogMath, eLogVerbosity::Log, eLogVerbosity::All)

DAEMON_LOG_CATEGORY(LogPlatform, eLogVerbosity::Log, eLogVerbosity::All)

DAEMON_LOG_CATEGORY(LogGame, eLogVerbosity::Log, eLogVerbosity::All)

//----------------------------------------------------------------------------------------------------
// Smart rotation support implementation
//----------------------------------------------------------------------------------------------------

void LogSubsystem::ForceLogRotation()
{
    if (m_isSmartRotationInitialized && m_smartFileDevice)
    {
        m_smartFileDevice->ForceRotation();
        LogMessage("LogRotation", eLogVerbosity::Display, "Smart log rotation forced successfully");
    }
    else
    {
        // Basic implementation - just flush current logs
        FlushAllOutputs();
        LogMessage("LogRotation", eLogVerbosity::Display, "Manual log flush performed (basic rotation mode)");
    }
}

sRotationStats LogSubsystem::GetRotationStats() const
{
    if (m_isSmartRotationInitialized && m_smartFileDevice)
    {
        return m_smartFileDevice->GetStats();
    }
    else
    {
        // Return basic stats
        sRotationStats stats;
        stats.totalRotations    = 0;
        stats.totalFilesDeleted = 0;
        stats.lastError         = "Using basic logging mode - smart rotation not initialized";
        return stats;
    }
}

//----------------------------------------------------------------------------------------------------
// Additional smart rotation methods
//----------------------------------------------------------------------------------------------------

void LogSubsystem::UpdateSmartRotationConfig(const sSmartRotationConfig& config)
{
    m_config.smartRotationConfig = config;
    if (m_isSmartRotationInitialized && m_smartFileDevice)
    {
        m_smartFileDevice->UpdateConfig(config);
        LogMessage("LogRotation", eLogVerbosity::Display, "Smart rotation configuration updated");
    }
}

SmartFileOutputDevice* LogSubsystem::GetSmartFileDevice() const
{
    return m_smartFileDevice;
}

bool LogSubsystem::LoadRotationConfigFromFile(const String& configPath)
{
    try
    {
        std::ifstream configFile(configPath);
        if (!configFile.is_open())
        {
            LogMessage("LogRotation", eLogVerbosity::Warning,
                       Stringf("Could not open rotation config file: %s - using defaults", configPath.c_str()));
            return false;
        }

        nlohmann::json jsonConfig;
        configFile >> jsonConfig;

        // Parse the rotation configuration using sSmartRotationConfig::FromJSON
        m_config.smartRotationConfig = sSmartRotationConfig::FromJSON(jsonConfig);

        LogMessage("LogRotation", eLogVerbosity::Display,
                   Stringf("Loaded rotation configuration from %s", configPath.c_str()));
        return true;
    }
    catch (nlohmann::json::exception const& e)
    {
        LogMessage("LogRotation", eLogVerbosity::Error,
                   Stringf("JSON parsing error in %s: %s - using defaults", configPath.c_str(), e.what()));
        return false;
    }
    catch (std::exception const& e)
    {
        LogMessage("LogRotation", eLogVerbosity::Error,
                   Stringf("Error loading rotation config from %s: %s - using defaults", configPath.c_str(), e.what()));
        return false;
    }
}
