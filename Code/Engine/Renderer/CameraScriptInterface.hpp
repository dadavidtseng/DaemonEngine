//----------------------------------------------------------------------------------------------------
// CameraScriptInterface.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
//----------------------------------------------------------------------------------------------------
#include "Engine/Script/IScriptableObject.hpp"

#include <unordered_map>
#include <string>

//-Forward-Declaration--------------------------------------------------------------------------------
class Camera;

//----------------------------------------------------------------------------------------------------
/// @brief Script interface for camera creation and manipulation
///
/// Provides JavaScript access to camera operations including:
/// - Camera instance creation and destruction
/// - Perspective and orthographic configuration
/// - Position and orientation control
/// - Viewport and transform management
//----------------------------------------------------------------------------------------------------
class CameraScriptInterface : public IScriptableObject
{
public:
    CameraScriptInterface();
    ~CameraScriptInterface() override;

    std::vector<ScriptMethodInfo> GetAvailableMethods() const override;
    StringList                    GetAvailableProperties() const override;

    ScriptMethodResult CallMethod(String const& methodName, ScriptArgs const& args) override;
    std::any           GetProperty(String const& propertyName) const override;
    bool               SetProperty(String const& propertyName, std::any const& value) override;

    // Public accessor for C++ code to get active world camera
    Camera* GetActiveWorldCameraPtr() const { return m_activeWorldCamera; }

    // Public accessor for C++ code to get camera by role (Phase 2)
    Camera* GetCameraByRole(std::string const& role) const
    {
        auto it = m_cameraRoles.find(role);
        return (it != m_cameraRoles.end()) ? it->second : nullptr;
    }

private:
    void InitializeMethodRegistry() override;

    // Camera creation and destruction
    ScriptMethodResult ExecuteCreateCamera(ScriptArgs const& args);
    ScriptMethodResult ExecuteDestroyCamera(ScriptArgs const& args);

    // Camera configuration
    ScriptMethodResult ExecuteSetPerspectiveView(ScriptArgs const& args);
    ScriptMethodResult ExecuteSetOrthographicView(ScriptArgs const& args);
    ScriptMethodResult ExecuteSetNormalizedViewport(ScriptArgs const& args);
    ScriptMethodResult ExecuteSetCameraToRenderTransform(ScriptArgs const& args);

    // Camera manipulation
    ScriptMethodResult ExecuteSetCameraPosition(ScriptArgs const& args);
    ScriptMethodResult ExecuteSetCameraOrientation(ScriptArgs const& args);
    ScriptMethodResult ExecuteSetCameraPositionAndOrientation(ScriptArgs const& args);

    // Camera queries
    ScriptMethodResult ExecuteGetCameraPosition(ScriptArgs const& args);
    ScriptMethodResult ExecuteGetCameraOrientation(ScriptArgs const& args);

    // Active camera management
    ScriptMethodResult ExecuteSetActiveWorldCamera(ScriptArgs const& args);
    ScriptMethodResult ExecuteGetActiveWorldCamera(ScriptArgs const& args);

    // Camera role management (Phase 2: Entity-based camera selection)
    ScriptMethodResult ExecuteSetCameraRole(ScriptArgs const& args);
    ScriptMethodResult ExecuteGetCameraByRole(ScriptArgs const& args);

    // Camera storage for JavaScript-created cameras
    std::vector<Camera*> m_createdCameras;

    // Active world camera (used for 3D rendering)
    Camera* m_activeWorldCamera = nullptr;

    // Camera roles (Phase 2: Support "world" and "screen" cameras)
    std::unordered_map<std::string, Camera*> m_cameraRoles;  // role name -> camera pointer
};
