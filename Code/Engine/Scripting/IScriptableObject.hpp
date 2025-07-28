//----------------------------------------------------------------------------------------------------
// IScriptableObject.hpp
// 通用的腳本化物件介面 - 讓 V8Subsystem 可以與任何物件互動而不需要知道具體類型
//----------------------------------------------------------------------------------------------------

#pragma once
#include <string>
#include <vector>
#include <any>

//----------------------------------------------------------------------------------------------------
// 腳本方法呼叫的結果類型
//----------------------------------------------------------------------------------------------------
struct ScriptMethodResult
{
    bool success = false;
    std::any result;
    std::string errorMessage;

    // 便利建構函式
    static ScriptMethodResult Success(const std::any& value = std::any{})
    {
        ScriptMethodResult result;
        result.success = true;
        result.result = value;
        return result;
    }

    static ScriptMethodResult Error(const std::string& message)
    {
        ScriptMethodResult result;
        result.success = false;
        result.errorMessage = message;
        return result;
    }
};

//----------------------------------------------------------------------------------------------------
// 腳本方法的元資料
//----------------------------------------------------------------------------------------------------
struct ScriptMethodInfo
{
    std::string name;
    std::string description;
    std::vector<std::string> parameterTypes;
    std::string returnType;

    ScriptMethodInfo(const std::string& methodName,
                    const std::string& desc = "",
                    const std::vector<std::string>& params = {},
                    const std::string& retType = "void")
        : name(methodName), description(desc), parameterTypes(params), returnType(retType)
    {
    }
};

//----------------------------------------------------------------------------------------------------
// 可腳本化物件的抽象介面
// 任何想要被 JavaScript 呼叫的 C++ 物件都應該實作這個介面
//----------------------------------------------------------------------------------------------------
class IScriptableObject
{
public:
    virtual ~IScriptableObject() = default;

    // 取得這個物件在 JavaScript 中的名稱
    virtual std::string GetScriptObjectName() const = 0;

    // 取得這個物件可用的方法資訊
    virtual std::vector<ScriptMethodInfo> GetAvailableMethods() const = 0;

    // 呼叫物件的方法
    // methodName: 要呼叫的方法名稱
    // args: 方法參數，使用 std::any 來支援任意類型
    // 回傳: ScriptMethodResult 包含執行結果或錯誤資訊
    virtual ScriptMethodResult CallMethod(const std::string& methodName,
                                        const std::vector<std::any>& args) = 0;

    // 取得物件的屬性值（可選實作）
    virtual std::any GetProperty(const std::string& propertyName) const
    {
        return std::any{};
    }

    // 設定物件的屬性值（可選實作）
    virtual bool SetProperty(const std::string& propertyName, const std::any& value)
    {
        return false;
    }

    // 取得可用屬性清單（可選實作）
    virtual std::vector<std::string> GetAvailableProperties() const
    {
        return {};
    }

    // 檢查是否有指定的方法
    bool HasMethod(const std::string& methodName) const
    {
        auto methods = GetAvailableMethods();
        for (const auto& method : methods)
        {
            if (method.name == methodName)
            {
                return true;
            }
        }
        return false;
    }

    // 檢查是否有指定的屬性
    virtual bool HasProperty(const std::string& propertyName) const
    {
        auto properties = GetAvailableProperties();
        for (const auto& prop : properties)
        {
            if (prop == propertyName)
            {
                return true;
            }
        }
        return false;
    }
};