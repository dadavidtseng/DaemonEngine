//----------------------------------------------------------------------------------------------------
// V8Subsystem.hpp - 整合 JavaScriptManager 功能的版本
//----------------------------------------------------------------------------------------------------

#pragma once
// 在任何標頭檔之前加入
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "Engine/Core/EngineCommon.hpp"
#include <string>
#include <functional>
// #include <v8-function-callback.h>
#include "ThirdParty/packages/v8-v143-x64.13.0.245.25/include/v8-function-callback.h"
// #include "ThirdParty/packages/v8-v143-x64/13.0.245.25/include/v8-function.h"
// #include <v8-function-callback.h>

// 前向宣告 V8 類型
namespace v8
{
    class Object;
    class Isolate;
    class Context;
    template<class T> class Local;
    template<class T> class Global;
    template<class T> class FunctionCallbackInfo;
    class Value;
    class Platform;
}

// 前向宣告遊戲類型
class Game;

//----------------------------------------------------------------------------------------------------
// V8 子系統配置
//----------------------------------------------------------------------------------------------------
struct sV8SubsystemConfig
{
    bool enableDebugging = false;
    int heapSizeLimit = 256; // MB
    bool enableGameBindings = true; // 是否啟用遊戲物件綁定
};

//----------------------------------------------------------------------------------------------------
// V8 子系統 - 完整的 JavaScript 引擎管理器
// 包含原本 JavaScriptManager 的所有功能
//----------------------------------------------------------------------------------------------------
class V8Subsystem
{
public:
    explicit V8Subsystem(sV8SubsystemConfig const& config);
    ~V8Subsystem();

    // 子系統生命週期
    void Startup();
    void Shutdown();
    void BeginFrame();
    void EndFrame();

    // 基本 JavaScript 執行功能
    bool ExecuteScript(const std::string& script);
    bool ExecuteScriptFile(const std::string& filename);

    // 遊戲特定功能（原 JavaScriptManager 功能）
    void BindGameObjects(Game* game);
    void UnbindGameObjects();

    // 安全的上下文執行
    bool ExecuteInContext(std::function<bool(v8::Local<v8::Context>)> callback);

    // 狀態查詢
    bool IsInitialized() const { return m_isInitialized; }
    bool HasError() const { return !m_lastError.empty(); }
    std::string GetLastError() const { return m_lastError; }
    std::string GetLastResult() const { return m_lastResult; }

    // V8 存取器
    v8::Isolate* GetIsolate() const { return m_isolate; }

private:
    sV8SubsystemConfig m_config;
    bool m_isInitialized;
    std::string m_lastError;
    std::string m_lastResult;
    Game* m_gameReference;

    // V8 核心物件
    static v8::Platform* s_platform;
    static int s_instanceCount;
    v8::Isolate* m_isolate;
    v8::Global<v8::Context>* m_context;

    // 內部方法
    bool InitializeV8();
    void ShutdownV8();
    void CreateGlobalObjects();
    void BindGameFunctions();

    // JavaScript 回呼函數（原 JavaScriptManager 的回呼）
    static void JSLog(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void JSCreateCube(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void JSMoveProp(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void JSGetPlayerPosition(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void JSSetPlayerPosition(const v8::FunctionCallbackInfo<v8::Value>& args);

    // 輔助方法
    bool BindFunction(v8::Local<v8::Context> context, v8::Local<v8::Object> object,
                     const char* name, v8::FunctionCallback callback);
    void LogError(const std::string& error);
    bool ExecuteFallbackScript(const std::string& script);
};

//----------------------------------------------------------------------------------------------------
// 全域 V8 子系統
extern V8Subsystem* g_theV8Subsystem;