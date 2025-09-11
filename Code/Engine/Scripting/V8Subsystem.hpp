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
#include <vector>

#include "Engine/Core/StringUtils.hpp"
#include "Engine/Scripting/IScriptableObject.hpp"

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
    explicit V8Subsystem(sV8SubsystemConfig config);
    ~V8Subsystem();

    void Startup();
    void Shutdown();
    void Update();

    // Execution related
    bool ExecuteScript(String const& script);
    bool ExecuteScriptFile(String const& scriptFilename);

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

private:
    //----------------------------------------------------------------------------------------------------
    /// @brief Internal implementation struct for v8
    /// @see https://www.geeksforgeeks.org/cpp/pimpl-idiom-in-c-with-examples/
    struct V8Implementation;

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
    void CreateSingleObjectBinding(const String&                             objectName,
                                   std::shared_ptr<IScriptableObject> const& object);

    // 創建單一函式綁定 (incremental binding)
    void CreateSingleFunctionBinding(const String&         functionName,
                                     const ScriptFunction& function);

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
