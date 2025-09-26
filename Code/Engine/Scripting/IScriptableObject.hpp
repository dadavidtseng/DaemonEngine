//----------------------------------------------------------------------------------------------------
// IScriptableObject.hpp

//----------------------------------------------------------------------------------------------------
#pragma once
#include <any>
#include <string>
#include <vector>

#include "Engine/Core/StringUtils.hpp"
#include "Engine/Scripting/ScriptCommon.hpp"


//----------------------------------------------------------------------------------------------------
// 可腳本化物件的抽象介面
// 任何想要被 JavaScript 呼叫的 C++ 物件都應該實作這個介面
//----------------------------------------------------------------------------------------------------
class IScriptableObject
{
public:
    virtual ~IScriptableObject() = default;

    // 取得這個物件在 JavaScript 中的名稱
    virtual String GetScriptObjectName() const = 0;

    // 取得這個物件可用的方法資訊
    virtual std::vector<ScriptMethodInfo> GetAvailableMethods() const = 0;

    // 呼叫物件的方法
    // methodName: 要呼叫的方法名稱
    // args: 方法參數，使用 std::any 來支援任意類型
    // 回傳: ScriptMethodResult 包含執行結果或錯誤資訊
    virtual ScriptMethodResult CallMethod(String const& methodName, std::vector<std::any> const& args) = 0;

    // 取得物件的屬性值（可選實作）
    virtual std::any GetProperty(String const& propertyName) const;

    // 設定物件的屬性值（可選實作）
    virtual bool SetProperty(const String& propertyName, const std::any& value);

    // 取得可用屬性清單（可選實作）
    virtual std::vector<String> GetAvailableProperties() const;

    // 檢查是否有指定的方法
    bool HasMethod(String const& methodName) const;

    // 檢查是否有指定的屬性
    virtual bool HasProperty(String const& propertyName) const;
};
