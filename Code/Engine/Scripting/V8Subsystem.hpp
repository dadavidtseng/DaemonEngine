//----------------------------------------------------------------------------------------------------
// V8Subsystem.hpp - JavaScript 引擎子系統
//----------------------------------------------------------------------------------------------------

#pragma once

#include "Engine/Core/EngineCommon.hpp"
#include <string>
#include <vector>
#include <functional>

//----------------------------------------------------------------------------------------------------
// 簡化的 V8 包裝器，避免在標頭檔中包含 V8 標頭
//----------------------------------------------------------------------------------------------------
class V8Subsystem
{
public:
    struct Config
    {
        std::string m_scriptsPath = "Data/Scripts/";
        bool m_enableConsoleOutput = true;
        bool m_enableDebugger = false;
    };

    // 建構和解構
    explicit V8Subsystem(const Config& config);
    ~V8Subsystem();

    // 子系統生命週期
    void Startup();
    void Shutdown();
    void Update(float deltaSeconds);

    // 腳本執行
    bool ExecuteScript(const std::string& scriptContent);
    bool ExecuteScriptFile(const std::string& filePath);

    // 與 C++ 的互動介面
    void RegisterGlobalFunction(const std::string& name, std::function<void()> func);
    void RegisterGlobalFunction(const std::string& name, std::function<float(float)> func);
    void RegisterGlobalFunction(const std::string& name, std::function<void(const std::string&)> func);

    // 設定全域變數
    void SetGlobalNumber(const std::string& name, double value);
    void SetGlobalString(const std::string& name, const std::string& value);
    void SetGlobalBoolean(const std::string& name, bool value);

    // 取得全域變數
    double GetGlobalNumber(const std::string& name);
    std::string GetGlobalString(const std::string& name);
    bool GetGlobalBoolean(const std::string& name);

    // 錯誤處理
    std::string GetLastError() const { return m_lastError; }
    bool HasError() const { return !m_lastError.empty(); }
    void ClearError() { m_lastError.clear(); }

private:
    // 內部實作細節隱藏在 .cpp 檔中
    struct V8InternalData;
    V8InternalData* m_internalData;

    Config m_config;
    std::string m_lastError;
    bool m_initialized;

    // 內部方法
    void InitializeV8();
    void ShutdownV8();
    void SetupBuiltinFunctions();
    void LogError(const std::string& error);
    void LogInfo(const std::string& error);
};

//----------------------------------------------------------------------------------------------------
// 全域指標
//----------------------------------------------------------------------------------------------------
extern V8Subsystem* g_theV8Subsystem;