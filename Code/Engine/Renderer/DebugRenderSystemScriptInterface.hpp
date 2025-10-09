//----------------------------------------------------------------------------------------------------
// DebugRenderSystemScriptInterface.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
//----------------------------------------------------------------------------------------------------
#include "Engine/Script/IScriptableObject.hpp"

//----------------------------------------------------------------------------------------------------

//-Forward-Declaration--------------------------------------------------------------------------------
class Camera;

//----------------------------------------------------------------------------------------------------
/// @brief JavaScript interface for DebugRenderSystem integration providing debug visualization control
///
/// @remark Exposes DebugRenderSystem global C-style API to JavaScript for debug rendering operations
///         including control, output, and geometry drawing functions for development and debugging.
///
/// @remark Implements method registry pattern for efficient JavaScript method dispatch and
///         provides type-safe parameter validation for all debug rendering operations.
///
/// @remark Note: DebugRenderSystem uses global C-style functions, so this interface wraps those
///         global functions rather than managing an instance pointer.
///
/// @see DebugRenderSystem.hpp for underlying debug rendering implementation
/// @see IScriptableObject for JavaScript integration framework
//----------------------------------------------------------------------------------------------------
class DebugRenderSystemScriptInterface : public IScriptableObject
{
public:
    /// @brief Construct DebugRenderSystemScriptInterface for debug rendering operations
    ///
    /// @remark DebugRenderSystem uses global functions, no instance pointer needed.
    /// @remark Automatically initializes method registry for efficient JavaScript dispatch.
    DebugRenderSystemScriptInterface();

    std::vector<ScriptMethodInfo> GetAvailableMethods() const override;
    StringList                    GetAvailableProperties() const override;

    ScriptMethodResult CallMethod(String const& methodName, ScriptArgs const& args) override;
    std::any           GetProperty(String const& propertyName) const override;
    bool               SetProperty(String const& propertyName, std::any const& value) override;

private:
    // === METHOD REGISTRY FOR EFFICIENT DISPATCH ===
    void InitializeMethodRegistry() override;

    // === CONTROL METHODS ===
    ScriptMethodResult ExecuteSetVisible(ScriptArgs const& args);
    ScriptMethodResult ExecuteSetHidden(ScriptArgs const& args);
    ScriptMethodResult ExecuteClear(ScriptArgs const& args);

    // === OUTPUT METHODS ===
    ScriptMethodResult ExecuteBeginFrame(ScriptArgs const& args);
    ScriptMethodResult ExecuteRenderWorld(ScriptArgs const& args);
    ScriptMethodResult ExecuteRenderScreen(ScriptArgs const& args);
    ScriptMethodResult ExecuteEndFrame(ScriptArgs const& args);

    // === GEOMETRY METHODS - WORLD SPACE ===
    ScriptMethodResult ExecuteAddWorldPoint(ScriptArgs const& args);
    ScriptMethodResult ExecuteAddWorldLine(ScriptArgs const& args);
    ScriptMethodResult ExecuteAddWorldCylinder(ScriptArgs const& args);
    ScriptMethodResult ExecuteAddWorldWireSphere(ScriptArgs const& args);
    ScriptMethodResult ExecuteAddWorldArrow(ScriptArgs const& args);
    ScriptMethodResult ExecuteAddWorldText(ScriptArgs const& args);
    ScriptMethodResult ExecuteAddBillboardText(ScriptArgs const& args);
    ScriptMethodResult ExecuteAddWorldBasis(ScriptArgs const& args);

    // === GEOMETRY METHODS - SCREEN SPACE ===
    ScriptMethodResult ExecuteAddScreenText(ScriptArgs const& args);
    ScriptMethodResult ExecuteAddMessage(ScriptArgs const& args);

    // === VALIDATION ===
    bool ValidateColor(int r, int g, int b, int a) const;
    bool ValidatePosition(float x, float y, float z) const;
    bool ValidateDuration(float duration) const;
    int  StringToDebugRenderMode(String const& modeStr) const;
};
