//----------------------------------------------------------------------------------------------------
// V8Subsystem.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include <any>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "Engine/Core/StringUtils.hpp"
#include "Engine/Scripting/IScriptableObject.hpp"

//----------------------------------------------------------------------------------------------------
using ScriptFunction = std::function<std::any(std::vector<std::any> const&)>;

//----------------------------------------------------------------------------------------------------
struct MethodCallbackData
{
    std::shared_ptr<IScriptableObject> m_object;
    std::string                        m_methodName;
};

//----------------------------------------------------------------------------------------------------
struct sV8SubsystemConfig
{
    bool        enableDebugging      = false;        // 是否啟用除錯功能
    size_t      heapSizeLimit        = 256;          // 堆疊大小限制 (MB)
    bool        enableScriptBindings = true;    // 是否啟用腳本綁定 (取代原本的 enableGameBindings)
    std::string scriptPath           = "Data/Scripts/"; // 腳本檔案路徑
    bool        enableConsoleOutput  = true;     // 是否啟用 console.log 輸出
};


//----------------------------------------------------------------------------------------------------
// V8 子系統類別
// 提供 JavaScript 執行環境，支援腳本化物件註冊和全域函式註冊
//----------------------------------------------------------------------------------------------------
class V8Subsystem
{
public:
    explicit V8Subsystem(sV8SubsystemConfig const& config);
    ~V8Subsystem();

    void Startup();
    void Shutdown();
    void Update();

    //------------------------------------------------------------------------------------------------
    // 腳本執行功能
    //------------------------------------------------------------------------------------------------

    // 執行 JavaScript 程式碼字串
    bool ExecuteScript(std::string const& script);

    // 執行 JavaScript 檔案
    bool ExecuteScriptFile(std::string const& filename);

    // 執行 JavaScript 程式碼並回傳結果
    std::any ExecuteScriptWithResult(std::string const& script);

    //------------------------------------------------------------------------------------------------
    // 錯誤處理和狀態查詢
    //------------------------------------------------------------------------------------------------

    // 檢查是否有錯誤
    bool HasError() const;

    // 取得最後的錯誤訊息
    std::string GetLastError() const;

    // 取得最後執行的結果
    std::string GetLastResult() const;

    // 檢查 V8 是否已初始化
    bool IsInitialized() const;

    // 清除錯誤狀態
    void ClearError();

    //------------------------------------------------------------------------------------------------
    // 物件和函式註冊系統（新的架構）
    //------------------------------------------------------------------------------------------------

    void RegisterScriptableObject(String const& name, std::shared_ptr<IScriptableObject> const& object);
    void UnregisterScriptableObject(String const& name);
    void RegisterGlobalFunction(String const& name, ScriptFunction const& function);
    void UnregisterGlobalFunction(String const& name);

    // 檢查是否有註冊指定的物件
    bool HasRegisteredObject(const std::string& name) const;

    // 檢查是否有註冊指定的函式
    bool HasRegisteredFunction(const std::string& name) const;

    // 取得所有已註冊的物件名稱
    std::vector<std::string> GetRegisteredObjectNames() const;

    // 取得所有已註冊的函式名稱
    std::vector<std::string> GetRegisteredFunctionNames() const;

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

private:
    //------------------------------------------------------------------------------------------------
    // 內部實作細節
    //------------------------------------------------------------------------------------------------

    // V8 引擎的具體實作（使用 Pimpl 模式隱藏 V8 的複雜性）
    struct V8Implementation;
    std::unique_ptr<V8Implementation> m_impl;

    // 設定資料
    sV8SubsystemConfig m_config;

    // 註冊的腳本物件
    std::unordered_map<std::string, std::shared_ptr<IScriptableObject>> m_scriptableObjects;

    // 註冊的全域函式
    std::unordered_map<std::string, ScriptFunction> m_globalFunctions;

    // 狀態追蹤
    bool        m_isInitialized = false;
    bool        m_hasError      = false;
    std::string m_lastError;
    std::string m_lastResult;

    // 執行統計
    mutable ExecutionStats m_stats;

    //------------------------------------------------------------------------------------------------
    // 內部輔助方法
    //------------------------------------------------------------------------------------------------

    // 初始化 V8 引擎
    bool InitializeV8Engine();

    // 清理 V8 引擎
    void ShutdownV8Engine();

    // 設定 V8 綁定
    void SetupV8Bindings();

    // 創建物件綁定
    void CreateObjectBindings();

    // 創建函式綁定
    void CreateFunctionBindings();

    // 設定內建的 JavaScript 物件和函式
    void SetupBuiltinObjects();

    // 錯誤處理
    void HandleV8Error(const std::string& error);

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
