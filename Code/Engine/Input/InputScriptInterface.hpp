//----------------------------------------------------------------------------------------------------
// InputScriptInterface.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
//----------------------------------------------------------------------------------------------------
#include "Engine/Script/IScriptableObject.hpp"
//----------------------------------------------------------------------------------------------------
#include <functional>

//-Forward-Declaration--------------------------------------------------------------------------------
class InputSystem;

//----------------------------------------------------------------------------------------------------
class InputScriptInterface : public IScriptableObject
{
public:
    explicit InputScriptInterface(InputSystem* inputSystem);

    std::vector<ScriptMethodInfo> GetAvailableMethods() const override;
    StringList                    GetAvailableProperties() const override;

    ScriptMethodResult CallMethod(String const& methodName, ScriptArgs const& args) override;
    std::any           GetProperty(String const& propertyName) const override;
    bool               SetProperty(String const& propertyName, std::any const& value) override;

private:
    InputSystem* m_inputSystem;

    // === METHOD REGISTRY FOR EFFICIENT DISPATCH ===
    using MethodFunction = std::function<ScriptMethodResult(ScriptArgs const&)>;
    std::unordered_map<String, MethodFunction> m_methodRegistry;
    void                                       InitializeMethodRegistry();

    // === KEYBOARD INPUT METHODS ===
    ScriptMethodResult ExecuteIsKeyPressed(ScriptArgs const& args);
    ScriptMethodResult ExecuteWasKeyJustPressed(ScriptArgs const& args);
    ScriptMethodResult ExecuteWasKeyJustReleased(ScriptArgs const& args);

    // === MOUSE INPUT METHODS ===
    ScriptMethodResult ExecuteGetMousePosition(ScriptArgs const& args);
    ScriptMethodResult ExecuteIsMouseButtonPressed(ScriptArgs const& args);
    ScriptMethodResult ExecuteWasMouseButtonJustPressed(ScriptArgs const& args);
    ScriptMethodResult ExecuteGetMouseDelta(ScriptArgs const& args);

    // === CONTROLLER INPUT METHODS ===
    ScriptMethodResult ExecuteIsControllerConnected(ScriptArgs const& args);
    ScriptMethodResult ExecuteGetControllerAxis(ScriptArgs const& args);
    ScriptMethodResult ExecuteIsControllerButtonPressed(ScriptArgs const& args);

    // === LEGACY METHODS (for backward compatibility) ===
    ScriptMethodResult ExecuteIsKeyDown(ScriptArgs const& args);
    ScriptMethodResult ExecuteGetCursorClientDelta(ScriptArgs const& args);
    ScriptMethodResult ExecuteGetCursorClientPosition(ScriptArgs const& args);
    ScriptMethodResult ExecuteGetController(ScriptArgs const& args);
    ScriptMethodResult ExecuteSetCursorMode(ScriptArgs const& args);

    // === VALIDATION AND SECURITY ===
    bool ValidateKeyCode(int keyCode) const;
    bool ValidateMouseButton(int button) const;
    bool ValidateControllerIndex(int index) const;
    bool ValidateControllerAxis(int axis) const;
    bool ValidateControllerButton(int button) const;
};
