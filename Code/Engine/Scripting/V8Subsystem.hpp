//----------------------------------------------------------------------------------------------------
// V8Subsystem.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include <any>
#include <functional>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "Engine/Core/StringUtils.hpp"
#include "Engine/Scripting/IScriptableObject.hpp"

// Forward declaration for Chrome DevTools
class ChromeDevToolsServer;

//----------------------------------------------------------------------------------------------------
using ScriptFunction = std::function<std::any(std::vector<std::any> const&)>;

//----------------------------------------------------------------------------------------------------
struct MethodCallbackData
{
    std::shared_ptr<IScriptableObject> object;
    std::string                        methodName;
};

//----------------------------------------------------------------------------------------------------
struct sV8SubsystemConfig
{
    bool        enableDebugging      = false;        // Enable V8 debugging functionality
    size_t      heapSizeLimit        = 256;          // Heap size limit (MB)
    bool        enableScriptBindings = true;         // Enable script bindings
    std::string scriptPath           = "Data/Scripts/"; // Script file path
    bool        enableConsoleOutput  = true;         // Enable console.log output

    // Chrome DevTools Inspector Configuration
#ifdef _DEBUG
    bool enableInspector = true;         // Enable Chrome DevTools integration (default in Debug)
#else
    bool enableInspector = false;        // Disable Chrome DevTools in Release
#endif
    int         inspectorPort   = 9229;         // Chrome DevTools connection port
    std::string inspectorHost   = "127.0.0.1"; // Inspector server bind address (localhost only)
    bool        waitForDebugger = false;        // Pause JavaScript execution until debugger connects
};


//----------------------------------------------------------------------------------------------------
// V8 子系統類別
// 提供 JavaScript 執行環境，支援腳本化物件註冊和全域函式註冊
//----------------------------------------------------------------------------------------------------
class V8Subsystem
{
public:
    explicit V8Subsystem(sV8SubsystemConfig config);
    ~V8Subsystem();

    void Startup();
    void Shutdown();
    void Update();

    // Execution related
    bool ExecuteScript(String const& script);
    bool ExecuteScriptFile(String const& scriptFilename);

    // SCRIPT REGISTRY APPROACH: Selective Chrome DevTools integration
    bool ExecuteRegisteredScript(String const& script, String const& scriptName);
    bool ExecuteUnregisteredScript(String const& script);

    // 執行 JavaScript 程式碼並回傳結果
    std::any ExecuteScriptWithResult(std::string const& script);

    //------------------------------------------------------------------------------------------------
    // 錯誤處理和狀態查詢
    //------------------------------------------------------------------------------------------------

    // 檢查是否有錯誤
    bool HasError() const;

    // 取得最後的錯誤訊息
    String GetLastError() const;

    // 取得最後執行的結果
    String GetLastResult() const;

    // 檢查 V8 是否已初始化
    bool IsInitialized() const;

    // 清除錯誤狀態
    void ClearError();

    // Registration of `object` and `function`
    void RegisterScriptableObject(String const& name, std::shared_ptr<IScriptableObject> const& object);
    void UnregisterScriptableObject(String const& name);
    void RegisterGlobalFunction(String const& name, ScriptFunction const& function);
    void UnregisterGlobalFunction(String const& name);

    // 檢查是否有註冊指定的物件
    bool HasRegisteredObject(String const& name) const;

    // 檢查是否有註冊指定的函式
    bool HasRegisteredFunction(String const& name) const;

    // 取得所有已註冊的物件名稱
    StringList GetRegisteredObjectNames() const;

    // 取得所有已註冊的函式名稱
    StringList GetRegisteredFunctionNames() const;

    //------------------------------------------------------------------------------------------------
    // 除錯和工具功能
    //------------------------------------------------------------------------------------------------

    // 設定是否啟用除錯輸出
    void SetDebugOutput(bool enabled);

    // 取得 JavaScript 執行統計資訊
    struct ExecutionStats
    {
        size_t scriptsExecuted    = 0;
        size_t errorsEncountered  = 0;
        size_t totalExecutionTime = 0; // 毫秒
    };

    ExecutionStats GetExecutionStats() const;

    // 重設執行統計
    void ResetExecutionStats();

    //------------------------------------------------------------------------------------------------
    // 記憶體管理
    //------------------------------------------------------------------------------------------------

    // 強制執行垃圾回收
    void ForceGarbageCollection();

    // 取得記憶體使用情況
    struct MemoryUsage
    {
        size_t usedHeapSize    = 0;       // 已使用的堆疊大小 (bytes)
        size_t totalHeapSize   = 0;      // 總堆疊大小 (bytes)
        size_t heapSizeLimit   = 0;      // 堆疊大小限制 (bytes)
        double usagePercentage = 0.0;  // 使用百分比
    };

    MemoryUsage GetMemoryUsage() const;

    // Chrome DevTools Support
    std::string HandleDebuggerGetScriptSource(const std::string& scriptId);
    void        ReplayScriptsToDevTools();
    void        StoreScriptIdMapping(const std::string& scriptId, const std::string& url);
    void        StoreScriptNotificationForReplay(const std::string& notification);

    // DevTools Panel Event Generation
    void SendPerformanceTimelineEvent(const std::string& eventType, const std::string& name, double timestamp);
    void SendNetworkRequestEvent(const std::string& url, const std::string& method, int statusCode);
    void SendMemoryHeapSnapshot();

private:
    //----------------------------------------------------------------------------------------------------
    /// @brief Internal implementation struct for v8
    /// @see https://www.geeksforgeeks.org/cpp/pimpl-idiom-in-c-with-examples/
    struct V8Implementation;

    // Private helper methods
    bool ExecuteScriptWithOrigin(String const& script, String const& scriptName);

    // Chrome DevTools Script Management (Private)
    std::string ConvertToDevToolsURL(const std::string& scriptPath);
    void        StoreScriptSource(const std::string& url, const std::string& source);
    std::string GetScriptSourceByURL(const std::string& url);
    void        ForwardConsoleMessageToDevTools(const std::string& message);

    // Pointer to the internal implementation
    std::unique_ptr<V8Implementation> m_impl;

    sV8SubsystemConfig m_config;

    // 註冊的腳本物件
    std::unordered_map<String, std::shared_ptr<IScriptableObject>> m_scriptableObjects;

    // 註冊的全域函式
    std::unordered_map<String, ScriptFunction> m_globalFunctions;

    // 狀態追蹤
    bool        m_isInitialized = false;
    bool        m_hasError      = false;
    std::string m_lastError;
    std::string m_lastResult;

    // 執行統計
    ExecutionStats m_stats;

    // 綁定追蹤 (防止重複綁定)
    std::set<String> m_boundObjects;    // 已綁定的物件
    std::set<String> m_boundFunctions;  // 已綁定的函式

    // Chrome DevTools Integration
    std::unique_ptr<ChromeDevToolsServer> m_devToolsServer;

    // Script Source Storage for DevTools
    std::unordered_map<std::string, std::string> m_scriptSources; // URL -> Source Code
    std::unordered_map<std::string, std::string> m_scriptIdToURL; // Script ID -> URL

    // SCRIPT REGISTRY: Selective Chrome DevTools integration
    std::unordered_set<std::string>              m_registeredScripts; // Scripts that should appear in DevTools
    std::unordered_map<std::string, std::string> m_scriptRegistry; // Name -> Source code

    // Priority-based script notification storage for better Chrome DevTools experience
    std::vector<std::string> m_priorityScriptNotifications; // High-priority scripts (JSEngine.js, JSGame.js)
    std::vector<std::string> m_scriptNotifications;         // Regular script notifications

    //------------------------------------------------------------------------------------------------
    // 內部輔助方法
    //------------------------------------------------------------------------------------------------

    // 初始化 V8 引擎
    bool InitializeV8Engine() const;

    // 清理 V8 引擎
    void ShutdownV8Engine();

    // 設定 V8 綁定
    void SetupV8Bindings();

    // 創建單一物件綁定 (incremental binding)
    void CreateSingleObjectBinding(String const&                             objectName,
                                   std::shared_ptr<IScriptableObject> const& object);

    // 創建單一函式綁定 (incremental binding)
    void CreateSingleFunctionBinding(String const&         functionName,
                                     ScriptFunction const& function);

    // 設定內建的 JavaScript 物件和函式
    void SetupBuiltinObjects();

    // 錯誤處理
    void HandleV8Error(String const& error);

    // 將 C++ 的 std::any 轉換為 V8 的值
    void* ConvertToV8Value(const std::any& value);

    // 將 V8 的值轉換為 C++ 的 std::any
    std::any ConvertFromV8Value(void* v8Value);

    // 驗證腳本檔案路徑
    std::string ValidateScriptPath(std::string const& filename) const;

    // 儲存回呼資料的容器（避免記憶體洩漏）
    std::vector<std::unique_ptr<MethodCallbackData>> m_methodCallbacks;
    std::vector<std::unique_ptr<ScriptFunction>>     m_functionCallbacks;
};
