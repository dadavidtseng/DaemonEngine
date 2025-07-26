//----------------------------------------------------------------------------------------------------
// V8Subsystem.cpp - 完整的 V8 實作
//----------------------------------------------------------------------------------------------------

#include "Engine/Scripting/V8Subsystem.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/FileUtils.hpp"

//----------------------------------------------------------------------------------------------------
// 暫時關閉 V8 標頭檔的警告
//----------------------------------------------------------------------------------------------------
#pragma warning(push)
#pragma warning(disable: 4100) // 未使用的參數
#pragma warning(disable: 4324) // 結構體對齊
#pragma warning(disable: 4127) // 常數條件運算式

#include <v8.h>
#include <v8-platform.h>

#pragma warning(pop)

//----------------------------------------------------------------------------------------------------
// V8 內部資料結構
//----------------------------------------------------------------------------------------------------
struct V8Subsystem::V8InternalData
{
    bool isStubMode = true;
};

//----------------------------------------------------------------------------------------------------
// 全域變數定義
//----------------------------------------------------------------------------------------------------
V8Subsystem* g_theV8Subsystem = nullptr;

//----------------------------------------------------------------------------------------------------
// 建構函數
//----------------------------------------------------------------------------------------------------
V8Subsystem::V8Subsystem(const Config& config)
    : m_config(config)
    , m_internalData(new V8InternalData())
    , m_initialized(false)
{
    LogInfo("V8 子系統已建立（Stub 模式）");
}

//----------------------------------------------------------------------------------------------------
// 解構函數
//----------------------------------------------------------------------------------------------------
V8Subsystem::~V8Subsystem()
{
    Shutdown();
    delete m_internalData;
    m_internalData = nullptr;
}

//----------------------------------------------------------------------------------------------------
// 啟動子系統
//----------------------------------------------------------------------------------------------------
void V8Subsystem::Startup()
{
    if (m_initialized)
    {
        LogError("V8 子系統已經初始化過了");
        return;
    }

    LogInfo("V8 子系統啟動（Stub 模式 - 需要解決 V8 標頭檔問題）");
    LogInfo("已安裝的 V8 版本：v8-v143-x64 13.0.245.25");
    m_initialized = true;
}

//----------------------------------------------------------------------------------------------------
// 關閉子系統
//----------------------------------------------------------------------------------------------------
void V8Subsystem::Shutdown()
{
    if (!m_initialized) return;

    LogInfo("V8 子系統關閉");
    m_initialized = false;
}

//----------------------------------------------------------------------------------------------------
// 更新子系統
//----------------------------------------------------------------------------------------------------
void V8Subsystem::Update(float deltaSeconds)
{
    UNUSED(deltaSeconds);
    // Stub 實作
}

//----------------------------------------------------------------------------------------------------
// 執行 JavaScript 程式碼
//----------------------------------------------------------------------------------------------------
bool V8Subsystem::ExecuteScript(const std::string& scriptContent)
{
    UNUSED(scriptContent);
    LogError("V8 JavaScript 執行功能尚未實作（需要 V8 標頭檔）");
    LogInfo("腳本內容預覽：" + scriptContent.substr(0, std::min(50u, (unsigned)scriptContent.length())) + "...");
    return false;
}

//----------------------------------------------------------------------------------------------------
// 執行腳本檔案
//----------------------------------------------------------------------------------------------------
bool V8Subsystem::ExecuteScriptFile(const std::string& filePath)
{
    LogInfo("嘗試執行腳本檔案：" + m_config.m_scriptsPath + filePath);
    LogError("V8 檔案執行功能尚未實作（需要 V8 標頭檔）");
    return false;
}

//----------------------------------------------------------------------------------------------------
// 設定全域變數（Stub 實作）
//----------------------------------------------------------------------------------------------------
void V8Subsystem::SetGlobalNumber(const std::string& name, double value)
{
    LogInfo(Stringf("設定全域數字變數：%s = %f（Stub 模式）", name.c_str(), value));
}

void V8Subsystem::SetGlobalString(const std::string& name, const std::string& value)
{
    LogInfo(Stringf("設定全域字串變數：%s = %s（Stub 模式）", name.c_str(), value.c_str()));
}

void V8Subsystem::SetGlobalBoolean(const std::string& name, bool value)
{
    LogInfo(Stringf("設定全域布林變數：%s = %s（Stub 模式）", name.c_str(), value ? "true" : "false"));
}

//----------------------------------------------------------------------------------------------------
// 取得全域變數（Stub 實作）
//----------------------------------------------------------------------------------------------------
double V8Subsystem::GetGlobalNumber(const std::string& name)
{
    UNUSED(name);
    return 0.0;
}

std::string V8Subsystem::GetGlobalString(const std::string& name)
{
    UNUSED(name);
    return "";
}

bool V8Subsystem::GetGlobalBoolean(const std::string& name)
{
    UNUSED(name);
    return false;
}

//----------------------------------------------------------------------------------------------------
// 輔助函數
//----------------------------------------------------------------------------------------------------
void V8Subsystem::LogError(const std::string& error)
{
    m_lastError = error;
    if (g_theDevConsole)
    {
        g_theDevConsole->AddLine(DevConsole::ERROR, Stringf("[V8錯誤] %s", error.c_str()));
    }
}

void V8Subsystem::LogInfo(const std::string& info)
{
    if (g_theDevConsole)
    {
        g_theDevConsole->AddLine(DevConsole::INFO_MAJOR, Stringf("[V8] %s", info.c_str()));
    }
}