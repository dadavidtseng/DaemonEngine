//----------------------------------------------------------------------------------------------------
// CameraScriptInterface.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Renderer/CameraScriptInterface.hpp"
//----------------------------------------------------------------------------------------------------
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Math/Mat44.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Script/ScriptTypeExtractor.hpp"

//----------------------------------------------------------------------------------------------------
CameraScriptInterface::CameraScriptInterface()
{
    CameraScriptInterface::InitializeMethodRegistry();
}

//----------------------------------------------------------------------------------------------------
CameraScriptInterface::~CameraScriptInterface()
{
    // Clean up all JavaScript-created cameras
    for (Camera* camera : m_createdCameras)
    {
        delete camera;
    }
    m_createdCameras.clear();
}

//----------------------------------------------------------------------------------------------------
std::vector<ScriptMethodInfo> CameraScriptInterface::GetAvailableMethods() const
{
    return {
        ScriptMethodInfo("createCamera",
                         "Create a new camera instance",
                         {},
                         "Camera*"),

        ScriptMethodInfo("destroyCamera",
                         "Destroy a camera instance",
                         {"Camera*"},
                         "void"),

        ScriptMethodInfo("setPerspectiveView",
                         "Configure camera for perspective projection",
                         {"Camera*", "float", "float", "float", "float"},
                         "void"),

        ScriptMethodInfo("setOrthographicView",
                         "Configure camera for orthographic projection",
                         {"Camera*", "float", "float", "float", "float"},
                         "void"),

        ScriptMethodInfo("setNormalizedViewport",
                         "Set camera viewport (normalized 0-1)",
                         {"Camera*", "float", "float", "float", "float"},
                         "void"),

        ScriptMethodInfo("setCameraToRenderTransform",
                         "Set camera-to-render coordinate transform matrix",
                         {"Camera*", "float[16]"},
                         "void"),

        ScriptMethodInfo("setCameraPosition",
                         "Set camera world position",
                         {"Camera*", "float", "float", "float"},
                         "void"),

        ScriptMethodInfo("setCameraOrientation",
                         "Set camera orientation (yaw, pitch, roll in degrees)",
                         {"Camera*", "float", "float", "float"},
                         "void"),

        ScriptMethodInfo("setCameraPositionAndOrientation",
                         "Set camera position and orientation",
                         {"Camera*", "float", "float", "float", "float", "float", "float"},
                         "void"),

        ScriptMethodInfo("getCameraPosition",
                         "Get camera world position",
                         {"Camera*"},
                         "object"),

        ScriptMethodInfo("getCameraOrientation",
                         "Get camera orientation (yaw, pitch, roll)",
                         {"Camera*"},
                         "object")
    };
}

//----------------------------------------------------------------------------------------------------
StringList CameraScriptInterface::GetAvailableProperties() const
{
    return {};
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult CameraScriptInterface::CallMethod(String const&     methodName,
                                                     ScriptArgs const& args)
{
    auto it = m_methodRegistry.find(methodName);
    if (it != m_methodRegistry.end())
    {
        return it->second(args);
    }

    return ScriptMethodResult::Error("Unknown method: " + methodName);
}

//----------------------------------------------------------------------------------------------------
std::any CameraScriptInterface::GetProperty(String const& propertyName) const
{
    UNUSED(propertyName)
    return std::any{};
}

//----------------------------------------------------------------------------------------------------
bool CameraScriptInterface::SetProperty(String const& propertyName, std::any const& value)
{
    UNUSED(propertyName)
    UNUSED(value)
    return false;
}

//----------------------------------------------------------------------------------------------------
void CameraScriptInterface::InitializeMethodRegistry()
{
    m_methodRegistry["createCamera"]                    = [this](ScriptArgs const& args) { return ExecuteCreateCamera(args); };
    m_methodRegistry["destroyCamera"]                   = [this](ScriptArgs const& args) { return ExecuteDestroyCamera(args); };
    m_methodRegistry["setPerspectiveView"]              = [this](ScriptArgs const& args) { return ExecuteSetPerspectiveView(args); };
    m_methodRegistry["setOrthographicView"]             = [this](ScriptArgs const& args) { return ExecuteSetOrthographicView(args); };
    m_methodRegistry["setNormalizedViewport"]           = [this](ScriptArgs const& args) { return ExecuteSetNormalizedViewport(args); };
    m_methodRegistry["setCameraToRenderTransform"]      = [this](ScriptArgs const& args) { return ExecuteSetCameraToRenderTransform(args); };
    m_methodRegistry["setCameraPosition"]               = [this](ScriptArgs const& args) { return ExecuteSetCameraPosition(args); };
    m_methodRegistry["setCameraOrientation"]            = [this](ScriptArgs const& args) { return ExecuteSetCameraOrientation(args); };
    m_methodRegistry["setCameraPositionAndOrientation"] = [this](ScriptArgs const& args) { return ExecuteSetCameraPositionAndOrientation(args); };
    m_methodRegistry["getCameraPosition"]               = [this](ScriptArgs const& args) { return ExecuteGetCameraPosition(args); };
    m_methodRegistry["getCameraOrientation"]            = [this](ScriptArgs const& args) { return ExecuteGetCameraOrientation(args); };
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult CameraScriptInterface::ExecuteCreateCamera(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 0, "createCamera");
    if (!result.success) return result;

    try
    {
        Camera* newCamera = new Camera();
        m_createdCameras.push_back(newCamera);

        // Return camera pointer as double for JavaScript
        // (JavaScript numbers are doubles, can safely hold pointer values)
        uint64_t cameraHandle       = reinterpret_cast<uint64_t>(newCamera);
        double   cameraHandleDouble = static_cast<double>(cameraHandle);
        return ScriptMethodResult::Success(cameraHandleDouble);
    }
    catch (std::exception const& e)
    {
        return ScriptMethodResult::Error("Failed to create camera: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult CameraScriptInterface::ExecuteDestroyCamera(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 1, "destroyCamera");
    if (!result.success) return result;

    try
    {
        double   cameraHandleDouble = ScriptTypeExtractor::ExtractDouble(args[0]);
        uint64_t cameraHandle       = static_cast<uint64_t>(cameraHandleDouble);
        Camera*  camera             = reinterpret_cast<Camera*>(cameraHandle);

        // Find and remove from created cameras list
        auto it = std::find(m_createdCameras.begin(), m_createdCameras.end(), camera);
        if (it != m_createdCameras.end())
        {
            delete camera;
            m_createdCameras.erase(it);
            return ScriptMethodResult::Success();
        }

        return ScriptMethodResult::Error("Camera not found in managed cameras");
    }
    catch (std::exception const& e)
    {
        return ScriptMethodResult::Error("Failed to destroy camera: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult CameraScriptInterface::ExecuteSetPerspectiveView(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 5, "setPerspectiveView");
    if (!result.success) return result;

    try
    {
        double   cameraHandleDouble = ScriptTypeExtractor::ExtractDouble(args[0]);
        uint64_t cameraHandle       = static_cast<uint64_t>(cameraHandleDouble);
        Camera*  camera             = reinterpret_cast<Camera*>(cameraHandle);

        float aspect     = ScriptTypeExtractor::ExtractFloat(args[1]);
        float fovDegrees = ScriptTypeExtractor::ExtractFloat(args[2]);
        float nearZ      = ScriptTypeExtractor::ExtractFloat(args[3]);
        float farZ       = ScriptTypeExtractor::ExtractFloat(args[4]);

        camera->SetPerspectiveGraphicView(aspect, fovDegrees, nearZ, farZ);
        return ScriptMethodResult::Success();
    }
    catch (std::exception const& e)
    {
        return ScriptMethodResult::Error("Failed to set perspective view: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult CameraScriptInterface::ExecuteSetOrthographicView(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 5, "setOrthographicView");
    if (!result.success) return result;

    try
    {
        double   cameraHandleDouble = ScriptTypeExtractor::ExtractDouble(args[0]);
        uint64_t cameraHandle       = static_cast<uint64_t>(cameraHandleDouble);
        Camera*  camera             = reinterpret_cast<Camera*>(cameraHandle);

        float minX = ScriptTypeExtractor::ExtractFloat(args[1]);
        float minY = ScriptTypeExtractor::ExtractFloat(args[2]);
        float maxX = ScriptTypeExtractor::ExtractFloat(args[3]);
        float maxY = ScriptTypeExtractor::ExtractFloat(args[4]);

        camera->SetOrthoGraphicView(Vec2(minX, minY), Vec2(maxX, maxY));
        return ScriptMethodResult::Success();
    }
    catch (std::exception const& e)
    {
        return ScriptMethodResult::Error("Failed to set orthographic view: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult CameraScriptInterface::ExecuteSetNormalizedViewport(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 5, "setNormalizedViewport");
    if (!result.success) return result;

    try
    {
        double   cameraHandleDouble = ScriptTypeExtractor::ExtractDouble(args[0]);
        uint64_t cameraHandle       = static_cast<uint64_t>(cameraHandleDouble);
        Camera*  camera             = reinterpret_cast<Camera*>(cameraHandle);

        float minX = ScriptTypeExtractor::ExtractFloat(args[1]);
        float minY = ScriptTypeExtractor::ExtractFloat(args[2]);
        float maxX = ScriptTypeExtractor::ExtractFloat(args[3]);
        float maxY = ScriptTypeExtractor::ExtractFloat(args[4]);

        camera->SetNormalizedViewport(AABB2(minX, minY, maxX, maxY));
        return ScriptMethodResult::Success();
    }
    catch (std::exception const& e)
    {
        return ScriptMethodResult::Error("Failed to set normalized viewport: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult CameraScriptInterface::ExecuteSetCameraToRenderTransform(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 17, "setCameraToRenderTransform");
    if (!result.success) return result;

    try
    {
        double   cameraHandleDouble = ScriptTypeExtractor::ExtractDouble(args[0]);
        uint64_t cameraHandle       = static_cast<uint64_t>(cameraHandleDouble);
        Camera*  camera             = reinterpret_cast<Camera*>(cameraHandle);

        Mat44 transform;
        for (int i = 0; i < 16; ++i)
        {
            transform.m_values[i] = ScriptTypeExtractor::ExtractFloat(args[i + 1]);
        }

        camera->SetCameraToRenderTransform(transform);
        return ScriptMethodResult::Success();
    }
    catch (std::exception const& e)
    {
        return ScriptMethodResult::Error("Failed to set camera-to-render transform: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult CameraScriptInterface::ExecuteSetCameraPosition(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 4, "setCameraPosition");
    if (!result.success) return result;

    try
    {
        double   cameraHandleDouble = ScriptTypeExtractor::ExtractDouble(args[0]);
        uint64_t cameraHandle       = static_cast<uint64_t>(cameraHandleDouble);
        Camera*  camera             = reinterpret_cast<Camera*>(cameraHandle);

        Vec3 position = ScriptTypeExtractor::ExtractVec3(args, 1);
        camera->SetPosition(position);
        return ScriptMethodResult::Success();
    }
    catch (std::exception const& e)
    {
        return ScriptMethodResult::Error("Failed to set camera position: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult CameraScriptInterface::ExecuteSetCameraOrientation(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 4, "setCameraOrientation");
    if (!result.success) return result;

    try
    {
        double   cameraHandleDouble = ScriptTypeExtractor::ExtractDouble(args[0]);
        uint64_t cameraHandle       = static_cast<uint64_t>(cameraHandleDouble);
        Camera*  camera             = reinterpret_cast<Camera*>(cameraHandle);

        float yawDegrees   = ScriptTypeExtractor::ExtractFloat(args[1]);
        float pitchDegrees = ScriptTypeExtractor::ExtractFloat(args[2]);
        float rollDegrees  = ScriptTypeExtractor::ExtractFloat(args[3]);

        EulerAngles orientation(yawDegrees, pitchDegrees, rollDegrees);
        camera->SetOrientation(orientation);
        return ScriptMethodResult::Success();
    }
    catch (std::exception const& e)
    {
        return ScriptMethodResult::Error("Failed to set camera orientation: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult CameraScriptInterface::ExecuteSetCameraPositionAndOrientation(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 7, "setCameraPositionAndOrientation");
    if (!result.success) return result;

    try
    {
        double   cameraHandleDouble = ScriptTypeExtractor::ExtractDouble(args[0]);
        uint64_t cameraHandle       = static_cast<uint64_t>(cameraHandleDouble);
        Camera*  camera             = reinterpret_cast<Camera*>(cameraHandle);

        Vec3 position = ScriptTypeExtractor::ExtractVec3(args, 1);

        float yawDegrees   = ScriptTypeExtractor::ExtractFloat(args[4]);
        float pitchDegrees = ScriptTypeExtractor::ExtractFloat(args[5]);
        float rollDegrees  = ScriptTypeExtractor::ExtractFloat(args[6]);

        EulerAngles orientation(yawDegrees, pitchDegrees, rollDegrees);
        camera->SetPositionAndOrientation(position, orientation);
        return ScriptMethodResult::Success();
    }
    catch (std::exception const& e)
    {
        return ScriptMethodResult::Error("Failed to set camera position and orientation: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult CameraScriptInterface::ExecuteGetCameraPosition(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 1, "getCameraPosition");
    if (!result.success) return result;

    try
    {
        double   cameraHandleDouble = ScriptTypeExtractor::ExtractDouble(args[0]);
        uint64_t cameraHandle       = static_cast<uint64_t>(cameraHandleDouble);
        Camera*  camera             = reinterpret_cast<Camera*>(cameraHandle);

        Vec3 position = camera->GetPosition();

        String positionStr = "{ x: " + std::to_string(position.x) +
            ", y: " + std::to_string(position.y) +
            ", z: " + std::to_string(position.z) + " }";

        return ScriptMethodResult::Success(positionStr);
    }
    catch (std::exception const& e)
    {
        return ScriptMethodResult::Error("Failed to get camera position: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult CameraScriptInterface::ExecuteGetCameraOrientation(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 1, "getCameraOrientation");
    if (!result.success) return result;

    try
    {
        double   cameraHandleDouble = ScriptTypeExtractor::ExtractDouble(args[0]);
        uint64_t cameraHandle       = static_cast<uint64_t>(cameraHandleDouble);
        Camera*  camera             = reinterpret_cast<Camera*>(cameraHandle);

        EulerAngles orientation = camera->GetOrientation();

        String orientationStr = "{ yaw: " + std::to_string(orientation.m_yawDegrees) +
            ", pitch: " + std::to_string(orientation.m_pitchDegrees) +
            ", roll: " + std::to_string(orientation.m_rollDegrees) + " }";

        return ScriptMethodResult::Success(orientationStr);
    }
    catch (std::exception const& e)
    {
        return ScriptMethodResult::Error("Failed to get camera orientation: " + String(e.what()));
    }
}
