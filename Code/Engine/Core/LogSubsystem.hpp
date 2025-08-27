//----------------------------------------------------------------------------------------------------
// LogSubsystem.hpp
// DaemonEngine 日誌子系統 - 基於 Unreal Engine 日誌系統設計
//----------------------------------------------------------------------------------------------------

#pragma once

#include <functional>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/Rgba8.hpp"

//----------------------------------------------------------------------------------------------------
// 日誌詳細程度層級 (按照 UE 的方式定義)
//----------------------------------------------------------------------------------------------------
enum class eLogVerbosity : int8_t
{
    NoLogging = 0,    // 不記錄任何日誌
    Fatal,            // 致命錯誤，會導致程式崩潰
    Error,            // 錯誤但程式可以繼續執行
    Warning,          // 警告
    Display,          // 顯示給使用者的重要訊息
    Log,              // 一般日誌訊息
    Verbose,          // 詳細資訊
    VeryVerbose,      // 非常詳細的資訊

    All = VeryVerbose // 所有層級
};

//----------------------------------------------------------------------------------------------------
// 日誌輸出目標
//----------------------------------------------------------------------------------------------------
enum class eLogOutput : uint8_t
{
    None         = 0,
    Console      = 1 << 0,  // 輸出到控制台
    File         = 1 << 1,  // 輸出到檔案
    DebugOutput  = 1 << 2,  // 輸出到 Visual Studio 輸出視窗
    OnScreen     = 1 << 3,  // 輸出到螢幕上 (類似 UE 的 AddOnScreenDebugMessage)
    DevConsole   = 1 << 4,  // 輸出到 DaemonEngine 的開發者控制台

    All = Console | File | DebugOutput | OnScreen | DevConsole
};

// 位元操作符重載
inline eLogOutput operator|(eLogOutput a, eLogOutput b)
{
    return static_cast<eLogOutput>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

inline eLogOutput operator&(eLogOutput a, eLogOutput b)
{
    return static_cast<eLogOutput>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

//----------------------------------------------------------------------------------------------------
// 日誌分類結構
//----------------------------------------------------------------------------------------------------
struct LogCategory
{
    String       name;                                    // 分類名稱
    eLogVerbosity defaultVerbosity   = eLogVerbosity::Log; // 預設詳細程度
    eLogVerbosity compileTimeVerbosity = eLogVerbosity::All; // 編譯時最大詳細程度
    eLogOutput   outputTargets       = eLogOutput::All;   // 輸出目標

    LogCategory() = default;
    LogCategory(const String& categoryName,
                eLogVerbosity defaultVerb = eLogVerbosity::Log,
                eLogVerbosity compileVerb = eLogVerbosity::All,
                eLogOutput outputs = eLogOutput::All)
        : name(categoryName)
        , defaultVerbosity(defaultVerb)
        , compileTimeVerbosity(compileVerb)
        , outputTargets(outputs)
    {
    }
};

//----------------------------------------------------------------------------------------------------
// 日誌條目結構
//----------------------------------------------------------------------------------------------------
struct LogEntry
{
    String       category;      // 日誌分類
    eLogVerbosity verbosity;    // 詳細程度
    String       message;       // 日誌訊息
    String       timestamp;     // 時間戳記
    String       threadId;      // 執行緒 ID
    String       functionName;  // 函數名稱
    String       fileName;      // 檔案名稱
    int          lineNumber;    // 行號

    LogEntry() = default;
    LogEntry(const String& cat, eLogVerbosity verb, const String& msg,
             const String& func = "", const String& file = "", int line = 0);
};

//----------------------------------------------------------------------------------------------------
// 日誌輸出裝置介面 (類似 UE 的 FOutputDevice)
//----------------------------------------------------------------------------------------------------
class ILogOutputDevice
{
public:
    virtual ~ILogOutputDevice() = default;
    virtual void WriteLog(const LogEntry& entry) = 0;
    virtual void Flush() {}
    virtual bool IsAvailable() const { return true; }
};

//----------------------------------------------------------------------------------------------------
// 控制台輸出裝置
//----------------------------------------------------------------------------------------------------
class ConsoleOutputDevice : public ILogOutputDevice
{
public:
    void WriteLog(const LogEntry& entry) override;
    Rgba8 GetVerbosityColor(eLogVerbosity verbosity) const;
};

//----------------------------------------------------------------------------------------------------
// 檔案輸出裝置
//----------------------------------------------------------------------------------------------------
class FileOutputDevice : public ILogOutputDevice
{
private:
    std::ofstream m_logFile;
    String m_filePath;
    mutable std::mutex m_fileMutex;

public:
    explicit FileOutputDevice(const String& filePath);
    ~FileOutputDevice();

    void WriteLog(const LogEntry& entry) override;
    void Flush() override;
    bool IsAvailable() const override;
};

//----------------------------------------------------------------------------------------------------
// Visual Studio 輸出視窗裝置
//----------------------------------------------------------------------------------------------------
class DebugOutputDevice : public ILogOutputDevice
{
public:
    void WriteLog(const LogEntry& entry) override;
    bool IsAvailable() const override;
};

//----------------------------------------------------------------------------------------------------
// 螢幕輸出裝置
//----------------------------------------------------------------------------------------------------
class OnScreenOutputDevice : public ILogOutputDevice
{
private:
    struct OnScreenMessage
    {
        String message;
        float displayTime;
        float remainingTime;
        Rgba8 color;
        int uniqueId;
    };

    std::vector<OnScreenMessage> m_messages;
    mutable std::mutex m_messagesMutex;
    int m_nextUniqueId = 0;

public:
    void WriteLog(const LogEntry& entry) override;
    void Update(float deltaTime);
    void RenderMessages(); // 需要與 Renderer 整合
    void AddMessage(const String& message, float displayTime, const Rgba8& color, int uniqueId = -1);
    void ClearMessages();
};

//----------------------------------------------------------------------------------------------------
// DaemonEngine 開發者控制台輸出裝置
//----------------------------------------------------------------------------------------------------
class DevConsoleOutputDevice : public ILogOutputDevice
{
public:
    void WriteLog(const LogEntry& entry) override;
    bool IsAvailable() const override;
};

//----------------------------------------------------------------------------------------------------
// 日誌子系統設定
//----------------------------------------------------------------------------------------------------
struct sLogSubsystemConfig
{
    String       logFilePath      = "Logs/DaemonEngine.log";  // 日誌檔案路徑
    bool         enableConsole    = true;                     // 啟用控制台輸出
    bool         enableFile       = true;                     // 啟用檔案輸出
    bool         enableDebugOut   = true;                     // 啟用除錯輸出
    bool         enableOnScreen   = true;                     // 啟用螢幕輸出
    bool         enableDevConsole = true;                     // 啟用開發者控制台輸出
    bool         asyncLogging     = true;                     // 啟用非同步日誌
    int          maxLogEntries    = 10000;                    // 記憶體中最大日誌條目數
    bool         timestampEnabled = true;                     // 啟用時間戳記
    bool         threadIdEnabled  = true;                     // 啟用執行緒 ID
    bool         autoFlush        = false;                    // 自動重新整理輸出
};

//----------------------------------------------------------------------------------------------------
// 日誌子系統主類別
//----------------------------------------------------------------------------------------------------
class LogSubsystem
{
private:
    sLogSubsystemConfig m_config;
    std::unordered_map<String, LogCategory> m_categories;
    std::vector<std::unique_ptr<ILogOutputDevice>> m_outputDevices;

    // 非同步日誌相關
    std::queue<LogEntry> m_logQueue;
    mutable std::mutex m_queueMutex;
    std::thread m_logThread;
    std::atomic<bool> m_shouldExit{false};

    // 記憶體中的日誌歷史
    std::vector<LogEntry> m_logHistory;
    mutable std::mutex m_historyMutex;

    // 單例模式
    static LogSubsystem* s_instance;

public:
    explicit LogSubsystem(const sLogSubsystemConfig& config = sLogSubsystemConfig{});
    ~LogSubsystem();

    // 子系統生命週期
    void Startup();
    void Shutdown();
    void BeginFrame();
    void EndFrame();
    void Update(float deltaTime);

    // 單例存取
    static LogSubsystem* GetInstance();
    static void SetInstance(LogSubsystem* instance);

    // 日誌分類管理
    void RegisterCategory(const String& categoryName,
                         eLogVerbosity defaultVerbosity = eLogVerbosity::Log,
                         eLogVerbosity compileTimeVerbosity = eLogVerbosity::All,
                         eLogOutput outputTargets = eLogOutput::All);
    void SetCategoryVerbosity(const String& categoryName, eLogVerbosity verbosity);
    LogCategory* GetCategory(const String& categoryName);
    bool IsCategoryRegistered(const String& categoryName) const;

    // 日誌記錄
    void LogMessage(const String& categoryName, eLogVerbosity verbosity, const String& message,
                   const String& functionName = "", const String& fileName = "", int lineNumber = 0);

    // 條件日誌記錄
    void LogMessageIf(bool condition, const String& categoryName, eLogVerbosity verbosity,
                     const String& message, const String& functionName = "",
                     const String& fileName = "", int lineNumber = 0);

    // 格式化日誌記錄 (支援 printf 風格)
    template<typename... Args>
    void LogFormatted(const String& categoryName, eLogVerbosity verbosity,
                     const String& format, Args... args)
    {
        if (!ShouldLog(categoryName, verbosity)) {
            return;
        }

        String formattedMessage = Stringf(format.c_str(), args...);
        LogMessage(categoryName, verbosity, formattedMessage);
    }

    // 螢幕訊息 (類似 UE 的 AddOnScreenDebugMessage)
    void AddOnScreenMessage(const String& message, float displayTime = 5.0f,
                           const Rgba8& color = Rgba8::WHITE, int uniqueId = -1);

    // 日誌歷史存取
    std::vector<LogEntry> GetLogHistory(const String& categoryFilter = "",
                                       eLogVerbosity minVerbosity = eLogVerbosity::NoLogging) const;
    void ClearLogHistory();

    // 輸出裝置管理
    void AddOutputDevice(std::unique_ptr<ILogOutputDevice> device);
    void FlushAllOutputs();

    // 設定存取
    const sLogSubsystemConfig& GetConfig() const { return m_config; }
    void SetConfig(const sLogSubsystemConfig& config) { m_config = config; }

    String GetCurrentTimestamp() const;
    String GetCurrentThreadId() const;

private:
    void ProcessLogQueue();
    void WriteToOutputDevices(const LogEntry& entry);


    bool ShouldLog(const String& categoryName, eLogVerbosity verbosity) const;
};

//----------------------------------------------------------------------------------------------------
// 全域日誌子系統指標
//----------------------------------------------------------------------------------------------------
extern LogSubsystem* g_logSubsystem;

//----------------------------------------------------------------------------------------------------
// 日誌巨集定義 (類似 UE 的 UE_LOG)
//----------------------------------------------------------------------------------------------------
#define DAEMON_LOG_CATEGORY_EXTERN(CategoryName, DefaultVerbosity, CompileTimeVerbosity) \
    extern LogCategory LogCategory##CategoryName;

#define DAEMON_LOG_CATEGORY(CategoryName, DefaultVerbosity, CompileTimeVerbosity) \
    LogCategory LogCategory##CategoryName(#CategoryName, DefaultVerbosity, CompileTimeVerbosity);

// 基本日誌巨集
#define DAEMON_LOG(CategoryName, Verbosity, Format, ...) \
    do { \
        if (g_logSubsystem && g_logSubsystem->IsCategoryRegistered(#CategoryName)) { \
            g_logSubsystem->LogFormatted(#CategoryName, Verbosity, Format, ##__VA_ARGS__); \
        } \
    } while(0)

// 條件日誌巨集
#define DAEMON_LOG_IF(Condition, CategoryName, Verbosity, Format, ...) \
    do { \
        if (Condition && g_logSubsystem && g_logSubsystem->IsCategoryRegistered(#CategoryName)) { \
            g_logSubsystem->LogFormatted(#CategoryName, Verbosity, Format, ##__VA_ARGS__); \
        } \
    } while(0)

// 螢幕訊息巨集 (類似 UE 的 AddOnScreenDebugMessage)
#define DAEMON_ON_SCREEN_MESSAGE(Message, DisplayTime, Color) \
    do { \
        if (g_logSubsystem) { \
            g_logSubsystem->AddOnScreenMessage(Message, DisplayTime, Color); \
        } \
    } while(0)

//----------------------------------------------------------------------------------------------------
// 預定義日誌分類
//----------------------------------------------------------------------------------------------------
DAEMON_LOG_CATEGORY_EXTERN(LogTemp, eLogVerbosity::Log, eLogVerbosity::All)
DAEMON_LOG_CATEGORY_EXTERN(LogCore, eLogVerbosity::Log, eLogVerbosity::All)
DAEMON_LOG_CATEGORY_EXTERN(LogRenderer, eLogVerbosity::Log, eLogVerbosity::All)
DAEMON_LOG_CATEGORY_EXTERN(LogAudio, eLogVerbosity::Log, eLogVerbosity::All)
DAEMON_LOG_CATEGORY_EXTERN(LogInput, eLogVerbosity::Log, eLogVerbosity::All)
DAEMON_LOG_CATEGORY_EXTERN(LogNetwork, eLogVerbosity::Log, eLogVerbosity::All)
DAEMON_LOG_CATEGORY_EXTERN(LogResource, eLogVerbosity::Log, eLogVerbosity::All)
DAEMON_LOG_CATEGORY_EXTERN(LogMath, eLogVerbosity::Log, eLogVerbosity::All)
DAEMON_LOG_CATEGORY_EXTERN(LogPlatform, eLogVerbosity::Log, eLogVerbosity::All)
DAEMON_LOG_CATEGORY_EXTERN(LogGame, eLogVerbosity::Log, eLogVerbosity::All)