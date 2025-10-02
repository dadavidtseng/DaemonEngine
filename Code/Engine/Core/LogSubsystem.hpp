//----------------------------------------------------------------------------------------------------
// LogSubsystem.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/ILogOutputDevice.hpp"
#include "Engine/Core/SmartFileOutputDevice.hpp"
#include "ThirdParty/json/json.hpp"

//----------------------------------------------------------------------------------------------------
// 日誌詳細程度層級 (按照 UE 的方式定義)
//----------------------------------------------------------------------------------------------------
enum class eLogVerbosity : int8_t
{
    NoLogging = 0,        // 不記錄任何日誌
    Fatal = 1,            // 致命錯誤，會導致程式崩潰
    Error = 2,            // 錯誤但程式可以繼續執行
    Warning = 3,          // 警告
    Display = 4,          // 顯示給使用者的重要訊息
    Log = 5,              // 一般日誌訊息
    Verbose = 6,          // 詳細資訊
    VeryVerbose = 7,      // 非常詳細的資訊

    All = VeryVerbose // 所有層級
};

//----------------------------------------------------------------------------------------------------
// 日誌輸出目標
//----------------------------------------------------------------------------------------------------
enum class eLogOutput : uint8_t
{
    None = 0,
    Console = 1 << 0,  // 輸出到控制台
    File = 1 << 1,  // 輸出到檔案
    DebugOutput = 1 << 2,  // 輸出到 Visual Studio 輸出視窗
    OnScreen = 1 << 3,  // 輸出到螢幕上 (類似 UE 的 AddOnScreenDebugMessage)
    DevConsole = 1 << 4,  // 輸出到 DaemonEngine 的開發者控制台

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
    String        name;                                    // 分類名稱
    eLogVerbosity defaultVerbosity     = eLogVerbosity::Log; // 預設詳細程度
    eLogVerbosity compileTimeVerbosity = eLogVerbosity::All; // 編譯時最大詳細程度
    eLogOutput    outputTargets        = eLogOutput::All;   // 輸出目標

    LogCategory() = default;

    explicit LogCategory(String              categoryName,
                         eLogVerbosity const defaultVerb = eLogVerbosity::Log,
                         eLogVerbosity const compileVerb = eLogVerbosity::All,
                         eLogOutput const    outputs     = eLogOutput::All)
        : name(std::move(categoryName)),
          defaultVerbosity(defaultVerb),
          compileTimeVerbosity(compileVerb),
          outputTargets(outputs)
    {
    }
};

//----------------------------------------------------------------------------------------------------
// 日誌條目結構
//----------------------------------------------------------------------------------------------------
struct LogEntry
{
    String        category;      // 日誌分類
    eLogVerbosity verbosity;    // 詳細程度
    String        message;       // 日誌訊息
    String        timestamp;     // 時間戳記
    String        threadId;      // 執行緒 ID
    String        functionName;  // 函數名稱
    String        fileName;      // 檔案名稱
    int           lineNumber;    // 行號

    LogEntry() = default;
    LogEntry(const String& cat, eLogVerbosity       verb, const String& msg,
             const String& func = "", const String& file = "", int      line = 0);
};

//----------------------------------------------------------------------------------------------------
// 日誌子系統設定 (Enhanced with rotation support)
//----------------------------------------------------------------------------------------------------
struct sLogSubsystemConfig
{
    String logFilePath      = "Logs/DaemonEngine.log";  // 日誌檔案路徑
    bool   enableConsole    = true;                     // 啟用控制台輸出
    bool   enableFile       = true;                     // 啟用檔案輸出
    bool   enableDebugOut   = true;                     // 啟用除錯輸出
    bool   enableOnScreen   = true;                     // 啟用螢幕輸出
    bool   enableDevConsole = true;                     // 啟用開發者控制台輸出
    bool   asyncLogging     = true;                     // 啟用非同步日誌
    int    maxLogEntries    = 10000;                    // 記憶體中最大日誌條目數
    bool   timestampEnabled = true;                     // 啟用時間戳記
    bool   threadIdEnabled  = true;                     // 啟用執行緒 ID
    bool   autoFlush        = false;                    // 自動重新整理輸出

    // Enhanced rotation settings
    bool                 enableSmartRotation = true;                  // 啟用智能日誌輪轉
    String               rotationConfigPath  = "Data/Config/LogRotation.json"; // 輪轉配置檔案路徑
    sSmartRotationConfig smartRotationConfig;                  // Smart rotation configuration

    // JSON Parsing
    static sLogSubsystemConfig FromJSON(nlohmann::json const& j)
    {
        sLogSubsystemConfig config;

        // Basic logging settings
        if (j.contains("logFilePath"))
            config.logFilePath = j["logFilePath"].get<std::string>();
        if (j.contains("enableConsole"))
            config.enableConsole = j["enableConsole"].get<bool>();
        if (j.contains("enableFile"))
            config.enableFile = j["enableFile"].get<bool>();
        if (j.contains("enableDebugOut"))
            config.enableDebugOut = j["enableDebugOut"].get<bool>();
        if (j.contains("enableOnScreen"))
            config.enableOnScreen = j["enableOnScreen"].get<bool>();
        if (j.contains("enableDevConsole"))
            config.enableDevConsole = j["enableDevConsole"].get<bool>();
        if (j.contains("asyncLogging"))
            config.asyncLogging = j["asyncLogging"].get<bool>();
        if (j.contains("maxLogEntries"))
            config.maxLogEntries = j["maxLogEntries"].get<int>();
        if (j.contains("timestampEnabled"))
            config.timestampEnabled = j["timestampEnabled"].get<bool>();
        if (j.contains("threadIdEnabled"))
            config.threadIdEnabled = j["threadIdEnabled"].get<bool>();
        if (j.contains("autoFlush"))
            config.autoFlush = j["autoFlush"].get<bool>();

        // Rotation settings (path only - SmartFileOutputDevice will load the actual config)
        if (j.contains("enableSmartRotation"))
            config.enableSmartRotation = j["enableSmartRotation"].get<bool>();
        if (j.contains("rotationConfigPath"))
            config.rotationConfigPath = j["rotationConfigPath"].get<std::string>();

        // Note: smartRotationConfig is NOT parsed here - SmartFileOutputDevice loads it from rotationConfigPath

        return config;
    }
};

//----------------------------------------------------------------------------------------------------
// 日誌子系統主類別
//----------------------------------------------------------------------------------------------------
class LogSubsystem
{
public:
    explicit LogSubsystem(sLogSubsystemConfig config = sLogSubsystemConfig{});
    ~LogSubsystem();

    // 子系統生命週期
    void Startup();
    void Shutdown();
    void BeginFrame();
    void EndFrame();
    void Update(float deltaSeconds);

    // 日誌分類管理
    void RegisterCategory(const String& categoryName,
                          eLogVerbosity defaultVerbosity     = eLogVerbosity::Log,
                          eLogVerbosity compileTimeVerbosity = eLogVerbosity::All,
                          eLogOutput    outputTargets        = eLogOutput::All);
    void         SetCategoryVerbosity(const String& categoryName, eLogVerbosity verbosity);
    LogCategory* GetCategory(const String& categoryName);
    bool         IsCategoryRegistered(const String& categoryName) const;

    // 日誌記錄
    void LogMessage(const String& categoryName, eLogVerbosity      verbosity, const String& message,
                    const String& functionName = "", const String& fileName = "", int       lineNumber = 0);

    // 條件日誌記錄
    void LogMessageIf(bool          condition, const String& categoryName, eLogVerbosity verbosity,
                      const String& message, const String&   functionName = "",
                      const String& fileName                              = "", int lineNumber = 0);

    // 格式化日誌記錄 (支援 printf 風格)
    template <typename... Args>
    void LogFormatted(String const&       categoryName,
                      eLogVerbosity const verbosity,
                      String const&       format,
                      Args...             args)
    {
        if (!ShouldLog(categoryName, verbosity))
        {
            return;
        }

        String formattedMessage = Stringf(format.c_str(), args...);
        LogMessage(categoryName, verbosity, formattedMessage);
    }

    // 螢幕訊息 (類似 UE 的 AddOnScreenDebugMessage)
    void AddOnScreenMessage(const String& message, float displayTime = 5.0f,
                            const Rgba8&  color                      = Rgba8::WHITE, int uniqueId = -1);

    // 日誌歷史存取
    std::vector<LogEntry> GetLogHistory(const String& categoryFilter = "",
                                        eLogVerbosity minVerbosity   = eLogVerbosity::NoLogging) const;
    void ClearLogHistory();

    // 輸出裝置管理
    void AddOutputDevice(std::unique_ptr<ILogOutputDevice> device);
    void FlushAllOutputs();

    // 設定存取
    const sLogSubsystemConfig& GetConfig() const { return m_config; }
    void                       SetConfig(const sLogSubsystemConfig& config) { m_config = config; }

    // Smart rotation support
    void                   ForceLogRotation();                                        // Force immediate log rotation
    sRotationStats         GetRotationStats() const;                        // Get rotation statistics
    bool                   IsSmartRotationEnabled() const { return m_config.enableSmartRotation; }
    void                   UpdateSmartRotationConfig(const sSmartRotationConfig& config); // Update rotation configuration
    SmartFileOutputDevice* GetSmartFileDevice() const;             // Get smart file device if available
    bool                   LoadRotationConfigFromFile(const String& configPath);     // Load configuration from JSON

    String GetCurrentTimestamp() const;
    String GetCurrentThreadId() const;

private:
    void ProcessLogQueue();
    void WriteToOutputDevices(const LogEntry& entry);

    bool ShouldLog(const String& categoryName, eLogVerbosity verbosity) const;

    sLogSubsystemConfig                            m_config;
    std::unordered_map<String, LogCategory>        m_categories;
    std::vector<std::unique_ptr<ILogOutputDevice>> m_outputDevices;

    // 非同步日誌相關
    std::queue<LogEntry> m_logQueue;
    mutable std::mutex   m_queueMutex;
    std::thread          m_logThread;
    std::atomic<bool>    m_shouldExit{false};

    // 記憶體中的日誌歷史
    std::vector<LogEntry> m_logHistory;
    mutable std::mutex    m_historyMutex;

    // Smart rotation support
    SmartFileOutputDevice* m_smartFileDevice;                      // Pointer to smart file device (if enabled)
    bool                   m_isSmartRotationInitialized;                             // Track if smart rotation is properly set up
};

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
