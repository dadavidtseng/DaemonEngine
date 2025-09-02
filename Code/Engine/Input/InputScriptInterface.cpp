//----------------------------------------------------------------------------------------------------
// InputScriptInterface.cpp
// InputSystem 的腳本介面包裝器實作
//----------------------------------------------------------------------------------------------------

#include "Engine/Input/InputScriptInterface.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include <stdexcept>
#include <sstream>

//----------------------------------------------------------------------------------------------------
InputScriptInterface::InputScriptInterface(InputSystem* inputSystem)
    : m_inputSystem(inputSystem)
{
    if (!m_inputSystem)
    {
        ERROR_AND_DIE("InputScriptInterface: InputSystem pointer cannot be null")
    }
}

//----------------------------------------------------------------------------------------------------
std::string InputScriptInterface::GetScriptObjectName() const
{
    return "input";
}

//----------------------------------------------------------------------------------------------------
std::vector<ScriptMethodInfo> InputScriptInterface::GetAvailableMethods() const
{
    return {
        ScriptMethodInfo("isKeyDown",
                         "檢查指定按鍵是否正在被按下",
                         {"int"},
                         "bool"),

        ScriptMethodInfo("wasKeyJustPressed",
                         "檢查指定按鍵是否剛被按下",
                         {"int"},
                         "bool"),

        ScriptMethodInfo("wasKeyJustReleased",
                         "檢查指定按鍵是否剛被釋放",
                         {"int"},
                         "bool"),

        ScriptMethodInfo("getCursorDelta",
                         "取得滑鼠游標移動增量",
                         {},
                         "object"),

        ScriptMethodInfo("getCursorPosition",
                         "取得滑鼠游標目前位置",
                         {},
                         "object"),

        ScriptMethodInfo("getController",
                         "取得指定索引的控制器",
                         {"int"},
                         "object"),

        ScriptMethodInfo("setCursorMode",
                         "設定游標模式 (0=POINTER, 1=FPS)",
                         {"int"},
                         "void")
    };
}

//----------------------------------------------------------------------------------------------------
std::vector<std::string> InputScriptInterface::GetAvailableProperties() const
{
    return {
        "cursorDelta",
        "cursorPosition"
    };
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult InputScriptInterface::CallMethod(std::string const&           methodName,
                                                     std::vector<std::any> const& args)
{
    try
    {
        if (methodName == "isKeyDown")
        {
            return ExecuteIsKeyDown(args);
        }
        else if (methodName == "wasKeyJustPressed")
        {
            return ExecuteWasKeyJustPressed(args);
        }
        else if (methodName == "wasKeyJustReleased")
        {
            return ExecuteWasKeyJustReleased(args);
        }
        else if (methodName == "getCursorDelta")
        {
            return ExecuteGetCursorClientDelta(args);
        }
        else if (methodName == "getCursorPosition")
        {
            return ExecuteGetCursorClientPosition(args);
        }
        else if (methodName == "getController")
        {
            return ExecuteGetController(args);
        }
        else if (methodName == "setCursorMode")
        {
            return ExecuteSetCursorMode(args);
        }

        return ScriptMethodResult::Error("未知的方法: " + methodName);
    }
    catch (std::exception const& e)
    {
        return ScriptMethodResult::Error("方法執行時發生例外: " + std::string(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
std::any InputScriptInterface::GetProperty(const std::string& propertyName) const
{
    if (propertyName == "cursorDelta")
    {
        Vec2 delta = m_inputSystem->GetCursorClientDelta();
        return std::string("{ x: " + std::to_string(delta.x) + ", y: " + std::to_string(delta.y) + " }");
    }
    else if (propertyName == "cursorPosition")
    {
        Vec2 position = m_inputSystem->GetCursorClientPosition();
        return std::string("{ x: " + std::to_string(position.x) + ", y: " + std::to_string(position.y) + " }");
    }

    return std::any{};
}

//----------------------------------------------------------------------------------------------------
bool InputScriptInterface::SetProperty(const std::string& propertyName, const std::any& value)
{
    // InputSystem 目前沒有可設定的屬性
    UNUSED(propertyName);
    UNUSED(value);
    return false;
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult InputScriptInterface::ExecuteIsKeyDown(const std::vector<std::any>& args)
{
    auto result = ValidateArgCount(args, 1, "isKeyDown");
    if (!result.success) return result;

    try
    {
        int keyCode = ExtractInt(args[0]);
        bool isDown = m_inputSystem->IsKeyDown(keyCode);
        return ScriptMethodResult::Success(isDown);
    }
    catch (const std::exception& e)
    {
        return ScriptMethodResult::Error("檢查按鍵狀態失敗: " + std::string(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult InputScriptInterface::ExecuteWasKeyJustPressed(const std::vector<std::any>& args)
{
    auto result = ValidateArgCount(args, 1, "wasKeyJustPressed");
    if (!result.success) return result;

    try
    {
        int keyCode = ExtractInt(args[0]);
        bool wasPressed = m_inputSystem->WasKeyJustPressed(keyCode);
        return ScriptMethodResult::Success(wasPressed);
    }
    catch (const std::exception& e)
    {
        return ScriptMethodResult::Error("檢查按鍵剛按下狀態失敗: " + std::string(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult InputScriptInterface::ExecuteWasKeyJustReleased(const std::vector<std::any>& args)
{
    auto result = ValidateArgCount(args, 1, "wasKeyJustReleased");
    if (!result.success) return result;

    try
    {
        int keyCode = ExtractInt(args[0]);
        bool wasReleased = m_inputSystem->WasKeyJustReleased(keyCode);
        return ScriptMethodResult::Success(wasReleased);
    }
    catch (const std::exception& e)
    {
        return ScriptMethodResult::Error("檢查按鍵剛釋放狀態失敗: " + std::string(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult InputScriptInterface::ExecuteGetCursorClientDelta(const std::vector<std::any>& args)
{
    auto result = ValidateArgCount(args, 0, "getCursorDelta");
    if (!result.success) return result;

    try
    {
        Vec2 delta = m_inputSystem->GetCursorClientDelta();
        std::string deltaStr = "{ x: " + std::to_string(delta.x) + ", y: " + std::to_string(delta.y) + " }";
        return ScriptMethodResult::Success(deltaStr);
    }
    catch (const std::exception& e)
    {
        return ScriptMethodResult::Error("取得游標移動增量失敗: " + std::string(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult InputScriptInterface::ExecuteGetCursorClientPosition(const std::vector<std::any>& args)
{
    auto result = ValidateArgCount(args, 0, "getCursorPosition");
    if (!result.success) return result;

    try
    {
        Vec2 position = m_inputSystem->GetCursorClientPosition();
        std::string positionStr = "{ x: " + std::to_string(position.x) + ", y: " + std::to_string(position.y) + " }";
        return ScriptMethodResult::Success(positionStr);
    }
    catch (const std::exception& e)
    {
        return ScriptMethodResult::Error("取得游標位置失敗: " + std::string(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult InputScriptInterface::ExecuteGetController(const std::vector<std::any>& args)
{
    auto result = ValidateArgCount(args, 1, "getController");
    if (!result.success) return result;

    try
    {
        int controllerIndex = ExtractInt(args[0]);
        XboxController const& controller = m_inputSystem->GetController(controllerIndex);
        
        // 簡化的控制器狀態回傳
        std::string controllerStr = "{ index: " + std::to_string(controllerIndex) + ", connected: true }";
        return ScriptMethodResult::Success(controllerStr);
    }
    catch (const std::exception& e)
    {
        return ScriptMethodResult::Error("取得控制器失敗: " + std::string(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult InputScriptInterface::ExecuteSetCursorMode(const std::vector<std::any>& args)
{
    auto result = ValidateArgCount(args, 1, "setCursorMode");
    if (!result.success) return result;

    try
    {
        int mode = ExtractInt(args[0]);
        eCursorMode cursorMode = static_cast<eCursorMode>(mode);
        m_inputSystem->SetCursorMode(cursorMode);
        return ScriptMethodResult::Success(std::string("游標模式已設定為: " + std::to_string(mode)));
    }
    catch (const std::exception& e)
    {
        return ScriptMethodResult::Error("設定游標模式失敗: " + std::string(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
// 輔助方法實作
//----------------------------------------------------------------------------------------------------

template <typename T>
T InputScriptInterface::ExtractArg(const std::any& arg, const std::string& expectedType) const
{
    try
    {
        return std::any_cast<T>(arg);
    }
    catch (const std::bad_any_cast& e)
    {
        std::string typeInfo = expectedType.empty() ? typeid(T).name() : expectedType;
        throw std::invalid_argument("參數類型錯誤，期望: " + typeInfo);
    }
}

//----------------------------------------------------------------------------------------------------
int InputScriptInterface::ExtractInt(const std::any& arg) const
{
    try
    {
        return std::any_cast<int>(arg);
    }
    catch (const std::bad_any_cast&)
    {
        try
        {
            return static_cast<int>(std::any_cast<float>(arg));
        }
        catch (const std::bad_any_cast&)
        {
            try
            {
                return static_cast<int>(std::any_cast<double>(arg));
            }
            catch (const std::bad_any_cast&)
            {
                throw std::invalid_argument("無法轉換為 int 類型");
            }
        }
    }
}

//----------------------------------------------------------------------------------------------------
std::string InputScriptInterface::ExtractString(const std::any& arg) const
{
    try
    {
        return std::any_cast<std::string>(arg);
    }
    catch (const std::bad_any_cast&)
    {
        try
        {
            const char* cstr = std::any_cast<const char*>(arg);
            return std::string(cstr);
        }
        catch (const std::bad_any_cast&)
        {
            throw std::invalid_argument("無法轉換為 string 類型");
        }
    }
}

//----------------------------------------------------------------------------------------------------
bool InputScriptInterface::ExtractBool(const std::any& arg) const
{
    try
    {
        return std::any_cast<bool>(arg);
    }
    catch (const std::bad_any_cast&)
    {
        try
        {
            int val = std::any_cast<int>(arg);
            return val != 0;
        }
        catch (const std::bad_any_cast&)
        {
            throw std::invalid_argument("無法轉換為 bool 類型");
        }
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult InputScriptInterface::ValidateArgCount(const std::vector<std::any>& args,
                                                           size_t                       expectedCount,
                                                           const std::string&           methodName) const
{
    if (args.size() != expectedCount)
    {
        std::ostringstream oss;
        oss << methodName << " needs " << expectedCount << " variables, but receives " << args.size();
        return ScriptMethodResult::Error(oss.str());
    }
    return ScriptMethodResult::Success();
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult InputScriptInterface::ValidateArgCountRange(const std::vector<std::any>& args,
                                                                size_t                       minCount,
                                                                size_t                       maxCount,
                                                                const std::string&           methodName) const
{
    if (args.size() < minCount || args.size() > maxCount)
    {
        std::ostringstream oss;
        oss << methodName << " needs " << minCount << "-" << maxCount << " variables, but receives " << args.size();
        return ScriptMethodResult::Error(oss.str());
    }
    return ScriptMethodResult::Success();
}