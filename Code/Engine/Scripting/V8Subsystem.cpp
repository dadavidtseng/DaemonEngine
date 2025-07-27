//----------------------------------------------------------------------------------------------------
// V8Subsystem.cpp - 整合 JavaScriptManager 功能的實作
//----------------------------------------------------------------------------------------------------

#include "Engine/Scripting/V8Subsystem.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Math/Vec3.hpp"
#include <fstream>

// 遊戲相關標頭檔
#include "Game/Game.hpp"
#include "Game/Prop.hpp"
#include "Game/Player.hpp"

// V8 標頭檔
#pragma warning(push)
#pragma warning(disable: 4100 4324 4127)
#include "ThirdParty/packages/v8-v143-x64.13.0.245.25/include/v8.h"
#include "ThirdParty/packages/v8-v143-x64.13.0.245.25/include/libplatform/libplatform.h"
#pragma warning(pop)

//----------------------------------------------------------------------------------------------------
// 全域變數
V8Subsystem* g_theV8Subsystem = nullptr;

// 靜態成員初始化
v8::Platform* V8Subsystem::s_platform = nullptr;
int V8Subsystem::s_instanceCount = 0;

//----------------------------------------------------------------------------------------------------
V8Subsystem::V8Subsystem(sV8SubsystemConfig const& config)
    : m_config(config)
    , m_isInitialized(false)
    , m_gameReference(nullptr)
    , m_isolate(nullptr)
    , m_context(nullptr)
{
}

//----------------------------------------------------------------------------------------------------
V8Subsystem::~V8Subsystem()
{
    Shutdown();
}

//----------------------------------------------------------------------------------------------------
void V8Subsystem::Startup()
{
    DebuggerPrintf("V8Subsystem 啟動中...\n");

    if (!InitializeV8())
    {
        LogError("V8 初始化失敗");
        return;
    }

    CreateGlobalObjects();

    m_isInitialized = true;
    DebuggerPrintf("V8Subsystem 啟動成功！\n");
}

//----------------------------------------------------------------------------------------------------
void V8Subsystem::Shutdown()
{
    if (!m_isInitialized)
        return;

    DebuggerPrintf("V8Subsystem 關閉中...\n");

    UnbindGameObjects();
    ShutdownV8();

    m_isInitialized = false;
    DebuggerPrintf("V8Subsystem 已關閉。\n");
}

//----------------------------------------------------------------------------------------------------
void V8Subsystem::BeginFrame()
{
    // V8 每幀處理（如垃圾回收提示等）
    if (m_isInitialized && m_isolate)
    {
        // 可選：定期觸發垃圾回收
        // m_isolate->LowMemoryNotification();
    }
}

//----------------------------------------------------------------------------------------------------
void V8Subsystem::EndFrame()
{
    // 清理每幀錯誤狀態
    if (!m_lastError.empty())
    {
        // 可選：在每幀結束後清理錯誤訊息
        // m_lastError.clear();
    }
}

//----------------------------------------------------------------------------------------------------
bool V8Subsystem::InitializeV8()
{
    try
    {
        // 初始化 V8 平台（全域，只需要一次）
        if (s_platform == nullptr)
        {
            v8::V8::InitializeICUDefaultLocation("");
            v8::V8::InitializeExternalStartupData("");
            s_platform = v8::platform::NewDefaultPlatform().release();
            v8::V8::InitializePlatform(s_platform);
            v8::V8::Initialize();
            DebuggerPrintf("V8 平台初始化完成\n");
        }
        s_instanceCount++;

        // 建立 Isolate
        v8::Isolate::CreateParams createParams;
        createParams.array_buffer_allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();
        m_isolate = v8::Isolate::New(createParams);

        if (!m_isolate)
        {
            LogError("無法建立 V8 Isolate");
            return false;
        }

        // 進入 Isolate 範圍
        v8::Isolate::Scope isolateScope(m_isolate);
        v8::HandleScope handleScope(m_isolate);

        // 建立上下文
        v8::Local<v8::Context> context = v8::Context::New(m_isolate);
        if (context.IsEmpty())
        {
            LogError("無法建立 V8 Context");
            return false;
        }

        // 儲存持久性上下文
        m_context = new v8::Global<v8::Context>(m_isolate, context);

        DebuggerPrintf("V8 引擎初始化成功\n");
        return true;
    }
    catch (const std::exception& e)
    {
        LogError(Stringf("V8 初始化例外: %s", e.what()));
        return false;
    }
    catch (...)
    {
        LogError("V8 初始化發生未知例外");
        return false;
    }
}

//----------------------------------------------------------------------------------------------------
void V8Subsystem::ShutdownV8()
{
    if (m_context)
    {
        m_context->Reset();
        delete m_context;
        m_context = nullptr;
    }

    if (m_isolate)
    {
        m_isolate->Dispose();
        m_isolate = nullptr;
    }

    s_instanceCount--;
    if (s_instanceCount <= 0)
    {
        v8::V8::Dispose();
        v8::V8::DisposePlatform();
        delete s_platform;
        s_platform = nullptr;
        s_instanceCount = 0;
        DebuggerPrintf("V8 平台已清理\n");
    }
}

//----------------------------------------------------------------------------------------------------
void V8Subsystem::CreateGlobalObjects()
{
    if (!m_isInitialized || !m_isolate || !m_context)
        return;

    ExecuteInContext([&](v8::Local<v8::Context> context) -> bool {
        v8::Isolate* isolate = m_isolate;

        // 建立 console 物件
        v8::Local<v8::Object> console = v8::Object::New(isolate);
        BindFunction(context, console, "log", JSLog);

        // 將 console 設為全域物件
        context->Global()->Set(context,
            v8::String::NewFromUtf8(isolate, "console").ToLocalChecked(),
            console
        );

        DebuggerPrintf("全域 JavaScript 物件建立完成\n");
        return true;
    });
}

//----------------------------------------------------------------------------------------------------
bool V8Subsystem::ExecuteScript(const std::string& script)
{
    if (!m_isInitialized)
    {
        DebuggerPrintf("V8Subsystem 未初始化，使用 fallback 模式\n");
        return ExecuteFallbackScript(script);
    }

    if (!m_isolate || !m_context)
    {
        LogError("V8 狀態無效");
        return false;
    }

    return ExecuteInContext([&](v8::Local<v8::Context> context) -> bool {
        try
        {
            v8::Isolate* isolate = m_isolate;
            v8::TryCatch tryCatch(isolate);

            // 編譯腳本
            v8::Local<v8::String> source = v8::String::NewFromUtf8(isolate, script.c_str()).ToLocalChecked();
            v8::Local<v8::Script> compiledScript;

            if (!v8::Script::Compile(context, source).ToLocal(&compiledScript))
            {
                if (tryCatch.HasCaught())
                {
                    v8::String::Utf8Value error(isolate, tryCatch.Exception());
                    LogError(Stringf("腳本編譯錯誤: %s", *error));
                }
                return false;
            }

            // 執行腳本
            v8::Local<v8::Value> result;
            if (!compiledScript->Run(context).ToLocal(&result))
            {
                if (tryCatch.HasCaught())
                {
                    v8::String::Utf8Value error(isolate, tryCatch.Exception());
                    LogError(Stringf("腳本執行錯誤: %s", *error));
                }
                return false;
            }

            // 儲存結果
            if (!result->IsUndefined())
            {
                v8::String::Utf8Value resultStr(isolate, result);
                m_lastResult = *resultStr;
            }
            else
            {
                m_lastResult = "";
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
    });
}

//----------------------------------------------------------------------------------------------------
bool V8Subsystem::ExecuteScriptFile(const std::string& filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        LogError(Stringf("無法開啟腳本檔案: %s", filename.c_str()));
        return false;
    }

    std::string script;
    std::string line;
    while (std::getline(file, line))
    {
        script += line + "\n";
    }
    file.close();

    DebuggerPrintf("載入腳本檔案: %s (%zu 字元)\n", filename.c_str(), script.length());
    return ExecuteScript(script);
}

//----------------------------------------------------------------------------------------------------
void V8Subsystem::BindGameObjects(Game* game)
{
    if (!m_config.enableGameBindings)
    {
        DebuggerPrintf("遊戲物件綁定已停用\n");
        return;
    }

    m_gameReference = game;

    if (!m_isInitialized)
    {
        DebuggerPrintf("V8Subsystem 未初始化，跳過遊戲物件綁定\n");
        return;
    }

    ExecuteInContext([&](v8::Local<v8::Context> context) -> bool {
        try
        {
            v8::Isolate* isolate = m_isolate;

            // 建立 Game 物件
            v8::Local<v8::Object> gameObj = v8::Object::New(isolate);

            // 綁定遊戲函數
            BindFunction(context, gameObj, "createCube", JSCreateCube);
            BindFunction(context, gameObj, "moveProp", JSMoveProp);
            BindFunction(context, gameObj, "getPlayerPos", JSGetPlayerPosition);
            BindFunction(context, gameObj, "setPlayerPos", JSSetPlayerPosition);

            // 將 Game 物件設為全域變數
            context->Global()->Set(context,
                v8::String::NewFromUtf8(isolate, "Game").ToLocalChecked(),
                gameObj
            );

            DebuggerPrintf("遊戲物件成功綁定到 JavaScript！\n");
            DebuggerPrintf("可用函數: Game.createCube(), Game.moveProp(), Game.getPlayerPos(), Game.setPlayerPos()\n");
            return true;
        }
        catch (const std::exception& e)
        {
            LogError(Stringf("綁定遊戲物件時發生例外: %s", e.what()));
            return false;
        }
        catch (...)
        {
            LogError("綁定遊戲物件時發生未知例外");
            return false;
        }
    });
}

//----------------------------------------------------------------------------------------------------
void V8Subsystem::UnbindGameObjects()
{
    m_gameReference = nullptr;
    // 在實際實作中，這裡可以移除 JavaScript 中的 Game 物件
}

//----------------------------------------------------------------------------------------------------
bool V8Subsystem::ExecuteInContext(std::function<bool(v8::Local<v8::Context>)> callback)
{
    if (!m_isolate || !m_context)
        return false;

    v8::Isolate::Scope isolateScope(m_isolate);
    v8::HandleScope handleScope(m_isolate);
    v8::Local<v8::Context> context = v8::Local<v8::Context>::New(m_isolate, *m_context);
    v8::Context::Scope contextScope(context);

    return callback(context);
}

//----------------------------------------------------------------------------------------------------
bool V8Subsystem::BindFunction(v8::Local<v8::Context> context, v8::Local<v8::Object> object,
                              const char* name, v8::FunctionCallback callback)
{
    try
    {
        v8::Isolate* isolate = m_isolate;
        v8::MaybeLocal<v8::Function> func = v8::Function::New(context, callback);
        if (!func.IsEmpty())
        {
            object->Set(context,
                v8::String::NewFromUtf8(isolate, name).ToLocalChecked(),
                func.ToLocalChecked()
            );
            return true;
        }
    }
    catch (...)
    {
        LogError(Stringf("綁定函數失敗: %s", name));
    }
    return false;
}

//----------------------------------------------------------------------------------------------------
void V8Subsystem::LogError(const std::string& error)
{
    m_lastError = error;
    DebuggerPrintf("V8Subsystem 錯誤: %s\n", error.c_str());
}

//----------------------------------------------------------------------------------------------------
bool V8Subsystem::ExecuteFallbackScript(const std::string& script)
{
    DebuggerPrintf("FALLBACK: 執行 JavaScript: %s\n", script.c_str());

    // 這裡保留你原本的 fallback 邏輯
    if (script.find("console.log") != std::string::npos)
    {
        size_t start = script.find("console.log(");
        if (start != std::string::npos)
        {
            start += 12;
            size_t end = script.find(")", start);
            if (end != std::string::npos)
            {
                std::string logContent = script.substr(start, end - start);
                if (!logContent.empty())
                {
                    if ((logContent.front() == '\'' && logContent.back() == '\'') ||
                        (logContent.front() == '"' && logContent.back() == '"'))
                    {
                        logContent = logContent.substr(1, logContent.size() - 2);
                    }
                }
                DebuggerPrintf("JS FALLBACK LOG: %s\n", logContent.c_str());
            }
        }
    }

    // 其他 fallback 邏輯...
    if (script.find("Game.createCube") != std::string::npos)
    {
        DebuggerPrintf("JS FALLBACK: Would create cube\n");
    }

    if (script.find("Game.getPlayerPos") != std::string::npos)
    {
        DebuggerPrintf("JS FALLBACK: Would get player position\n");
        m_lastResult = "{x: -2, y: 0, z: 1}";
    }

    return true;
}

//----------------------------------------------------------------------------------------------------
// JavaScript 回呼函數實作（原 JavaScriptManager 的函數）
//----------------------------------------------------------------------------------------------------

void V8Subsystem::JSLog(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    std::string output = "JS: ";

    for (int i = 0; i < args.Length(); i++)
    {
        if (i > 0) output += " ";
        v8::String::Utf8Value str(args.GetIsolate(), args[i]);
        output += *str;
    }

    DebuggerPrintf("%s\n", output.c_str());
}

//----------------------------------------------------------------------------------------------------
void V8Subsystem::JSCreateCube(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    if (!g_theV8Subsystem || !g_theV8Subsystem->m_gameReference)
    {
        DebuggerPrintf("JSCreateCube: 遊戲參考無效\n");
        return;
    }

    float x = 0.0f, y = 0.0f, z = 0.0f;

    if (args.Length() >= 3)
    {
        x = (float)args[0]->NumberValue(args.GetIsolate()->GetCurrentContext()).FromMaybe(0.0);
        y = (float)args[1]->NumberValue(args.GetIsolate()->GetCurrentContext()).FromMaybe(0.0);
        z = (float)args[2]->NumberValue(args.GetIsolate()->GetCurrentContext()).FromMaybe(0.0);
    }

    DebuggerPrintf("JS: 在位置 (%.2f, %.2f, %.2f) 建立方塊\n", x, y, z);
    // 實際遊戲邏輯呼叫
    g_theV8Subsystem->m_gameReference->CreateCube(Vec3(x, y, z));

}

//----------------------------------------------------------------------------------------------------
void V8Subsystem::JSMoveProp(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    if (!g_theV8Subsystem || !g_theV8Subsystem->m_gameReference)
    {
        DebuggerPrintf("JSMoveProp: 遊戲參考無效\n");
        return;
    }

    if (args.Length() >= 4)
    {
        int propIndex = (int)args[0]->Int32Value(args.GetIsolate()->GetCurrentContext()).FromMaybe(0);
        float x = (float)args[1]->NumberValue(args.GetIsolate()->GetCurrentContext()).FromMaybe(0.0);
        float y = (float)args[2]->NumberValue(args.GetIsolate()->GetCurrentContext()).FromMaybe(0.0);
        float z = (float)args[3]->NumberValue(args.GetIsolate()->GetCurrentContext()).FromMaybe(0.0);

        DebuggerPrintf("JS: 移動物件 %d 到位置 (%.2f, %.2f, %.2f)\n", propIndex, x, y, z);
        g_theV8Subsystem->m_gameReference->MoveProp(propIndex, Vec3(x, y, z));
    }
}

//----------------------------------------------------------------------------------------------------
void V8Subsystem::JSGetPlayerPosition(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    if (!g_theV8Subsystem || !g_theV8Subsystem->m_gameReference)
    {
        DebuggerPrintf("JSGetPlayerPosition: 遊戲參考無效\n");
        return;
    }

    Player* player = g_theV8Subsystem->m_gameReference->GetPlayer();
    Vec3 pos = player ? player->m_position : Vec3(-2.0f, 0.0f, 1.0f);

    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    v8::Local<v8::Object> posObj = v8::Object::New(isolate);

    posObj->Set(context, v8::String::NewFromUtf8(isolate, "x").ToLocalChecked(), v8::Number::New(isolate, pos.x));
    posObj->Set(context, v8::String::NewFromUtf8(isolate, "y").ToLocalChecked(), v8::Number::New(isolate, pos.y));
    posObj->Set(context, v8::String::NewFromUtf8(isolate, "z").ToLocalChecked(), v8::Number::New(isolate, pos.z));

    args.GetReturnValue().Set(posObj);
    DebuggerPrintf("JS: 取得玩家位置 (%.2f, %.2f, %.2f)\n", pos.x, pos.y, pos.z);
}

//----------------------------------------------------------------------------------------------------
void V8Subsystem::JSSetPlayerPosition(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    if (!g_theV8Subsystem || !g_theV8Subsystem->m_gameReference)
    {
        DebuggerPrintf("JSSetPlayerPosition: 遊戲參考無效\n");
        return;
    }

    if (args.Length() >= 3)
    {
        float x = (float)args[0]->NumberValue(args.GetIsolate()->GetCurrentContext()).FromMaybe(0.0);
        float y = (float)args[1]->NumberValue(args.GetIsolate()->GetCurrentContext()).FromMaybe(0.0);
        float z = (float)args[2]->NumberValue(args.GetIsolate()->GetCurrentContext()).FromMaybe(0.0);

        DebuggerPrintf("JS: 設定玩家位置為 (%.2f, %.2f, %.2f)\n", x, y, z);

        Player* player = g_theV8Subsystem->m_gameReference->GetPlayer();
        if (player)
        {
            player->m_position = Vec3(x, y, z);
            DebuggerPrintf("JS: 玩家位置設定成功\n");
        }
    }
}