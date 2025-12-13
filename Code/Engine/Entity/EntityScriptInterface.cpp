//----------------------------------------------------------------------------------------------------
// EntityScriptInterface.cpp
// M4-T8: JavaScript Interface for Entity API Implementation (camera methods removed)
//----------------------------------------------------------------------------------------------------

#include "Engine/Entity/EntityScriptInterface.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/LogSubsystem.hpp"
#include "Engine/Entity/EntityAPI.hpp"
#include "Engine/Script/ScriptTypeExtractor.hpp"

//----------------------------------------------------------------------------------------------------
// Construction / Destruction
//----------------------------------------------------------------------------------------------------

EntityScriptInterface::EntityScriptInterface(EntityAPI* entityAPI)
    : m_entityAPI(entityAPI)
{
    GUARANTEE_OR_DIE(m_entityAPI != nullptr, "EntityScriptInterface: EntityAPI is nullptr!");

    // CRITICAL: Initialize method registry so CallMethod() can find methods
    EntityScriptInterface::InitializeMethodRegistry();

    DebuggerPrintf("EntityScriptInterface: Initialized with %zu methods (M4-T8)\n", m_methodRegistry.size());
}

//----------------------------------------------------------------------------------------------------
// IScriptableObject Interface
//----------------------------------------------------------------------------------------------------

void EntityScriptInterface::InitializeMethodRegistry()
{
    // Entity management methods only (camera methods moved to CameraScriptInterface)
    m_methodRegistry["createMesh"]        = [this](ScriptArgs const& args) { return ExecuteCreateMesh(args); };
    m_methodRegistry["updatePosition"]    = [this](ScriptArgs const& args) { return ExecuteUpdatePosition(args); };
    m_methodRegistry["moveBy"]            = [this](ScriptArgs const& args) { return ExecuteMoveBy(args); };
    m_methodRegistry["updateOrientation"] = [this](ScriptArgs const& args) { return ExecuteUpdateOrientation(args); };
    m_methodRegistry["updateColor"]       = [this](ScriptArgs const& args) { return ExecuteUpdateColor(args); };
    m_methodRegistry["destroy"]           = [this](ScriptArgs const& args) { return ExecuteDestroyEntity(args); };
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult EntityScriptInterface::CallMethod(String const&     methodName,
                                                     ScriptArgs const& args)
{
    // Look up method in registry
    auto it = m_methodRegistry.find(methodName);
    if (it != m_methodRegistry.end())
    {
        // Execute method
        return it->second(args);
    }

    // Method not found
    return ScriptMethodResult::Error("EntityScriptInterface: Unknown method '" + methodName + "'");
}

//----------------------------------------------------------------------------------------------------
std::vector<ScriptMethodInfo> EntityScriptInterface::GetAvailableMethods() const
{
    return {
        // Entity management only (camera methods moved to CameraScriptInterface)
        ScriptMethodInfo("createMesh",
                         "Create a mesh entity (async with callback)",
                         {"string type", "object properties", "function callback"},
                         "number callbackId"),
        ScriptMethodInfo("updatePosition",
                         "Update entity position (absolute)",
                         {"number entityId", "object position"},
                         "void"),
        ScriptMethodInfo("moveBy",
                         "Move entity by delta (relative)",
                         {"number entityId", "object delta"},
                         "void"),
        ScriptMethodInfo("updateOrientation",
                         "Update entity orientation (Euler angles)",
                         {"number entityId", "object orientation"},
                         "void"),
        ScriptMethodInfo("updateColor",
                         "Update entity color (RGBA)",
                         {"number entityId", "object color"},
                         "void"),
        ScriptMethodInfo("destroy",
                         "Destroy entity",
                         {"number entityId"},
                         "void")
    };
}

//----------------------------------------------------------------------------------------------------
std::vector<String> EntityScriptInterface::GetAvailableProperties() const
{
    // No properties exposed in Phase 2
    return {};
}

//----------------------------------------------------------------------------------------------------
std::any EntityScriptInterface::GetProperty(String const& propertyName) const
{
    (void)propertyName; // Unused in Phase 2
    // No properties in Phase 2
    return std::any{};
}

//----------------------------------------------------------------------------------------------------
bool EntityScriptInterface::SetProperty(String const& propertyName, std::any const& value)
{
    (void)propertyName; // Unused in Phase 2
    (void)value;        // Unused in Phase 2
    // No properties in Phase 2
    return false;
}

//----------------------------------------------------------------------------------------------------
// Entity Management Methods
//----------------------------------------------------------------------------------------------------

ScriptMethodResult EntityScriptInterface::ExecuteCreateMesh(ScriptArgs const& args) const
{
    // Debug logging
    DebuggerPrintf("[DEBUG] ExecuteCreateMesh called with %zu arguments\n", args.size());
    for (size_t i = 0; i < args.size(); ++i)
    {
        DebuggerPrintf("[DEBUG]   arg[%zu] type: %s\n", i, args[i].type().name());
    }

    // NEW FLATTENED API: createMesh(meshType, posX, posY, posZ, scale, colorR, colorG, colorB, colorA, callback)
    // Total: 10 arguments (8 primitives + 1 string + 1 function)
    if (args.size() != 10)
    {
        return ScriptMethodResult::Error("createMesh: Expected 10 arguments (meshType, posX, posY, posZ, scale, colorR, colorG, colorB, colorA, callback), got " +
            std::to_string(args.size()));
    }

    try
    {
        // Extract mesh type (string)
        std::string meshType = std::any_cast<std::string>(args[0]);

        // Extract position components (3 doubles)
        double posX = std::any_cast<double>(args[1]);
        double posY = std::any_cast<double>(args[2]);
        double posZ = std::any_cast<double>(args[3]);
        Vec3   position(static_cast<float>(posX), static_cast<float>(posY), static_cast<float>(posZ));

        // Extract scale (double)
        double scaleDouble = std::any_cast<double>(args[4]);
        float  scale       = static_cast<float>(scaleDouble);

        // Extract color components (4 doubles)
        double colorR = std::any_cast<double>(args[5]);
        double colorG = std::any_cast<double>(args[6]);
        double colorB = std::any_cast<double>(args[7]);
        double colorA = std::any_cast<double>(args[8]);
        Rgba8  color(static_cast<unsigned char>(colorR),
                    static_cast<unsigned char>(colorG),
                    static_cast<unsigned char>(colorB),
                    static_cast<unsigned char>(colorA));

        // Extract callback (function)
        auto callbackOpt = ExtractCallback(args[9]);
        if (!callbackOpt.has_value())
        {
            return ScriptMethodResult::Error("createMesh: Invalid callback function");
        }

        DebuggerPrintf("[DEBUG] ExecuteCreateMesh - Parsed: meshType=%s, pos=(%.1f,%.1f,%.1f), scale=%.1f, color=(%d,%d,%d,%d)\n",
                       meshType.c_str(), position.x, position.y, position.z, scale,
                       color.r, color.g, color.b, color.a);

        // Call HighLevelEntityAPI
        CallbackID callbackId = m_entityAPI->CreateMesh(meshType,
                                                        position,
                                                        scale,
                                                        color,
                                                        callbackOpt.value());

        // Return callback ID as double (JavaScript numbers are IEEE-754 doubles)
        // V8 cannot directly marshal uint64_t, so we explicitly cast to double
        double callbackIdDouble = static_cast<double>(callbackId);
        return ScriptMethodResult::Success(callbackIdDouble);
    }
    catch (std::bad_any_cast const& e)
    {
        return ScriptMethodResult::Error("createMesh: Type conversion error - " + std::string(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult EntityScriptInterface::ExecuteUpdatePosition(ScriptArgs const& args)
{
    // FLATTENED API: V8 cannot handle nested objects, expect individual primitive arguments
    // Signature: updatePosition(entityId, posX, posY, posZ)
    // Validate argument count
    if (args.size() != 4)
    {
        return ScriptMethodResult::Error("updatePosition: Expected 4 arguments (entityId, posX, posY, posZ), got " +
            std::to_string(args.size()));
    }

    try
    {
        // Extract entity ID
        auto entityIdOpt = ExtractEntityID(args[0]);
        if (!entityIdOpt.has_value())
        {
            return ScriptMethodResult::Error("updatePosition: Invalid entityId");
        }

        // Extract position components (flattened - doubles from JavaScript)
        double posX = std::any_cast<double>(args[1]);
        double posY = std::any_cast<double>(args[2]);
        double posZ = std::any_cast<double>(args[3]);

        Vec3 position(static_cast<float>(posX),
                      static_cast<float>(posY),
                      static_cast<float>(posZ));

        // Call HighLevelEntityAPI
        m_entityAPI->UpdatePosition(entityIdOpt.value(), position);

        return ScriptMethodResult::Success();
    }
    catch (std::bad_any_cast const& e)
    {
        return ScriptMethodResult::Error("updatePosition: Type conversion error - " + std::string(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult EntityScriptInterface::ExecuteMoveBy(ScriptArgs const& args)
{
    // FLATTENED API: V8 cannot handle nested objects, expect individual primitive arguments
    // Signature: moveBy(entityId, dx, dy, dz)
    // Validate argument count
    if (args.size() != 4)
    {
        return ScriptMethodResult::Error("moveBy: Expected 4 arguments (entityId, dx, dy, dz), got " +
            std::to_string(args.size()));
    }

    try
    {
        // Extract entity ID
        auto entityIdOpt = ExtractEntityID(args[0]);
        if (!entityIdOpt.has_value())
        {
            return ScriptMethodResult::Error("moveBy: Invalid entityId");
        }

        // Extract delta components (flattened - doubles from JavaScript)
        double dx = std::any_cast<double>(args[1]);
        double dy = std::any_cast<double>(args[2]);
        double dz = std::any_cast<double>(args[3]);

        Vec3 delta(static_cast<float>(dx),
                   static_cast<float>(dy),
                   static_cast<float>(dz));

        // Call HighLevelEntityAPI
        m_entityAPI->MoveBy(entityIdOpt.value(), delta);

        return ScriptMethodResult::Success();
    }
    catch (std::bad_any_cast const& e)
    {
        return ScriptMethodResult::Error("moveBy: Type conversion error - " + std::string(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult EntityScriptInterface::ExecuteUpdateOrientation(ScriptArgs const& args)
{
    // FLATTENED API: V8 cannot handle nested objects, expect individual primitive arguments
    // Signature: updateOrientation(entityId, yaw, pitch, roll)
    // Validate argument count
    if (args.size() != 4)
    {
        return ScriptMethodResult::Error("updateOrientation: Expected 4 arguments (entityId, yaw, pitch, roll), got " +
            std::to_string(args.size()));
    }

    try
    {
        // Extract entity ID
        auto entityIdOpt = ExtractEntityID(args[0]);
        if (!entityIdOpt.has_value())
        {
            return ScriptMethodResult::Error("updateOrientation: Invalid entityId");
        }

        // Extract orientation components (flattened - doubles from JavaScript)
        double yaw   = std::any_cast<double>(args[1]);
        double pitch = std::any_cast<double>(args[2]);
        double roll  = std::any_cast<double>(args[3]);

        EulerAngles orientation(static_cast<float>(yaw),
                                static_cast<float>(pitch),
                                static_cast<float>(roll));

        // Call HighLevelEntityAPI
        m_entityAPI->UpdateOrientation(entityIdOpt.value(), orientation);

        return ScriptMethodResult::Success();
    }
    catch (std::bad_any_cast const& e)
    {
        return ScriptMethodResult::Error("updateOrientation: Type conversion error - " + std::string(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult EntityScriptInterface::ExecuteUpdateColor(ScriptArgs const& args)
{
    // FLATTENED API: V8 cannot handle nested objects, expect individual primitive arguments
    // Signature: updateColor(entityId, r, g, b, a)
    // Validate argument count
    if (args.size() != 5)
    {
        return ScriptMethodResult::Error("updateColor: Expected 5 arguments (entityId, r, g, b, a), got " +
            std::to_string(args.size()));
    }

    try
    {
        // Extract entity ID
        auto entityIdOpt = ExtractEntityID(args[0]);
        if (!entityIdOpt.has_value())
        {
            return ScriptMethodResult::Error("updateColor: Invalid entityId");
        }

        // Extract color components (flattened - doubles from JavaScript)
        double r = std::any_cast<double>(args[1]);
        double g = std::any_cast<double>(args[2]);
        double b = std::any_cast<double>(args[3]);
        double a = std::any_cast<double>(args[4]);

        Rgba8 color(static_cast<unsigned char>(r),
                    static_cast<unsigned char>(g),
                    static_cast<unsigned char>(b),
                    static_cast<unsigned char>(a));

        // Call HighLevelEntityAPI
        m_entityAPI->UpdateColor(entityIdOpt.value(), color);

        return ScriptMethodResult::Success();
    }
    catch (std::bad_any_cast const& e)
    {
        return ScriptMethodResult::Error("updateColor: Type conversion error - " + std::string(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult EntityScriptInterface::ExecuteDestroyEntity(ScriptArgs const& args)
{
    // Validate argument count
    if (args.size() != 1)
    {
        return ScriptMethodResult::Error("destroy: Expected 1 argument (entityId), got " +
            std::to_string(args.size()));
    }

    try
    {
        // Extract entity ID
        auto entityIdOpt = ExtractEntityID(args[0]);
        if (!entityIdOpt.has_value())
        {
            return ScriptMethodResult::Error("destroy: Invalid entityId");
        }

        // Call HighLevelEntityAPI
        m_entityAPI->DestroyEntity(entityIdOpt.value());

        return ScriptMethodResult::Success();
    }
    catch (std::bad_any_cast const& e)
    {
        return ScriptMethodResult::Error("destroy: Type conversion error - " + std::string(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
// Helper Methods
//----------------------------------------------------------------------------------------------------

std::optional<Vec3> EntityScriptInterface::ExtractVec3(std::any const& value) const
{
    try
    {
        // Expect JavaScript object: {x: number, y: number, z: number}
        auto map = std::any_cast<std::unordered_map<std::string, std::any>>(value);

        double x = std::any_cast<double>(map["x"]);
        double y = std::any_cast<double>(map["y"]);
        double z = std::any_cast<double>(map["z"]);

        return Vec3(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
    }
    catch (...)
    {
        return std::nullopt;
    }
}

//----------------------------------------------------------------------------------------------------
std::optional<Rgba8> EntityScriptInterface::ExtractRgba8(std::any const& value) const
{
    try
    {
        // Expect JavaScript object: {r: number, g: number, b: number, a: number}
        auto map = std::any_cast<std::unordered_map<std::string, std::any>>(value);

        double r = std::any_cast<double>(map["r"]);
        double g = std::any_cast<double>(map["g"]);
        double b = std::any_cast<double>(map["b"]);
        double a = std::any_cast<double>(map["a"]);

        return Rgba8(static_cast<unsigned char>(r),
                     static_cast<unsigned char>(g),
                     static_cast<unsigned char>(b),
                     static_cast<unsigned char>(a));
    }
    catch (...)
    {
        return std::nullopt;
    }
}

//----------------------------------------------------------------------------------------------------
std::optional<EulerAngles> EntityScriptInterface::ExtractEulerAngles(std::any const& value) const
{
    try
    {
        // Expect JavaScript object: {yaw: number, pitch: number, roll: number}
        auto map = std::any_cast<std::unordered_map<std::string, std::any>>(value);

        double yaw   = std::any_cast<double>(map["yaw"]);
        double pitch = std::any_cast<double>(map["pitch"]);
        double roll  = std::any_cast<double>(map["roll"]);

        return EulerAngles(static_cast<float>(yaw), static_cast<float>(pitch), static_cast<float>(roll));
    }
    catch (...)
    {
        return std::nullopt;
    }
}

//----------------------------------------------------------------------------------------------------
std::optional<EntityID> EntityScriptInterface::ExtractEntityID(std::any const& value) const
{
    try
    {
        // JavaScript numbers are doubles, convert to uint64_t
        double idDouble = std::any_cast<double>(value);
        return static_cast<EntityID>(idDouble);
    }
    catch (...)
    {
        return std::nullopt;
    }
}

//----------------------------------------------------------------------------------------------------
std::optional<ScriptCallback> EntityScriptInterface::ExtractCallback(std::any const& value) const
{
    // Callback is already a std::any, just return it as-is
    // V8Subsystem will handle conversion to v8::Function when executing
    return value;
}

//----------------------------------------------------------------------------------------------------
// Design Notes
//
// M4-T8 API Splitting Implementation:
//   - Changed from HighLevelEntityAPI to EntityAPI
//   - Removed all camera methods (moved to CameraScriptInterface)
//   - Maintains identical signature patterns for entity operations
//   - Uses EntityAPI instead of HighLevelEntityAPI
//   - Registered as "entity" global in JavaScript
//
// Error Handling Strategy:
//   - All methods wrapped in try-catch to handle std::bad_any_cast
//   - Helper methods return std::nullopt on extraction failure
//   - Main methods return ScriptMethodResult::Error() with descriptive message
//   - No C++ crashes on invalid JavaScript input
//
// Type Conversion:
//   - JavaScript numbers → double → float/uint64_t
//   - JavaScript objects → std::unordered_map<std::string, std::any>
//   - JavaScript functions → std::any (opaque, handled by EntityAPI)
//
// Performance Considerations:
//   - std::unordered_map lookups for method registry (O(1) average)
//   - Minimal copying (pass by const reference where possible)
//   - No allocations in hot path (command submission)
//----------------------------------------------------------------------------------------------------
