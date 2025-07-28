//----------------------------------------------------------------------------------------------------
// V8Subsystem.cpp (重構後)
// JavaScript V8 引擎子系統實作 - 不再依賴特定的遊戲類別
//----------------------------------------------------------------------------------------------------

#include "Engine/Scripting/V8Subsystem.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/FileUtils.hpp"
#include <fstream>
#include <sstream>
#include <chrono>

#include "Engine/Core/EngineCommon.hpp"

// V8 相關的 include（這些應該只在 .cpp 檔案中）
// 注意：您可能需要根據實際的 V8 設定調整這些 include


#include "v8.h"
#include "libplatform/libplatform.h"


//----------------------------------------------------------------------------------------------------
// V8 實作細節（使用 Pimpl 模式）
//----------------------------------------------------------------------------------------------------
struct V8Subsystem::V8Implementation
{
    std::unique_ptr<v8::Platform>               platform;
    v8::Isolate::CreateParams                   createParams;
    v8::Isolate*                                isolate = nullptr;
    v8::Global<v8::Context>                     context;
    std::unique_ptr<v8::ArrayBuffer::Allocator> allocator;


    bool                                  isInitialized = false;
    std::chrono::steady_clock::time_point lastExecutionStart;
};

V8Subsystem* g_theV8Subsystem = nullptr;
//----------------------------------------------------------------------------------------------------
// 靜態成員初始化
//----------------------------------------------------------------------------------------------------
V8Subsystem* V8Subsystem::s_instance = nullptr;

//----------------------------------------------------------------------------------------------------
V8Subsystem::V8Subsystem(const sV8SubsystemConfig& config)
    : m_config(config)
      , m_impl(std::make_unique<V8Implementation>())
      , m_isInitialized(false)
      , m_hasError(false)
{
    // 設定全域實例
    if (s_instance == nullptr)
    {
        s_instance = this;
    }
    else
    {
        ERROR_AND_DIE("V8Subsystem: 只能有一個 V8Subsystem 實例");
    }
}

//----------------------------------------------------------------------------------------------------
V8Subsystem::~V8Subsystem()
{
    if (m_isInitialized)
    {
        Shutdown();
    }

    // 清除全域實例
    if (s_instance == this)
    {
        s_instance = nullptr;
    }
}

//----------------------------------------------------------------------------------------------------
V8Subsystem* V8Subsystem::GetInstance()
{
    return s_instance;
}

//----------------------------------------------------------------------------------------------------
void V8Subsystem::Startup()
{
    DebuggerPrintf("V8Subsystem: 啟動中...\n");

    if (m_isInitialized)
    {
        DebuggerPrintf("V8Subsystem: 已經初始化，跳過\n");
        return;
    }

    if (!InitializeV8Engine())
    {
        HandleV8Error("V8 引擎初始化失敗");
        return;
    }

    SetupV8Bindings();

    m_isInitialized = true;
    DebuggerPrintf("V8Subsystem: 啟動完成\n");
}

//----------------------------------------------------------------------------------------------------
void V8Subsystem::Shutdown()
{
    DebuggerPrintf("V8Subsystem: 關閉中...\n");

    if (!m_isInitialized)
    {
        return;
    }
    // 清理回呼資料
    m_methodCallbacks.clear();
    m_functionCallbacks.clear();

    // 清理註冊的物件和函式
    m_scriptableObjects.clear();
    m_globalFunctions.clear();

    // 關閉 V8 引擎
    ShutdownV8Engine();

    m_isInitialized = false;
    DebuggerPrintf("V8Subsystem: 關閉完成\n");
}

//----------------------------------------------------------------------------------------------------
void V8Subsystem::Update()
{
    if (!m_isInitialized)
    {
        return;
    }

    // 這裡可以添加定期的 V8 維護工作
    // 例如：垃圾回收、統計更新等
}

//----------------------------------------------------------------------------------------------------
bool V8Subsystem::ExecuteScript(const std::string& script)
{
    if (!m_isInitialized)
    {
        HandleV8Error("V8 引擎尚未初始化");
        return false;
    }

    if (script.empty())
    {
        HandleV8Error("腳本內容為空");
        return false;
    }

    ClearError();
    m_impl->lastExecutionStart = std::chrono::steady_clock::now();


    v8::Isolate::Scope     isolateScope(m_impl->isolate);
    v8::HandleScope        handleScope(m_impl->isolate);
    v8::Local<v8::Context> context = m_impl->context.Get(m_impl->isolate);
    v8::Context::Scope     contextScope(context);

    v8::TryCatch tryCatch(m_impl->isolate);

    // 編譯腳本
    v8::Local<v8::String> source = v8::String::NewFromUtf8(m_impl->isolate, script.c_str()).ToLocalChecked();
    v8::Local<v8::Script> compiledScript;

    if (!v8::Script::Compile(context, source).ToLocal(&compiledScript))
    {
        // 編譯錯誤
        v8::String::Utf8Value error(m_impl->isolate, tryCatch.Exception());
        HandleV8Error("腳本編譯錯誤: " + std::string(*error));
        return false;
    }

    // 執行腳本
    v8::Local<v8::Value> result;
    if (!compiledScript->Run(context).ToLocal(&result))
    {
        // 執行錯誤
        v8::String::Utf8Value error(m_impl->isolate, tryCatch.Exception());
        HandleV8Error("腳本執行錯誤: " + std::string(*error));
        return false;
    }

    // 儲存結果
    if (!result->IsUndefined())
    {
        v8::String::Utf8Value resultStr(m_impl->isolate, result);
        m_lastResult = *resultStr;
    }
    else
    {
        m_lastResult.clear();
    }

    // 更新統計
    auto executionTime = std::chrono::steady_clock::now() - m_impl->lastExecutionStart;
    m_stats.scriptsExecuted++;
    m_stats.totalExecutionTime += std::chrono::duration_cast<std::chrono::milliseconds>(executionTime).count();

    return true;

    // 如果沒有編譯 V8 支援，回傳錯誤
    HandleV8Error("V8 支援未編譯進引擎");
    return false;
}

//----------------------------------------------------------------------------------------------------
bool V8Subsystem::ExecuteScriptFile(const std::string& filename)
{
    if (!m_isInitialized)
    {
        HandleV8Error("V8 引擎尚未初始化");
        return false;
    }

    std::string fullPath = ValidateScriptPath(filename);

    // 讀取檔案內容
    std::ifstream file(fullPath);
    if (!file.is_open())
    {
        HandleV8Error("無法開啟腳本檔案: " + fullPath);
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string scriptContent = buffer.str();

    if (scriptContent.empty())
    {
        HandleV8Error("腳本檔案為空: " + fullPath);
        return false;
    }

    DebuggerPrintf("V8Subsystem: 執行腳本檔案: %s\n", fullPath.c_str());
    return ExecuteScript(scriptContent);
}

//----------------------------------------------------------------------------------------------------
std::any V8Subsystem::ExecuteScriptWithResult(const std::string& script)
{
    if (ExecuteScript(script))
    {
        return std::any(m_lastResult);
    }
    return std::any{};
}

//----------------------------------------------------------------------------------------------------
bool V8Subsystem::HasError() const
{
    return m_hasError;
}

//----------------------------------------------------------------------------------------------------
std::string V8Subsystem::GetLastError() const
{
    return m_lastError;
}

//----------------------------------------------------------------------------------------------------
std::string V8Subsystem::GetLastResult() const
{
    return m_lastResult;
}

//----------------------------------------------------------------------------------------------------
bool V8Subsystem::IsInitialized() const
{
    return m_isInitialized;
}

//----------------------------------------------------------------------------------------------------
void V8Subsystem::ClearError()
{
    m_hasError = false;
    m_lastError.clear();
}

//----------------------------------------------------------------------------------------------------
void V8Subsystem::RegisterScriptableObject(const std::string& name, std::shared_ptr<IScriptableObject> object)
{
    if (!object)
    {
        HandleV8Error("嘗試註冊空的腳本物件: " + name);
        return;
    }

    DebuggerPrintf("V8Subsystem: 註冊腳本物件: %s\n", name.c_str());

    // 如果物件已存在，先移除舊的
    if (m_scriptableObjects.find(name) != m_scriptableObjects.end())
    {
        DebuggerPrintf("V8Subsystem: 警告 - 覆蓋現有的腳本物件: %s\n", name.c_str());
    }

    m_scriptableObjects[name] = object;

    // 如果 V8 已初始化，立即創建綁定
    if (m_isInitialized)
    {
        CreateObjectBindings();
    }
}

//----------------------------------------------------------------------------------------------------
void V8Subsystem::UnregisterScriptableObject(const std::string& name)
{
    auto it = m_scriptableObjects.find(name);
    if (it != m_scriptableObjects.end())
    {
        DebuggerPrintf("V8Subsystem: 取消註冊腳本物件: %s\n", name.c_str());
        m_scriptableObjects.erase(it);

        // 重新創建綁定（移除該物件）
        if (m_isInitialized)
        {
            CreateObjectBindings();
        }
    }
}

//----------------------------------------------------------------------------------------------------
void V8Subsystem::RegisterGlobalFunction(const std::string& name, ScriptFunction function)
{
    if (!function)
    {
        HandleV8Error("嘗試註冊空的全域函式: " + name);
        return;
    }

    DebuggerPrintf("V8Subsystem: 註冊全域函式: %s\n", name.c_str());

    m_globalFunctions[name] = function;

    // 如果 V8 已初始化，立即創建綁定
    if (m_isInitialized)
    {
        CreateFunctionBindings();
    }
}

//----------------------------------------------------------------------------------------------------
void V8Subsystem::UnregisterGlobalFunction(const std::string& name)
{
    auto it = m_globalFunctions.find(name);
    if (it != m_globalFunctions.end())
    {
        DebuggerPrintf("V8Subsystem: 取消註冊全域函式: %s\n", name.c_str());
        m_globalFunctions.erase(it);

        // 重新創建綁定（移除該函式）
        if (m_isInitialized)
        {
            CreateFunctionBindings();
        }
    }
}

//----------------------------------------------------------------------------------------------------
bool V8Subsystem::HasRegisteredObject(const std::string& name) const
{
    return m_scriptableObjects.find(name) != m_scriptableObjects.end();
}

//----------------------------------------------------------------------------------------------------
bool V8Subsystem::HasRegisteredFunction(const std::string& name) const
{
    return m_globalFunctions.find(name) != m_globalFunctions.end();
}

//----------------------------------------------------------------------------------------------------
std::vector<std::string> V8Subsystem::GetRegisteredObjectNames() const
{
    std::vector<std::string> names;
    names.reserve(m_scriptableObjects.size());

    for (const auto& pair : m_scriptableObjects)
    {
        names.push_back(pair.first);
    }

    return names;
}

//----------------------------------------------------------------------------------------------------
std::vector<std::string> V8Subsystem::GetRegisteredFunctionNames() const
{
    std::vector<std::string> names;
    names.reserve(m_globalFunctions.size());

    for (const auto& pair : m_globalFunctions)
    {
        names.push_back(pair.first);
    }

    return names;
}

//----------------------------------------------------------------------------------------------------
void V8Subsystem::SetDebugOutput(bool enabled)
{
    m_config.enableConsoleOutput = enabled;
}

//----------------------------------------------------------------------------------------------------
V8Subsystem::ExecutionStats V8Subsystem::GetExecutionStats() const
{
    return m_stats;
}

//----------------------------------------------------------------------------------------------------
void V8Subsystem::ResetExecutionStats()
{
    m_stats = ExecutionStats{};
}

//----------------------------------------------------------------------------------------------------
void V8Subsystem::ForceGarbageCollection()
{
    if (m_isInitialized && m_impl->isolate)
    {
        m_impl->isolate->RequestGarbageCollectionForTesting(v8::Isolate::kFullGarbageCollection);
        DebuggerPrintf("V8Subsystem: 強制執行垃圾回收\n");
    }
}

//----------------------------------------------------------------------------------------------------
V8Subsystem::MemoryUsage V8Subsystem::GetMemoryUsage() const
{
    MemoryUsage usage;


    if (m_isInitialized && m_impl->isolate)
    {
        v8::HeapStatistics stats;
        m_impl->isolate->GetHeapStatistics(&stats);

        usage.usedHeapSize  = stats.used_heap_size();
        usage.totalHeapSize = stats.total_heap_size();
        usage.heapSizeLimit = stats.heap_size_limit();

        if (usage.heapSizeLimit > 0)
        {
            usage.usagePercentage = (double)usage.usedHeapSize / usage.heapSizeLimit * 100.0;
        }
    }


    return usage;
}

//----------------------------------------------------------------------------------------------------
// 私有方法實作
//----------------------------------------------------------------------------------------------------

bool V8Subsystem::InitializeV8Engine()
{
    DebuggerPrintf("V8Subsystem: 初始化 V8 引擎...\n");

    // 初始化 V8 平台
    v8::V8::InitializeICUDefaultLocation("");
    v8::V8::InitializeExternalStartupData("");
    m_impl->platform = v8::platform::NewDefaultPlatform();
    v8::V8::InitializePlatform(m_impl->platform.get());
    v8::V8::Initialize();

    // 創建 Isolate
    m_impl->allocator.reset(v8::ArrayBuffer::Allocator::NewDefaultAllocator());
    m_impl->createParams.array_buffer_allocator = m_impl->allocator.get();

    // 設定堆疊大小限制
    // 修正 2: 根據 V8 版本使用不同的記憶體限制設定方法
    if (m_config.heapSizeLimit > 0)
    {
        // 嘗試新版本的 API
#if V8_MAJOR_VERSION >= 9
        m_impl->createParams.constraints.set_max_old_generation_size_in_bytes(m_config.heapSizeLimit);
        // m_impl->createParams.constraints.set_max_old_space_size(m_config.heapSizeLimit);
#else
        // 舊版本的 API
        m_impl->createParams.constraints.ConfigureDefaults(
            v8::internal::Heap::kPointerMultiplier * m_config.heapSizeLimit * v8::internal::MB,
            v8::internal::Heap::kPointerMultiplier * m_config.heapSizeLimit * v8::internal::MB);
#endif
    }

    m_impl->isolate = v8::Isolate::New(m_impl->createParams);
    if (!m_impl->isolate)
    {
        DebuggerPrintf("V8Subsystem: 無法創建 V8 Isolate\n");
        return false;
    }

    // 創建 Context
    {
        v8::Isolate::Scope isolateScope(m_impl->isolate);
        v8::HandleScope    handleScope(m_impl->isolate);

        v8::Local<v8::Context> context = v8::Context::New(m_impl->isolate);
        m_impl->context.Reset(m_impl->isolate, context);
    }

    m_impl->isInitialized = true;
    DebuggerPrintf("V8Subsystem: V8 引擎初始化成功\n");
    return true;

    DebuggerPrintf("V8Subsystem: V8 支援未編譯進引擎\n");
    return false;
}

//----------------------------------------------------------------------------------------------------
void V8Subsystem::ShutdownV8Engine()
{
    if (m_impl->isInitialized)
    {
        // 清理 Context
        m_impl->context.Reset();

        // 清理 Isolate
        if (m_impl->isolate)
        {
            m_impl->isolate->Dispose();
            m_impl->isolate = nullptr;
        }

        // 清理 Allocator
        if (m_impl->allocator)
        {
            delete m_impl->allocator.release();
        }

        // 清理平台
        v8::V8::Dispose();
        v8::V8::DisposePlatform();

        m_impl->isInitialized = false;
        DebuggerPrintf("V8Subsystem: V8 引擎已關閉\n");
    }
}

//----------------------------------------------------------------------------------------------------
void V8Subsystem::SetupV8Bindings()
{
    if (!m_isInitialized)
    {
        return;
    }

    DebuggerPrintf("V8Subsystem: 設定 V8 綁定...\n");

    SetupBuiltinObjects();
    CreateObjectBindings();
    CreateFunctionBindings();

    DebuggerPrintf("V8Subsystem: V8 綁定設定完成\n");
}

//----------------------------------------------------------------------------------------------------
void V8Subsystem::CreateObjectBindings()
{
    if (!m_impl->isolate)
        return;

    v8::Isolate::Scope isolateScope(m_impl->isolate);
    v8::HandleScope handleScope(m_impl->isolate);
    v8::Local<v8::Context> context = m_impl->context.Get(m_impl->isolate);
    v8::Context::Scope contextScope(context);

    v8::Local<v8::Object> global = context->Global();

    for (const auto& pair : m_scriptableObjects)
    {
        const std::string& objectName = pair.first;
        std::shared_ptr<IScriptableObject> object = pair.second;

        DebuggerPrintf("V8Subsystem: 創建 V8 綁定 - 物件: %s\n", objectName.c_str());

        // 創建 JavaScript 物件
        v8::Local<v8::Object> jsObject = v8::Object::New(m_impl->isolate);

        // 取得物件的可用方法
        auto methods = object->GetAvailableMethods();

        for (const auto& method : methods)
        {
            DebuggerPrintf("V8Subsystem: 綁定方法 %s.%s\n", objectName.c_str(), method.name.c_str());

            // 為每個方法創建 V8 函式回呼
            auto methodCallback = [](const v8::FunctionCallbackInfo<v8::Value>& args) {
                v8::Isolate* isolate = args.GetIsolate();
                v8::HandleScope scope(isolate);

                // 從函式的內部欄位取得物件和方法名稱
                v8::Local<v8::External> objectExternal = v8::Local<v8::External>::Cast(args.Data());
                auto* callbackData = static_cast<MethodCallbackData*>(objectExternal->Value());

                // 轉換參數
                std::vector<std::any> cppArgs;
                for (int i = 0; i < args.Length(); i++)
                {
                    v8::Local<v8::Value> arg = args[i];
                    if (arg->IsNumber())
                    {
                        double num = arg->NumberValue(isolate->GetCurrentContext()).ToChecked();
                        cppArgs.push_back(num);
                    }
                    else if (arg->IsString())
                    {
                        v8::String::Utf8Value str(isolate, arg);
                        cppArgs.push_back(std::string(*str));
                    }
                    else if (arg->IsBoolean())
                    {
                        cppArgs.push_back(arg->BooleanValue(isolate));
                    }
                }

                // 呼叫 C++ 方法
                ScriptMethodResult result = callbackData->object->CallMethod(callbackData->methodName, cppArgs);

                if (result.success)
                {
                    // 轉換回傳值
                    try
                    {
                        if (result.result.type() == typeid(std::string))
                        {
                            std::string str = std::any_cast<std::string>(result.result);
                            args.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, str.c_str()).ToLocalChecked());
                        }
                        else if (result.result.type() == typeid(bool))
                        {
                            bool value = std::any_cast<bool>(result.result);
                            args.GetReturnValue().Set(v8::Boolean::New(isolate, value));
                        }
                        else if (result.result.type() == typeid(double) || result.result.type() == typeid(float))
                        {
                            double value = std::any_cast<double>(result.result);
                            args.GetReturnValue().Set(v8::Number::New(isolate, value));
                        }
                        else
                        {
                            args.GetReturnValue().Set(v8::Undefined(isolate));
                        }
                    }
                    catch (const std::bad_any_cast&)
                    {
                        args.GetReturnValue().Set(v8::Undefined(isolate));
                    }
                }
                else
                {
                    // 拋出 JavaScript 錯誤
                    isolate->ThrowException(v8::String::NewFromUtf8(isolate, result.errorMessage.c_str()).ToLocalChecked());
                }
            };

            // 創建回呼資料
            auto callbackData = std::make_unique<MethodCallbackData>();
            callbackData->object = object;
            callbackData->methodName = method.name;

            v8::Local<v8::External> external = v8::External::New(m_impl->isolate, callbackData.get());

            // 修正：直接創建函式，而不是使用 FunctionTemplate
            v8::Local<v8::Function> methodFunction = v8::Function::New(context, methodCallback, external).ToLocalChecked();

            // 將方法添加到 JavaScript 物件
            jsObject->Set(context,
                         v8::String::NewFromUtf8(m_impl->isolate, method.name.c_str()).ToLocalChecked(),
                         methodFunction).Check();

            // 儲存回呼資料避免被釋放
            m_methodCallbacks.push_back(std::move(callbackData));
        }

        // 將物件綁定到全域範圍
        global->Set(context,
                   v8::String::NewFromUtf8(m_impl->isolate, objectName.c_str()).ToLocalChecked(),
                   jsObject).Check();

        DebuggerPrintf("V8Subsystem: 物件 %s 已綁定到 JavaScript 全域範圍\n", objectName.c_str());
    }
}

//----------------------------------------------------------------------------------------------------
void V8Subsystem::CreateFunctionBindings()
{
    if (!m_impl->isolate)
        return;

    v8::Isolate::Scope isolateScope(m_impl->isolate);
    v8::HandleScope handleScope(m_impl->isolate);
    v8::Local<v8::Context> context = m_impl->context.Get(m_impl->isolate);
    v8::Context::Scope contextScope(context);

    v8::Local<v8::Object> global = context->Global();

    for (const auto& pair : m_globalFunctions)
    {
        const std::string& functionName = pair.first;
        const ScriptFunction& function = pair.second;

        DebuggerPrintf("V8Subsystem: 綁定全域函式: %s\n", functionName.c_str());

        // 創建函式回呼
        auto functionCallback = [](const v8::FunctionCallbackInfo<v8::Value>& args) {
            v8::Isolate* isolate = args.GetIsolate();
            v8::HandleScope scope(isolate);

            // 從回呼資料取得 C++ 函式
            v8::Local<v8::External> external = v8::Local<v8::External>::Cast(args.Data());
            auto* function = static_cast<ScriptFunction*>(external->Value());

            // 轉換參數
            std::vector<std::any> cppArgs;
            for (int i = 0; i < args.Length(); i++)
            {
                v8::Local<v8::Value> arg = args[i];
                if (arg->IsNumber())
                {
                    double num = arg->NumberValue(isolate->GetCurrentContext()).ToChecked();
                    cppArgs.push_back(num);
                }
                else if (arg->IsString())
                {
                    v8::String::Utf8Value str(isolate, arg);
                    cppArgs.push_back(std::string(*str));
                }
                else if (arg->IsBoolean())
                {
                    cppArgs.push_back(arg->BooleanValue(isolate));
                }
            }

            // 呼叫 C++ 函式
            try
            {
                std::any result = (*function)(cppArgs);

                // 轉換回傳值（如果有的話）
                if (result.type() == typeid(std::string))
                {
                    std::string str = std::any_cast<std::string>(result);
                    args.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, str.c_str()).ToLocalChecked());
                }
                else
                {
                    args.GetReturnValue().Set(v8::Undefined(isolate));
                }
            }
            catch (const std::exception& e)
            {
                isolate->ThrowException(v8::String::NewFromUtf8(isolate, e.what()).ToLocalChecked());
            }
        };

        // 創建外部資料來儲存函式指標
        auto functionPtr = std::make_unique<ScriptFunction>(function);
        v8::Local<v8::External> external = v8::External::New(m_impl->isolate, functionPtr.get());

        // 修正：直接創建函式
        v8::Local<v8::Function> jsFunction = v8::Function::New(context, functionCallback, external).ToLocalChecked();

        // 將函式綁定到全域範圍
        global->Set(context,
                   v8::String::NewFromUtf8(m_impl->isolate, functionName.c_str()).ToLocalChecked(),
                   jsFunction).Check();

        // 儲存函式指標避免被釋放
        m_functionCallbacks.push_back(std::move(functionPtr));
    }
}

//----------------------------------------------------------------------------------------------------
void V8Subsystem::SetupBuiltinObjects()
{
    if (!m_impl->isolate)
        return;

    v8::Isolate::Scope isolateScope(m_impl->isolate);
    v8::HandleScope handleScope(m_impl->isolate);
    v8::Local<v8::Context> context = m_impl->context.Get(m_impl->isolate);
    v8::Context::Scope contextScope(context);

    if (m_config.enableConsoleOutput)
    {
        DebuggerPrintf("V8Subsystem: 設定 console 物件\n");

        // 創建 console 物件
        v8::Local<v8::Object> console = v8::Object::New(m_impl->isolate);

        // 創建 console.log 方法回呼
        auto logCallback = [](const v8::FunctionCallbackInfo<v8::Value>& args) {
            v8::Isolate* isolate = args.GetIsolate();
            v8::HandleScope scope(isolate);

            std::string output = "CONSOLE: ";
            for (int i = 0; i < args.Length(); i++)
            {
                if (i > 0) output += " ";

                v8::Local<v8::Value> arg = args[i];
                if (arg->IsString())
                {
                    v8::String::Utf8Value str(isolate, arg);
                    output += *str;
                }
                else if (arg->IsNumber())
                {
                    double num = arg->NumberValue(isolate->GetCurrentContext()).ToChecked();
                    output += std::to_string(num);
                }
                else if (arg->IsBoolean())
                {
                    bool val = arg->BooleanValue(isolate);
                    output += val ? "true" : "false";
                }
                else
                {
                    output += "[object]";
                }
            }

            DebuggerPrintf("%s\n", output.c_str());
        };

        // 修正：直接創建函式
        v8::Local<v8::Function> logFunction = v8::Function::New(context, logCallback).ToLocalChecked();
        console->Set(context,
                    v8::String::NewFromUtf8(m_impl->isolate, "log").ToLocalChecked(),
                    logFunction).Check();

        // 將 console 物件綁定到全域範圍
        v8::Local<v8::Object> global = context->Global();
        global->Set(context,
                   v8::String::NewFromUtf8(m_impl->isolate, "console").ToLocalChecked(),
                   console).Check();
    }
}

//----------------------------------------------------------------------------------------------------
void V8Subsystem::HandleV8Error(const std::string& error)
{
    m_hasError  = true;
    m_lastError = error;
    m_stats.errorsEncountered++;

    DebuggerPrintf("V8Subsystem 錯誤: %s\n", error.c_str());
}

//----------------------------------------------------------------------------------------------------
void* V8Subsystem::ConvertToV8Value(const std::any& value)
{
    // 實作 C++ std::any 到 V8 值的轉換
    // 這是一個複雜的函式，需要處理各種類型
    UNUSED(value);
    return nullptr; // 占位符
}

//----------------------------------------------------------------------------------------------------
std::any V8Subsystem::ConvertFromV8Value(void* v8Value)
{
    // 實作 V8 值到 C++ std::any 的轉換
    UNUSED(v8Value);
    return std::any{}; // 占位符
}

//----------------------------------------------------------------------------------------------------
std::string V8Subsystem::ValidateScriptPath(const std::string& filename) const
{
    std::string fullPath;

    // 檢查是否已經是完整路徑
    if (filename.find(':') != std::string::npos || filename[0] == '/' || filename[0] == '\\')
    {
        fullPath = filename;
    }
    else
    {
        // 檢查檔案名是否已經包含路徑前綴
        if (filename.find(m_config.scriptPath) == 0)
        {
            // 已經包含路徑前綴，直接使用
            fullPath = filename;
        }
        else
        {
            // 沒有路徑前綴，添加它
            fullPath = m_config.scriptPath + filename;
        }
    }

    // 確保副檔名是 .js
    if (fullPath.length() < 3 || fullPath.substr(fullPath.length() - 3) != ".js")
    {
        fullPath += ".js";
    }

    return fullPath;
}
