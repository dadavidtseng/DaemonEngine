//----------------------------------------------------------------------------------------------------
// LogSubsystem.cpp
// DaemonEngine 日誌子系統實作
//----------------------------------------------------------------------------------------------------

#include "Engine/Core/LogSubsystem.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/Timer.hpp"

#include <chrono>
#include <iomanip>
#include <sstream>

#include "EngineCommon.hpp"

#ifdef _WIN32
#include <windows.h>
#include <debugapi.h>

// 外部全域變數聲明
extern HANDLE g_consoleHandle;
#endif
//----------------------------------------------------------------------------------------------------
#if defined ERROR
#undef ERROR
#endif
//----------------------------------------------------------------------------------------------------
// 全域變數
//----------------------------------------------------------------------------------------------------
LogSubsystem* g_logSubsystem           = nullptr;
LogSubsystem* LogSubsystem::s_instance = nullptr;

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
LogEntry::LogEntry(const String& cat, eLogVerbosity  verb, const String& msg,
                   const String& func, const String& file, int           line)
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
LogSubsystem::LogSubsystem(sLogSubsystemConfig const& config)
    : m_config(config)
{
    s_instance = this;
}

LogSubsystem::~LogSubsystem()
{
    Shutdown();
    s_instance = nullptr;
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

    if (m_config.enableFile)
    {
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

void LogSubsystem::Update(float deltaTime)
{
    // 更新螢幕輸出裝置
    for (auto& device : m_outputDevices)
    {
        auto* onScreenDevice = dynamic_cast<OnScreenOutputDevice*>(device.get());
        if (onScreenDevice)
        {
            onScreenDevice->Update(deltaTime);
        }
    }
}

LogSubsystem* LogSubsystem::GetInstance()
{
    return s_instance;
}

void LogSubsystem::SetInstance(LogSubsystem* instance)
{
    s_instance = instance;
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

LogCategory* LogSubsystem::GetCategory(const String& categoryName)
{
    auto it = m_categories.find(categoryName);
    return (it != m_categories.end()) ? &it->second : nullptr;
}

bool LogSubsystem::IsCategoryRegistered(const String& categoryName) const
{
    return m_categories.find(categoryName) != m_categories.end();
}

void LogSubsystem::LogMessage(const String& categoryName, eLogVerbosity verbosity, const String& message,
                              const String& functionName, const String& fileName, int            lineNumber)
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
