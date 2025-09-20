//----------------------------------------------------------------------------------------------------
// LogSubsystem.cpp
//----------------------------------------------------------------------------------------------------

#include "Engine/Core/LogSubsystem.hpp"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <thread>
#include <vector>

#include "EngineCommon.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/StringUtils.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// 外部全域變數聲明
extern HANDLE g_consoleHandle;

//----------------------------------------------------------------------------------------------------
#if defined ERROR
#undef ERROR
#endif

LogSubsystem* g_logSubsystem = nullptr;

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
// LogEntry 實作
//----------------------------------------------------------------------------------------------------
LogEntry::LogEntry(String const&       cat,
                   eLogVerbosity const verb,
                   String const&       msg,
                   String const&       func,
                   String const&       file,
                   int const           line)
    : category(cat)
      , verbosity(verb)
      , message(msg)
      , functionName(func)
      , fileName(file)
      , lineNumber(line)
{
    if (g_logSubsystem)
    {
        timestamp = g_logSubsystem->GetCurrentTimestamp();
        threadId  = g_logSubsystem->GetCurrentThreadId();
    }
}

//----------------------------------------------------------------------------------------------------
// ConsoleOutputDevice 實作
//----------------------------------------------------------------------------------------------------
void ConsoleOutputDevice::WriteLog(const LogEntry& entry)
{
    // 使用現有的 Window 控制台控制代碼 (Windows)
#ifdef _WIN32
    HANDLE hConsole = g_consoleHandle ? g_consoleHandle : GetStdHandle(STD_OUTPUT_HANDLE);

    // 檢查控制台是否可用
    if (hConsole && hConsole != INVALID_HANDLE_VALUE)
    {
        WORD                       originalAttributes = 0;
        CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
        if (GetConsoleScreenBufferInfo(hConsole, &consoleInfo))
        {
            originalAttributes = consoleInfo.wAttributes;
        }

        // 根據詳細程度設定顏色
        WORD color = originalAttributes;
        switch (entry.verbosity)
        {
        case eLogVerbosity::Fatal:
            color = BACKGROUND_RED | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
            break;
        case eLogVerbosity::Error:
            color = FOREGROUND_RED | FOREGROUND_INTENSITY;
            break;
        case eLogVerbosity::Warning:
            color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
            break;
        case eLogVerbosity::Display:
            color = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
            break;
        case eLogVerbosity::Log:
            color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE; // 白色
            break;
        case eLogVerbosity::Verbose:
            color = FOREGROUND_BLUE | FOREGROUND_GREEN;
            break;
        case eLogVerbosity::VeryVerbose:
            color = FOREGROUND_BLUE;
            break;
        default:
            color = originalAttributes;
            break;
        }

        SetConsoleTextAttribute(hConsole, color);

        // 格式化輸出（與您原始的 printf 風格一致）
        printf("[%s] [%s] %s\n",
               entry.timestamp.c_str(),
               entry.category.c_str(),
               entry.message.c_str());

        SetConsoleTextAttribute(hConsole, originalAttributes);
    }
    else
    {
        // 如果沒有控制台控制代碼，使用標準輸出
        printf("[%s] [%s] %s\n",
               entry.timestamp.c_str(),
               entry.category.c_str(),
               entry.message.c_str());
    }
#else
    // Unix/Linux 系統使用 ANSI 顏色碼
    const char* colorCode = "\033[0m"; // 預設白色
    switch (entry.verbosity)
    {
    case eLogVerbosity::Fatal: colorCode = "\033[41;37;1m";
        break; // 紅底白字
    case eLogVerbosity::Error: colorCode = "\033[31;1m";
        break;    // 亮紅色
    case eLogVerbosity::Warning: colorCode = "\033[33;1m";
        break;    // 亮黃色
    case eLogVerbosity::Display: colorCode = "\033[32;1m";
        break;    // 亮綠色
    case eLogVerbosity::Log: colorCode = "\033[37m";
        break;      // 白色
    case eLogVerbosity::Verbose: colorCode = "\033[36m";
        break;      // 青色
    case eLogVerbosity::VeryVerbose: colorCode = "\033[34m";
        break;     // 藍色
    }

    printf("%s[%s] [%s] %s\033[0m\n",
           colorCode,
           entry.timestamp.c_str(),
           entry.category.c_str(),
           entry.message.c_str());
#endif
}

Rgba8 ConsoleOutputDevice::GetVerbosityColor(eLogVerbosity verbosity) const
{
    switch (verbosity)
    {
    case eLogVerbosity::Fatal: return Rgba8::RED;
    case eLogVerbosity::Error: return Rgba8(255, 100, 100, 255);
    case eLogVerbosity::Warning: return Rgba8::YELLOW;
    case eLogVerbosity::Display: return Rgba8::GREEN;
    case eLogVerbosity::Log: return Rgba8::WHITE;
    case eLogVerbosity::Verbose: return Rgba8(200, 200, 200, 255);
    case eLogVerbosity::VeryVerbose: return Rgba8(150, 150, 150, 255);
    default: return Rgba8::WHITE;
    }
}

//----------------------------------------------------------------------------------------------------
// FileOutputDevice 實作
//----------------------------------------------------------------------------------------------------
FileOutputDevice::FileOutputDevice(const String& filePath)
    : m_filePath(filePath)
{
    // 嘗試直接開啟檔案
    m_logFile.open(filePath, std::ios::out | std::ios::app);

    if (!m_logFile.is_open())
    {
        // 如果是 Logs/ 開頭的路徑，嘗試先建立目錄
        if (filePath.find("Logs/") == 0 || filePath.find("Logs\\") == 0)
        {
#ifdef _WIN32
            CreateDirectoryA("Logs", nullptr);
#else
            system("mkdir -p Logs");
#endif
            // 再次嘗試開啟原始路徑
            m_logFile.open(filePath, std::ios::out | std::ios::app);
        }

        // 如果還是失敗，在當前目錄建立
        if (!m_logFile.is_open())
        {
            m_logFile.open("DaemonEngine.log", std::ios::out | std::ios::app);
        }
    }
}

FileOutputDevice::~FileOutputDevice()
{
    if (m_logFile.is_open())
    {
        m_logFile.close();
    }
}

void FileOutputDevice::WriteLog(const LogEntry& entry)
{
    std::lock_guard lock(m_fileMutex);

    if (m_logFile.is_open())
    {
        m_logFile << "[" << entry.timestamp << "] "
        << "[" << entry.threadId << "] "
        << "[" << entry.category << "] ";

        // 轉換詳細程度為字串
        const char* verbosityStr = "Unknown";
        switch (entry.verbosity)
        {
        case eLogVerbosity::Fatal: verbosityStr = "Fatal";
            break;
        case eLogVerbosity::Error: verbosityStr = "Error";
            break;
        case eLogVerbosity::Warning: verbosityStr = "Warning";
            break;
        case eLogVerbosity::Display: verbosityStr = "Display";
            break;
        case eLogVerbosity::Log: verbosityStr = "Log";
            break;
        case eLogVerbosity::Verbose: verbosityStr = "Verbose";
            break;
        case eLogVerbosity::VeryVerbose: verbosityStr = "VeryVerbose";
            break;
        }

        m_logFile << "[" << verbosityStr << "] " << entry.message;

        // 如果有檔案和行號資訊，也記錄下來
        if (!entry.fileName.empty() && entry.lineNumber > 0)
        {
            m_logFile << " (" << entry.fileName << ":" << entry.lineNumber << ")";
        }

        m_logFile << std::endl;
    }
}

void FileOutputDevice::Flush()
{
    std::lock_guard<std::mutex> lock(m_fileMutex);
    if (m_logFile.is_open())
    {
        m_logFile.flush();
    }
}

bool FileOutputDevice::IsAvailable() const
{
    return m_logFile.is_open();
}

//----------------------------------------------------------------------------------------------------
// DebugOutputDevice 實作
//----------------------------------------------------------------------------------------------------
void DebugOutputDevice::WriteLog(const LogEntry& entry)
{
#ifdef _WIN32
    String outputString = "[" + entry.category + "] " + entry.message + "\n";
    OutputDebugStringA(outputString.c_str());
#endif
}

bool DebugOutputDevice::IsAvailable() const
{
#ifdef _WIN32
    return IsDebuggerPresent();
#else
    return false;
#endif
}

//----------------------------------------------------------------------------------------------------
// OnScreenOutputDevice 實作
//----------------------------------------------------------------------------------------------------
void OnScreenOutputDevice::WriteLog(const LogEntry& entry)
{
    Rgba8 color;
    switch (entry.verbosity)
    {
    case eLogVerbosity::Fatal: color = Rgba8::RED;
        break;
    case eLogVerbosity::Error: color = Rgba8(255, 100, 100, 255);
        break;
    case eLogVerbosity::Warning: color = Rgba8::YELLOW;
        break;
    case eLogVerbosity::Display: color = Rgba8::GREEN;
        break;
    default: color = Rgba8::WHITE;
        break;
    }

    AddMessage("[" + entry.category + "] " + entry.message, 5.0f, color);
}

void OnScreenOutputDevice::Update(float deltaTime)
{
    std::lock_guard<std::mutex> lock(m_messagesMutex);

    for (auto it = m_messages.begin(); it != m_messages.end();)
    {
        it->remainingTime -= deltaTime;
        if (it->remainingTime <= 0.0f)
        {
            it = m_messages.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void OnScreenOutputDevice::AddMessage(const String& message, float displayTime, const Rgba8& color, int uniqueId)
{
    std::lock_guard<std::mutex> lock(m_messagesMutex);

    // 如果指定了唯一 ID，先移除舊的相同 ID 訊息
    if (uniqueId >= 0)
    {
        m_messages.erase(
            std::remove_if(m_messages.begin(), m_messages.end(),
                           [uniqueId](const OnScreenMessage& msg) { return msg.uniqueId == uniqueId; }),
            m_messages.end());
    }

    OnScreenMessage newMessage;
    newMessage.message       = message;
    newMessage.displayTime   = displayTime;
    newMessage.remainingTime = displayTime;
    newMessage.color         = color;
    newMessage.uniqueId      = uniqueId >= 0 ? uniqueId : m_nextUniqueId++;

    m_messages.push_back(newMessage);
}

void OnScreenOutputDevice::ClearMessages()
{
    std::lock_guard<std::mutex> lock(m_messagesMutex);
    m_messages.clear();
}

void OnScreenOutputDevice::RenderMessages()
{
    // 這個方法需要與 DaemonEngine 的 Renderer 整合
    // 在實際使用時，需要呼叫 g_theRenderer 來渲染文字
    // 這裡只是提供介面
}

//----------------------------------------------------------------------------------------------------
// DevConsoleOutputDevice 實作
//----------------------------------------------------------------------------------------------------
void DevConsoleOutputDevice::WriteLog(const LogEntry& entry)
{
    if (g_devConsole)
    {
        // 根據詳細程度選擇適當的顏色類型
        Rgba8 lineType = DevConsole::INFO_MINOR;
        switch (entry.verbosity)
        {
        case eLogVerbosity::Fatal:
        case eLogVerbosity::Error:
            lineType = DevConsole::ERROR;
            break;
        case eLogVerbosity::Warning:
            lineType = DevConsole::WARNING;
            break;
        case eLogVerbosity::Display:
            lineType = DevConsole::INFO_MAJOR;
            break;
        default:
            lineType = DevConsole::INFO_MINOR;
            break;
        }

        g_devConsole->AddLine(lineType, "[" + entry.category + "] " + entry.message);
    }
}

bool DevConsoleOutputDevice::IsAvailable() const
{
    return g_devConsole != nullptr;
}

//----------------------------------------------------------------------------------------------------
// LogSubsystem 實作
//----------------------------------------------------------------------------------------------------
LogSubsystem::LogSubsystem(sLogSubsystemConfig config)
    : m_config(std::move(config)),
      m_smartFileDevice(nullptr),
      m_isSmartRotationInitialized(false)
{
}

LogSubsystem::~LogSubsystem()
{
    Shutdown();
}

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
            auto smartDevice = std::make_unique<SmartFileOutputDevice>(
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

    // 停止非同步日誌執行緒
    if (m_logThread.joinable())
    {
        m_shouldExit = true;
        m_logThread.join();
    }

    // 清理輸出裝置
    FlushAllOutputs();
    m_outputDevices.clear();

    // 清理日誌歷史
    ClearLogHistory();
    m_categories.clear();

    LogMessage("LogCore", eLogVerbosity::Display, "LogSubsystem::Shutdown() finish");
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
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_logQueue.push(entry);
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
        if (!categoryFilter.empty() && entry.category != categoryFilter)
        {
            continue;
        }

        // 詳細程度篩選
        if (entry.verbosity > minVerbosity)
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

void LogSubsystem::ProcessLogQueue()
{
    while (!m_shouldExit)
    {
        std::unique_lock<std::mutex> lock(m_queueMutex);

        if (!m_logQueue.empty())
        {
            LogEntry entry = m_logQueue.front();
            m_logQueue.pop();
            lock.unlock();

            WriteToOutputDevices(entry);
        }
        else
        {
            lock.unlock();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    // 處理剩餘的日誌條目
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
    LogCategory* category = GetCategory(entry.category);
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
    // For now, return true to indicate basic configuration loading
    // In a full implementation, this would parse JSON configuration
    LogMessage("LogRotation", eLogVerbosity::Display, Stringf("Configuration loading from %s - using defaults", configPath.c_str()));
    return true;
}

//----------------------------------------------------------------------------------------------------
// SmartFileOutputDevice implementation - Minecraft-style log rotation
//----------------------------------------------------------------------------------------------------

SmartFileOutputDevice::SmartFileOutputDevice(const String& logDirectory, const sSmartRotationConfig& config)
    : m_logDirectory(logDirectory)
      , m_config(config)
      , m_currentSegmentNumber(1)
      , m_currentFileSize(0)
      , m_sessionStartTime(std::chrono::system_clock::now())
      , m_lastRotationTime(std::chrono::system_clock::now())
{
    // Generate session ID (timestamp-based, Minecraft-style)
    m_sessionId = GenerateSessionId();

    // Create log directory if needed
    CreateDirectoryIfNeeded(m_logDirectory);

    // Set up current log file (latest.log)
    m_currentFilePath = m_logDirectory / m_config.currentLogName;

    // Rotate existing latest.log if it exists (application startup rotation)
    if (std::filesystem::exists(m_currentFilePath))
    {
        ArchiveCurrentFile();
    }

    // Open new latest.log for current session
    m_currentFile.open(m_currentFilePath, std::ios::out | std::ios::trunc);
    if (!m_currentFile.is_open())
    {
        throw std::runtime_error("Failed to create latest.log file");
    }

    // Start background rotation thread
    m_rotationThread = std::thread(&SmartFileOutputDevice::RotationThreadMain, this);

    LogRotationEvent("SmartFileOutputDevice initialized - Minecraft-style rotation active");
}

SmartFileOutputDevice::~SmartFileOutputDevice()
{
    // Stop background thread
    m_shouldStop = true;
    if (m_rotationThread.joinable())
    {
        m_rotationThread.join();
    }

    // Close current file
    if (m_currentFile.is_open())
    {
        m_currentFile.close();
    }

    // Perform final rotation if needed
    if (m_currentFileSize > 0)
    {
        ArchiveCurrentFile();
    }
}

//----------------------------------------------------------------------------------------------------
// Core SmartFileOutputDevice methods
//----------------------------------------------------------------------------------------------------

void SmartFileOutputDevice::WriteLog(const LogEntry& entry)
{
    std::lock_guard<std::mutex> lock(m_fileMutex);

    if (!m_currentFile.is_open())
    {
        return; // Fail silently if file not available
    }

    // Format log entry (similar to regular FileOutputDevice)
    String formattedEntry = Stringf("[%s][%s][%s:%d] %s\n",
                                    entry.timestamp.c_str(),
                                    entry.category.c_str(),
                                    entry.fileName.c_str(),
                                    entry.lineNumber,
                                    entry.message.c_str()
    );

    // Write to current file
    m_currentFile << formattedEntry;
    m_currentFile.flush();

    // Track file size
    m_currentFileSize += formattedEntry.length();

    // Check if rotation is needed
    if (ShouldRotateBySize() || ShouldRotateByTime())
    {
        m_rotationPending = true;
    }
}

void SmartFileOutputDevice::Flush()
{
    std::lock_guard<std::mutex> lock(m_fileMutex);
    if (m_currentFile.is_open())
    {
        m_currentFile.flush();
    }
}

bool SmartFileOutputDevice::IsAvailable() const
{
    std::lock_guard<std::mutex> lock(m_fileMutex);
    return m_currentFile.is_open();
}

//----------------------------------------------------------------------------------------------------
// Rotation control methods
//----------------------------------------------------------------------------------------------------

void SmartFileOutputDevice::ForceRotation()
{
    std::lock_guard<std::mutex> lock(m_rotationMutex);
    PerformRotation();
}

bool SmartFileOutputDevice::ShouldRotateBySize() const
{
    return m_currentFileSize >= m_config.maxFileSizeBytes;
}

bool SmartFileOutputDevice::ShouldRotateByTime() const
{
    auto now                   = std::chrono::system_clock::now();
    auto timeSinceLastRotation = std::chrono::duration_cast<std::chrono::hours>(now - m_lastRotationTime);
    return timeSinceLastRotation >= m_config.maxTimeInterval;
}

void SmartFileOutputDevice::UpdateConfig(const sSmartRotationConfig& config)
{
    std::lock_guard<std::mutex> lock(m_rotationMutex);
    m_config = config;
}

//----------------------------------------------------------------------------------------------------
// Internal rotation logic
//----------------------------------------------------------------------------------------------------

void SmartFileOutputDevice::PerformRotation()
{
    if (m_currentFileSize == 0)
    {
        return; // Nothing to rotate
    }

    // Close current file
    {
        std::lock_guard<std::mutex> fileLock(m_fileMutex);
        if (m_currentFile.is_open())
        {
            m_currentFile.close();
        }
    }

    // Archive the current file
    ArchiveCurrentFile();

    // Increment segment number for next rotation
    m_currentSegmentNumber++;

    // Open new latest.log
    {
        std::lock_guard<std::mutex> fileLock(m_fileMutex);
        m_currentFile.open(m_currentFilePath, std::ios::out | std::ios::trunc);
        m_currentFileSize  = 0;
        m_lastRotationTime = std::chrono::system_clock::now();
    }

    // Update statistics
    m_stats.totalRotations++;

    // Perform cleanup if needed
    PerformRetentionCleanup();

    LogRotationEvent(Stringf("Log rotation completed - segment %d", m_currentSegmentNumber));
}

void SmartFileOutputDevice::RotationThreadMain()
{
    while (!m_shouldStop)
    {
        // Check for pending rotation
        if (m_rotationPending)
        {
            std::lock_guard<std::mutex> lock(m_rotationMutex);
            PerformRotation();
            m_rotationPending = false;
        }

        // Check disk space periodically
        double availableSpaceGB = GetAvailableDiskSpaceGB();
        if (availableSpaceGB < m_config.diskSpaceEmergencyGB)
        {
            EmergencyCleanup();
        }

        // Sleep for a short period
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

//----------------------------------------------------------------------------------------------------
// File management methods
//----------------------------------------------------------------------------------------------------

std::filesystem::path SmartFileOutputDevice::GenerateNewLogFilePath()
{
    // Date-based folder organization: Logs/YYYY-MM-DD/session-HHMMSS-segXXX.log
    std::filesystem::path dateFolderPath = GetDateBasedFolderPath();

    String filename = Stringf("%s-%s-seg%03d.log",
                              m_config.sessionPrefix.c_str(),
                              GetTimeOnlySessionId().c_str(),
                              m_currentSegmentNumber
    );

    return dateFolderPath / filename;
}

String SmartFileOutputDevice::GenerateSessionId()
{
    auto now    = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
#ifdef _WIN32
    // Use secure version on Windows
    struct tm timeinfo;
    localtime_s(&timeinfo, &time_t);
    ss << std::put_time(&timeinfo, "%Y-%m-%d-%H%M%S");
#else
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d-%H%M%S");
#endif
    return ss.str();
}

void SmartFileOutputDevice::ArchiveCurrentFile()
{
    if (!std::filesystem::exists(m_currentFilePath))
    {
        return; // Nothing to archive
    }

    // Generate archive file path with date-based folder organization
    std::filesystem::path archivePath = GenerateNewLogFilePath();

    try
    {
        // Create date-based directory if needed
        CreateDirectoryIfNeeded(archivePath.parent_path());

        // Simply move the file to the date-based folder
        std::filesystem::rename(m_currentFilePath, archivePath);

        LogRotationEvent(Stringf("Archived log file: %s", archivePath.string().c_str()));
    }
    catch (const std::exception& e)
    {
        m_stats.lastError = e.what();
        LogRotationEvent(Stringf("Archive failed: %s", e.what()));
    }
}

std::filesystem::path SmartFileOutputDevice::GetDateBasedFolderPath() const
{
    if (!m_config.organizeDateFolders)
    {
        return m_logDirectory; // Use base directory if date organization is disabled
    }

    // Generate date folder path: Logs/YYYY-MM-DD
    auto now    = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    struct tm         timeinfo;
#ifdef _WIN32
    localtime_s(&timeinfo, &time_t);
#else
    localtime_r(&time_t, &timeinfo);
#endif
    ss << std::put_time(&timeinfo, "%Y-%m-%d");

    return m_logDirectory / ss.str();
}

String SmartFileOutputDevice::GetTimeOnlySessionId() const
{
    // Extract time portion from session ID (remove date prefix)
    // Original format: YYYY-MM-DD-HHMMSS -> HHMMSS
    size_t lastDashPos = m_sessionId.find_last_of('-');
    if (lastDashPos != String::npos && lastDashPos < m_sessionId.length() - 1)
    {
        return m_sessionId.substr(lastDashPos + 1);
    }
    return m_sessionId; // Fallback to full session ID
}

//----------------------------------------------------------------------------------------------------
// Cleanup and maintenance methods
//----------------------------------------------------------------------------------------------------

void SmartFileOutputDevice::PerformRetentionCleanup()
{
    try
    {
        auto oldLogs = ScanForOldLogs();

        // Remove logs older than retention period
        auto now = std::chrono::system_clock::now();
        for (const auto& logPath : oldLogs)
        {
            try
            {
                auto lastWrite = std::filesystem::last_write_time(logPath);
                auto sctp      = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                    lastWrite - std::filesystem::file_time_type::clock::now() + now);
                auto fileAge = std::chrono::duration_cast<std::chrono::hours>(now - sctp);

                if (fileAge > m_config.retentionHours)
                {
                    std::filesystem::remove(logPath);
                    m_stats.totalFilesDeleted++;
                    LogRotationEvent(Stringf("Deleted old log: %s", logPath.filename().string().c_str()));
                }
            }
            catch (const std::exception&)
            {
                // Skip files that can't be processed
                continue;
            }
        }

        // Enforce maximum number of archived files
        if (oldLogs.size() > m_config.maxArchivedFiles)
        {
            // Sort by modification time (oldest first)
            std::sort(oldLogs.begin(), oldLogs.end(), [](const auto& a, const auto& b) {
                return std::filesystem::last_write_time(a) < std::filesystem::last_write_time(b);
            });

            // Remove excess files
            size_t filesToDelete = oldLogs.size() - m_config.maxArchivedFiles;
            for (size_t i = 0; i < filesToDelete; ++i)
            {
                std::filesystem::remove(oldLogs[i]);
                m_stats.totalFilesDeleted++;
                LogRotationEvent(Stringf("Deleted excess log: %s", oldLogs[i].filename().string().c_str()));
            }
        }
    }
    catch (const std::exception& e)
    {
        m_stats.lastError = e.what();
        LogRotationEvent(Stringf("Cleanup failed: %s", e.what()));
    }
}

void SmartFileOutputDevice::EmergencyCleanup()
{
    LogRotationEvent("Emergency disk space cleanup initiated");

    try
    {
        auto oldLogs = ScanForOldLogs();

        // Delete up to 50% of old log files in emergency
        size_t maxFiles      = oldLogs.size();
        size_t halfFiles     = maxFiles / 2;
        size_t filesToDelete = (halfFiles < maxFiles) ? halfFiles : maxFiles;

        // Sort by modification time (oldest first)
        std::sort(oldLogs.begin(), oldLogs.end(), [](const auto& a, const auto& b) {
            return std::filesystem::last_write_time(a) < std::filesystem::last_write_time(b);
        });

        for (size_t i = 0; i < filesToDelete; ++i)
        {
            std::filesystem::remove(oldLogs[i]);
            m_stats.totalFilesDeleted++;
        }

        LogRotationEvent(Stringf("Emergency cleanup: deleted %d log files", static_cast<int>(filesToDelete)));
    }
    catch (const std::exception& e)
    {
        m_stats.lastError = e.what();
        LogRotationEvent(Stringf("Emergency cleanup failed: %s", e.what()));
    }
}

std::vector<std::filesystem::path> SmartFileOutputDevice::ScanForOldLogs() const
{
    std::vector<std::filesystem::path> logFiles;

    try
    {
        if (m_config.organizeDateFolders)
        {
            // Scan date-based subfolders
            for (const auto& entry : std::filesystem::directory_iterator(m_logDirectory))
            {
                if (entry.is_directory())
                {
                    // Check if it's a date folder (YYYY-MM-DD format)
                    String folderName = entry.path().filename().string();
                    if (folderName.length() == 10 && folderName[4] == '-' && folderName[7] == '-')
                    {
                        // Scan files in date folder
                        try
                        {
                            for (const auto& logEntry : std::filesystem::directory_iterator(entry.path()))
                            {
                                if (logEntry.is_regular_file())
                                {
                                    String filename = logEntry.path().filename().string();
                                    if (filename.find(m_config.sessionPrefix.c_str()) == 0)
                                    {
                                        logFiles.push_back(logEntry.path());
                                    }
                                }
                            }
                        }
                        catch (const std::exception&)
                        {
                            // Skip folders that can't be read
                            continue;
                        }
                    }
                }
            }
        }
        else
        {
            // Scan base directory (non-date organized mode)
            for (const auto& entry : std::filesystem::directory_iterator(m_logDirectory))
            {
                if (entry.is_regular_file())
                {
                    const auto& path     = entry.path();
                    String      filename = path.filename().string();

                    // Skip current latest.log
                    if (filename == m_config.currentLogName.c_str())
                    {
                        continue;
                    }

                    // Check if it's a session log file (contains session prefix)
                    if (filename.find(m_config.sessionPrefix.c_str()) == 0)
                    {
                        logFiles.push_back(path);
                    }
                }
            }
        }
    }
    catch (const std::exception& e)
    {
		UNUSED(e)
        // Log scan failed, return empty vector
    }

    return logFiles;
}

//----------------------------------------------------------------------------------------------------
// Utility functions
//----------------------------------------------------------------------------------------------------

bool SmartFileOutputDevice::CreateDirectoryIfNeeded(const std::filesystem::path& path)
{
    try
    {
        if (!std::filesystem::exists(path))
        {
            std::filesystem::create_directories(path);
            LogRotationEvent(Stringf("Created log directory: %s", path.string().c_str()));
            return true;
        }
        return true;
    }
    catch (const std::exception& e)
    {
        LogRotationEvent(Stringf("Failed to create directory: %s", e.what()));
        return false;
    }
}

void SmartFileOutputDevice::LogRotationEvent(const String& message)
{
    UNUSED(message)
    // Simple logging to console/debug output
    // In a full implementation, this would use the parent LogSubsystem
#ifdef _DEBUG
    OutputDebugStringA(Stringf("[SmartRotation] %s\n", message.c_str()).c_str());
#endif
}

double SmartFileOutputDevice::GetAvailableDiskSpaceGB() const
{
    try
    {
        std::filesystem::space_info const spaceInfo = std::filesystem::space(m_logDirectory);
        return static_cast<double>(spaceInfo.available) / (1024.0 * 1024.0 * 1024.0);
    }
    catch (std::exception const&)
    {
        return 100.0; // Return a safe default if space check fails
    }
}
