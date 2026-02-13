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
class CameraStateBuffer;
class DebugRenderAPI;

//----------------------------------------------------------------------------------------------------
/// @brief JavaScript interface for DebugRenderSystem integration providing debug visualization control
///
/// @remark Phase 4: Updated to use DebugRenderAPI for async command submission to RenderCommandQueue
///         instead of calling global DebugRenderSystem functions directly (synchronous).
///
/// @remark Implements method registry pattern for efficient JavaScript method dispatch and
///         provides type-safe parameter validation for all debug rendering operations.
///
/// @remark Architecture: JavaScript → ScriptInterface → DebugRenderAPI → RenderCommandQueue
///                       → App::ProcessRenderCommands() → DebugRenderStateBuffer
///                       → App::RenderDebugPrimitives() → DebugRenderSystem (global functions)
///
/// @see DebugRenderAPI.hpp for async command submission layer
/// @see DebugRenderSystem.hpp for underlying debug rendering implementation
/// @see IScriptableObject for JavaScript integration framework
//----------------------------------------------------------------------------------------------------
class DebugRenderSystemScriptInterface : public IScriptableObject
{
public:
    /// @brief Construct DebugRenderSystemScriptInterface with DebugRenderAPI for async operations
    ///
    /// @param debugRenderAPI Pointer to DebugRenderAPI for async command submission (NO OWNERSHIP)
    /// @param cameraAPI      Pointer to CameraStateBuffer for resolving cameraId → Camera* (NO OWNERSHIP, optional)
    ///
    /// @remark Phase 4: Constructor now requires DebugRenderAPI pointer (replaces parameterless constructor)
    /// @remark Automatically initializes method registry for efficient JavaScript dispatch.
    explicit DebugRenderSystemScriptInterface(DebugRenderAPI* debugRenderAPI, CameraStateBuffer* cameraStateBuffer = nullptr);

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
    ScriptMethodResult ExecuteClearAll(ScriptArgs const& args);

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

    // === MEMBER VARIABLES (Phase 4) ===
    DebugRenderAPI*     m_debugRenderAPI     = nullptr;  // Async API for command submission (NO OWNERSHIP)
    CameraStateBuffer*  m_cameraStateBuffer  = nullptr;  // For resolving cameraId → Camera* (NO OWNERSHIP)
};
