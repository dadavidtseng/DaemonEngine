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
    bool        success = false;
    std::any    result;
    std::string errorMessage;

    static ScriptMethodResult Success(const std::any& value = std::any{})
    {
        ScriptMethodResult result;
        result.success = true;
        result.result  = value;
        return result;
    }

    static ScriptMethodResult Error(const std::string& message)
    {
        ScriptMethodResult result;
        result.success      = false;
        result.errorMessage = message;
        return result;
    }
};

//----------------------------------------------------------------------------------------------------
// 腳本方法的元資料
//----------------------------------------------------------------------------------------------------
struct ScriptMethodInfo
{
    std::string              name;
    std::string              description;
    std::vector<std::string> parameterTypes;
    std::string              returnType;

    explicit ScriptMethodInfo(std::string const&              methodName,
                              std::string const&              desc       = "",
                              std::vector<std::string> const& params     = {},
                              std::string const&              returnType = "void")
        : name(methodName), description(desc), parameterTypes(params), returnType(returnType)
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
    virtual ScriptMethodResult CallMethod(std::string const& methodName, std::vector<std::any> const& args) = 0;

    // 取得物件的屬性值（可選實作）
    virtual std::any GetProperty(std::string const& propertyName) const;

    // 設定物件的屬性值（可選實作）
    virtual bool SetProperty(const std::string& propertyName, const std::any& value);

    // 取得可用屬性清單（可選實作）
    virtual std::vector<std::string> GetAvailableProperties() const;

    // 檢查是否有指定的方法
    bool HasMethod(std::string const& methodName) const;

    // 檢查是否有指定的屬性
    virtual bool HasProperty(std::string const& propertyName) const;
};
