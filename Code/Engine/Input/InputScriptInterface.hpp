//----------------------------------------------------------------------------------------------------
// InputScriptInterface.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include "Engine/Scripting/IScriptableObject.hpp"

//-Forward-Declaration--------------------------------------------------------------------------------
class InputSystem;

//----------------------------------------------------------------------------------------------------
// InputSystem 的腳本介面包裝器
// 這個類別作為 InputSystem 物件與 V8Subsystem 之間的橋樑
//----------------------------------------------------------------------------------------------------
class InputScriptInterface : public IScriptableObject
{
public:
    explicit InputScriptInterface(InputSystem* inputSystem);

    // 實作 IScriptableObject 介面
    String                        GetScriptObjectName() const override;
    std::vector<ScriptMethodInfo> GetAvailableMethods() const override;
    ScriptMethodResult            CallMethod(String const& methodName, std::vector<std::any> const& args) override;

    // 實作屬性存取
    std::any            GetProperty(String const& propertyName) const override;
    bool                SetProperty(String const& propertyName, const std::any& value) override;
    std::vector<String> GetAvailableProperties() const override;

private:
    InputSystem* m_inputSystem; // 不擁有，只是參考

    // 輔助方法來處理類型轉換和錯誤檢查
    template <typename T>
    T ExtractArg(std::any const& arg, String const& expectedType = "") const;

    // 專門的類型提取方法
    int    ExtractInt(const std::any& arg) const;
    String ExtractString(const std::any& arg) const;
    bool   ExtractBool(const std::any& arg) const;

    // 參數驗證輔助方法
    ScriptMethodResult ValidateArgCount(const std::vector<std::any>& args,
                                        size_t                       expectedCount,
                                        const String&                methodName) const;

    ScriptMethodResult ValidateArgCountRange(const std::vector<std::any>& args,
                                             size_t                       minCount,
                                             size_t                       maxCount,
                                             const String&                methodName) const;

    // 方法實作
    ScriptMethodResult ExecuteIsKeyDown(const std::vector<std::any>& args);
    ScriptMethodResult ExecuteWasKeyJustPressed(const std::vector<std::any>& args);
    ScriptMethodResult ExecuteWasKeyJustReleased(const std::vector<std::any>& args);
    ScriptMethodResult ExecuteGetCursorClientDelta(const std::vector<std::any>& args);
    ScriptMethodResult ExecuteGetCursorClientPosition(const std::vector<std::any>& args);
    ScriptMethodResult ExecuteGetController(const std::vector<std::any>& args);
    ScriptMethodResult ExecuteSetCursorMode(const std::vector<std::any>& args);
};
