// V8Subsystem.cpp - 修正 HandleScope 問題的版本
#include "Engine/Scripting/V8Subsystem.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/FileUtils.hpp"

// V8 標頭檔
#pragma warning(push)
#pragma warning(disable: 4100 4324 4127)

#include <stdexcept>
#include <v8.h>
// #include <v8-platform.h>
#include "ThirdParty/packages/v8-v143-x64.13.0.245.25/include/libplatform/libplatform.h"

#pragma warning(pop)

//----------------------------------------------------------------------------------------------------
// V8 內部資料結構 - 現在是真實的
//----------------------------------------------------------------------------------------------------
struct V8Subsystem::V8InternalData
{
    std::unique_ptr<v8::Platform> platform;
    v8::ArrayBuffer::Allocator* allocator = nullptr;
    v8::Isolate* isolate = nullptr;
    v8::Global<v8::Context> context;
    bool isStubMode = false;
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
    LogInfo("V8 子系統已建立");
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

    try
    {
        LogInfo("正在初始化 V8 引擎...");

        // 第一步：初始化 V8 Platform
        v8::V8::InitializeICU();
        v8::V8::InitializeExternalStartupData("");

        m_internalData->platform = v8::platform::NewDefaultPlatform();
        v8::V8::InitializePlatform(m_internalData->platform.get());

        // 第二步：初始化 V8
        if (!v8::V8::Initialize())
        {
            throw std::runtime_error("V8::Initialize() 失敗");
        }

        LogInfo("V8 Platform 和引擎初始化成功");

        // 第三步：建立 Allocator
        m_internalData->allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();

        // 第四步：建立 Isolate
        v8::Isolate::CreateParams create_params;
        create_params.array_buffer_allocator = m_internalData->allocator;

        m_internalData->isolate = v8::Isolate::New(create_params);
        if (!m_internalData->isolate)
        {
            throw std::runtime_error("無法建立 V8 Isolate");
        }

        // 第五步：建立 Context
        {
            v8::Isolate::Scope isolate_scope(m_internalData->isolate);
            v8::HandleScope handle_scope(m_internalData->isolate);

            v8::Local<v8::Context> context = v8::Context::New(m_internalData->isolate);
            if (context.IsEmpty())
            {
                throw std::runtime_error("無法建立 V8 Context");
            }

            m_internalData->context.Reset(m_internalData->isolate, context);

            // 設定內建函數
            v8::Context::Scope context_scope(context);
            SetupBuiltinFunctions();
        }

        m_initialized = true;
        LogInfo("V8 子系統初始化成功！");
        LogInfo("已安裝的 V8 版本：v8-v143-x64 13.0.245.25");
    }
    catch (const std::exception& e)
    {
        LogError(Stringf("V8 初始化失敗: %s", e.what()));
        LogError("切換到 Stub 模式");

        // 清理失敗的資源
        ShutdownV8();
        m_internalData->isStubMode = true;
        m_initialized = true; // 標記為已初始化，但是是 stub 模式
    }
    catch (...)
    {
        LogError("V8 初始化時發生未知錯誤");
        LogError("切換到 Stub 模式");

        ShutdownV8();
        m_internalData->isStubMode = true;
        m_initialized = true;
    }
}

//----------------------------------------------------------------------------------------------------
// 關閉子系統
//----------------------------------------------------------------------------------------------------
void V8Subsystem::Shutdown()
{
    if (!m_initialized) return;

    LogInfo("正在關閉 V8 子系統...");
    ShutdownV8();
    m_initialized = false;
    LogInfo("V8 子系統已關閉");
}

//----------------------------------------------------------------------------------------------------
// 內部 V8 關閉
//----------------------------------------------------------------------------------------------------
void V8Subsystem::ShutdownV8()
{
    if (!m_internalData->context.IsEmpty())
    {
        m_internalData->context.Reset();
    }

    if (m_internalData->isolate)
    {
        m_internalData->isolate->Dispose();
        m_internalData->isolate = nullptr;
    }

    if (m_internalData->allocator)
    {
        delete m_internalData->allocator;
        m_internalData->allocator = nullptr;
    }

    if (m_internalData->platform)
    {
        v8::V8::Dispose();
        v8::V8::DisposePlatform();
        m_internalData->platform.reset();
    }
}

//----------------------------------------------------------------------------------------------------
// 更新子系統
//----------------------------------------------------------------------------------------------------
void V8Subsystem::Update(float deltaSeconds)
{
    UNUSED(deltaSeconds);

    if (!m_initialized || m_internalData->isStubMode) return;

    // 處理 V8 的微任務（如果有的話）
    if (m_internalData->isolate)
    {
        v8::Isolate::Scope isolate_scope(m_internalData->isolate);
        v8::HandleScope handle_scope(m_internalData->isolate);

        // 處理待處理的微任務
        m_internalData->isolate->PerformMicrotaskCheckpoint();
    }
}

//----------------------------------------------------------------------------------------------------
// 執行 JavaScript 程式碼
//----------------------------------------------------------------------------------------------------
bool V8Subsystem::ExecuteScript(const std::string& scriptContent)
{
    if (!m_initialized)
    {
        LogError("V8 子系統尚未初始化");
        return false;
    }

    if (m_internalData->isStubMode)
    {
        LogInfo("STUB 模式：執行腳本 - " + scriptContent.substr(0, std::min(50u, (unsigned)scriptContent.length())));
        return true;
    }

    try
    {
        v8::Isolate::Scope isolate_scope(m_internalData->isolate);
        v8::HandleScope handle_scope(m_internalData->isolate);
        v8::Local<v8::Context> context = v8::Local<v8::Context>::New(m_internalData->isolate, m_internalData->context);
        v8::Context::Scope context_scope(context);
        v8::TryCatch try_catch(m_internalData->isolate);

        // 編譯腳本
        v8::Local<v8::String> source = v8::String::NewFromUtf8(m_internalData->isolate, scriptContent.c_str()).ToLocalChecked();
        v8::Local<v8::Script> script;

        if (!v8::Script::Compile(context, source).ToLocal(&script))
        {
            if (try_catch.HasCaught())
            {
                v8::String::Utf8Value exception(m_internalData->isolate, try_catch.Exception());
                LogError(Stringf("JavaScript 編譯錯誤: %s", *exception));
            }
            return false;
        }

        // 執行腳本
        v8::Local<v8::Value> result;
        if (!script->Run(context).ToLocal(&result))
        {
            if (try_catch.HasCaught())
            {
                v8::String::Utf8Value exception(m_internalData->isolate, try_catch.Exception());
                LogError(Stringf("JavaScript 執行錯誤: %s", *exception));
            }
            return false;
        }

        // 輸出結果（如果不是 undefined）
        if (!result->IsUndefined())
        {
            v8::String::Utf8Value utf8(m_internalData->isolate, result);
            LogInfo(Stringf("腳本執行結果: %s", *utf8));
        }

        return true;
    }
    catch (const std::exception& e)
    {
        LogError(Stringf("執行腳本時發生例外: %s", e.what()));
        return false;
    }
    catch (...)
    {
        LogError("執行腳本時發生未知例外");
        return false;
    }
}

//----------------------------------------------------------------------------------------------------
// 執行腳本檔案
//----------------------------------------------------------------------------------------------------
bool V8Subsystem::ExecuteScriptFile(const std::string& filePath)
{
    // std::string fullPath = m_config.m_scriptsPath + filePath;
    std::string fullPath =  filePath;
    LogInfo("正在執行腳本檔案：" + fullPath);

    // 讀取檔案內容
    std::string scriptContent;
    if (!FileReadToString(scriptContent, fullPath))
    {
        LogError("無法讀取腳本檔案：" + fullPath);
        return false;
    }

    return ExecuteScript(scriptContent);
}

//----------------------------------------------------------------------------------------------------
// 設定內建函數
//----------------------------------------------------------------------------------------------------
void V8Subsystem::SetupBuiltinFunctions()
{
    if (!m_internalData->isolate || m_internalData->context.IsEmpty()) return;

    v8::Local<v8::Context> context = v8::Local<v8::Context>::New(m_internalData->isolate, m_internalData->context);

    // 建立 console 物件
    v8::Local<v8::Object> console = v8::Object::New(m_internalData->isolate);

    // 建立 console.log 函數
    auto logCallback = [](const v8::FunctionCallbackInfo<v8::Value>& args) {
        std::string output = "JS: ";
        for (int i = 0; i < args.Length(); i++) {
            if (i > 0) output += " ";
            v8::String::Utf8Value str(args.GetIsolate(), args[i]);
            output += *str;
        }
        if (g_theV8Subsystem) {
            g_theV8Subsystem->LogInfo(output);
        }
    };

    v8::Local<v8::Function> logFunction = v8::Function::New(context, logCallback).ToLocalChecked();
    console->Set(context, v8::String::NewFromUtf8(m_internalData->isolate, "log").ToLocalChecked(), logFunction);

    // 將 console 設為全域變數
    context->Global()->Set(context, v8::String::NewFromUtf8(m_internalData->isolate, "console").ToLocalChecked(), console);

    LogInfo("V8 內建函數設定完成");
}

//----------------------------------------------------------------------------------------------------
// 取得 V8 Isolate（供 JavaScriptManager 使用）
//----------------------------------------------------------------------------------------------------
v8::Isolate* V8Subsystem::GetIsolate() const
{
    return m_internalData->isolate;
}

//----------------------------------------------------------------------------------------------------
// 取得 V8 Context（供 JavaScriptManager 使用）- 修正版本
//----------------------------------------------------------------------------------------------------
bool V8Subsystem::GetContext(v8::Local<v8::Context>& outContext) const
{
    if (m_internalData->context.IsEmpty() || !m_internalData->isolate)
    {
        return false;
    }

    // 這個方法應該只在已經有 HandleScope 的情況下被呼叫
    outContext = v8::Local<v8::Context>::New(m_internalData->isolate, m_internalData->context);
    return true;
}

//----------------------------------------------------------------------------------------------------
// 新的安全執行方法，供 JavaScriptManager 使用
//----------------------------------------------------------------------------------------------------
bool V8Subsystem::ExecuteInContext(std::function<bool(v8::Local<v8::Context>)> callback)
{
    if (!m_initialized || m_internalData->isStubMode || !m_internalData->isolate)
    {
        LogError("V8Subsystem 不可用於執行操作");
        return false;
    }

    try
    {
        v8::Isolate::Scope isolate_scope(m_internalData->isolate);
        v8::HandleScope handle_scope(m_internalData->isolate);
        v8::Local<v8::Context> context = v8::Local<v8::Context>::New(m_internalData->isolate, m_internalData->context);
        v8::Context::Scope context_scope(context);

        return callback(context);
    }
    catch (const std::exception& e)
    {
        LogError(Stringf("執行上下文操作時發生例外: %s", e.what()));
        return false;
    }
    catch (...)
    {
        LogError("執行上下文操作時發生未知例外");
        return false;
    }
}

//----------------------------------------------------------------------------------------------------
// 設定全域變數實作
//----------------------------------------------------------------------------------------------------
void V8Subsystem::SetGlobalNumber(const std::string& name, double value)
{
    if (m_internalData->isStubMode)
    {
        LogInfo(Stringf("STUB: 設定全域數字變數：%s = %f", name.c_str(), value));
        return;
    }

    ExecuteInContext([&](v8::Local<v8::Context> context) -> bool {
        context->Global()->Set(context,
            v8::String::NewFromUtf8(m_internalData->isolate, name.c_str()).ToLocalChecked(),
            v8::Number::New(m_internalData->isolate, value)
        );
        return true;
    });
}

void V8Subsystem::SetGlobalString(const std::string& name, const std::string& value)
{
    if (m_internalData->isStubMode)
    {
        LogInfo(Stringf("STUB: 設定全域字串變數：%s = %s", name.c_str(), value.c_str()));
        return;
    }

    ExecuteInContext([&](v8::Local<v8::Context> context) -> bool {
        context->Global()->Set(context,
            v8::String::NewFromUtf8(m_internalData->isolate, name.c_str()).ToLocalChecked(),
            v8::String::NewFromUtf8(m_internalData->isolate, value.c_str()).ToLocalChecked()
        );
        return true;
    });
}

void V8Subsystem::SetGlobalBoolean(const std::string& name, bool value)
{
    if (m_internalData->isStubMode)
    {
        LogInfo(Stringf("STUB: 設定全域布林變數：%s = %s", name.c_str(), value ? "true" : "false"));
        return;
    }

    ExecuteInContext([&](v8::Local<v8::Context> context) -> bool {
        context->Global()->Set(context,
            v8::String::NewFromUtf8(m_internalData->isolate, name.c_str()).ToLocalChecked(),
            v8::Boolean::New(m_internalData->isolate, value)
        );
        return true;
    });
}

//----------------------------------------------------------------------------------------------------
// 取得全域變數實作
//----------------------------------------------------------------------------------------------------
double V8Subsystem::GetGlobalNumber(const std::string& name)
{
    if (m_internalData->isStubMode) return 0.0;

    double result = 0.0;
    ExecuteInContext([&](v8::Local<v8::Context> context) -> bool {
        v8::Local<v8::Value> value;
        if (context->Global()->Get(context, v8::String::NewFromUtf8(m_internalData->isolate, name.c_str()).ToLocalChecked()).ToLocal(&value))
        {
            if (value->IsNumber())
            {
                result = value->NumberValue(context).FromMaybe(0.0);
            }
        }
        return true;
    });
    return result;
}

std::string V8Subsystem::GetGlobalString(const std::string& name)
{
    if (m_internalData->isStubMode) return "";

    std::string result;
    ExecuteInContext([&](v8::Local<v8::Context> context) -> bool {
        v8::Local<v8::Value> value;
        if (context->Global()->Get(context, v8::String::NewFromUtf8(m_internalData->isolate, name.c_str()).ToLocalChecked()).ToLocal(&value))
        {
            if (value->IsString())
            {
                v8::String::Utf8Value utf8(m_internalData->isolate, value);
                result = *utf8;
            }
        }
        return true;
    });
    return result;
}

bool V8Subsystem::GetGlobalBoolean(const std::string& name)
{
    if (m_internalData->isStubMode) return false;

    bool result = false;
    ExecuteInContext([&](v8::Local<v8::Context> context) -> bool {
        v8::Local<v8::Value> value;
        if (context->Global()->Get(context, v8::String::NewFromUtf8(m_internalData->isolate, name.c_str()).ToLocalChecked()).ToLocal(&value))
        {
            if (value->IsBoolean())
            {
                result = value->BooleanValue(m_internalData->isolate);
            }
        }
        return true;
    });
    return result;
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