//----------------------------------------------------------------------------------------------------
// InputScriptInterface.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include "Engine/Scripting/IScriptableObject.hpp"
#include <functional>
#include <unordered_map>

//-Forward-Declaration--------------------------------------------------------------------------------
class InputSystem;

//----------------------------------------------------------------------------------------------------
// InputSystem 的腳本介面包裝器
// 這個類別作為 InputSystem 物件與 ScriptSubsystem 之間的橋樑
// 實現了計畫中的安全白名單方法
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

    // === METHOD REGISTRY FOR EFFICIENT DISPATCH ===
    using MethodFunction = std::function<ScriptMethodResult(const std::vector<std::any>&)>;
    std::unordered_map<String, MethodFunction> m_methodRegistry;
    void                                       InitializeMethodRegistry();

    // === KEYBOARD INPUT METHODS ===
    ScriptMethodResult ExecuteIsKeyPressed(const std::vector<std::any>& args);
    ScriptMethodResult ExecuteWasKeyJustPressed(const std::vector<std::any>& args);
    ScriptMethodResult ExecuteWasKeyJustReleased(const std::vector<std::any>& args);

    // === MOUSE INPUT METHODS ===
    ScriptMethodResult ExecuteGetMousePosition(const std::vector<std::any>& args);
    ScriptMethodResult ExecuteIsMouseButtonPressed(const std::vector<std::any>& args);
    ScriptMethodResult ExecuteWasMouseButtonJustPressed(const std::vector<std::any>& args);
    ScriptMethodResult ExecuteGetMouseDelta(const std::vector<std::any>& args);

    // === CONTROLLER INPUT METHODS ===
    ScriptMethodResult ExecuteIsControllerConnected(const std::vector<std::any>& args);
    ScriptMethodResult ExecuteGetControllerAxis(const std::vector<std::any>& args);
    ScriptMethodResult ExecuteIsControllerButtonPressed(const std::vector<std::any>& args);

    // === LEGACY METHODS (for backward compatibility) ===
    ScriptMethodResult ExecuteIsKeyDown(const std::vector<std::any>& args);
    ScriptMethodResult ExecuteGetCursorClientDelta(const std::vector<std::any>& args);
    ScriptMethodResult ExecuteGetCursorClientPosition(const std::vector<std::any>& args);
    ScriptMethodResult ExecuteGetController(const std::vector<std::any>& args);
    ScriptMethodResult ExecuteSetCursorMode(const std::vector<std::any>& args);

    // === VALIDATION AND SECURITY ===
    bool ValidateKeyCode(int keyCode) const;
    bool ValidateMouseButton(int button) const;
    bool ValidateControllerIndex(int index) const;
    bool ValidateControllerAxis(int axis) const;
    bool ValidateControllerButton(int button) const;
};
