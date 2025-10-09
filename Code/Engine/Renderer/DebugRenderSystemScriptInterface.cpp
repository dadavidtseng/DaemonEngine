//----------------------------------------------------------------------------------------------------
// DebugRenderSystemScriptInterface.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Renderer/DebugRenderSystemScriptInterface.hpp"

//----------------------------------------------------------------------------------------------------
#include "Engine/Renderer/DebugRenderSystem.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/LogSubsystem.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/Mat44.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Script/ScriptTypeExtractor.hpp"

//----------------------------------------------------------------------------------------------------
DebugRenderSystemScriptInterface::DebugRenderSystemScriptInterface()
{
    // Initialize method registry for efficient dispatch
    DebugRenderSystemScriptInterface::InitializeMethodRegistry();
}

//----------------------------------------------------------------------------------------------------
void DebugRenderSystemScriptInterface::InitializeMethodRegistry()
{
    // === CONTROL METHODS ===
    m_methodRegistry["setVisible"] = [this](ScriptArgs const& args) { return ExecuteSetVisible(args); };
    m_methodRegistry["setHidden"]  = [this](ScriptArgs const& args) { return ExecuteSetHidden(args); };
    m_methodRegistry["clear"]      = [this](ScriptArgs const& args) { return ExecuteClear(args); };

    // === OUTPUT METHODS ===
    m_methodRegistry["beginFrame"]   = [this](ScriptArgs const& args) { return ExecuteBeginFrame(args); };
    m_methodRegistry["renderWorld"]  = [this](ScriptArgs const& args) { return ExecuteRenderWorld(args); };
    m_methodRegistry["renderScreen"] = [this](ScriptArgs const& args) { return ExecuteRenderScreen(args); };
    m_methodRegistry["endFrame"]     = [this](ScriptArgs const& args) { return ExecuteEndFrame(args); };

    // === GEOMETRY METHODS - WORLD SPACE ===
    m_methodRegistry["addWorldPoint"]      = [this](ScriptArgs const& args) { return ExecuteAddWorldPoint(args); };
    m_methodRegistry["addWorldLine"]       = [this](ScriptArgs const& args) { return ExecuteAddWorldLine(args); };
    m_methodRegistry["addWorldCylinder"]   = [this](ScriptArgs const& args) { return ExecuteAddWorldCylinder(args); };
    m_methodRegistry["addWorldWireSphere"] = [this](ScriptArgs const& args) { return ExecuteAddWorldWireSphere(args); };
    m_methodRegistry["addWorldArrow"]      = [this](ScriptArgs const& args) { return ExecuteAddWorldArrow(args); };
    m_methodRegistry["addWorldText"]       = [this](ScriptArgs const& args) { return ExecuteAddWorldText(args); };
    m_methodRegistry["addBillboardText"]   = [this](ScriptArgs const& args) { return ExecuteAddBillboardText(args); };
    m_methodRegistry["addWorldBasis"]      = [this](ScriptArgs const& args) { return ExecuteAddWorldBasis(args); };

    // === GEOMETRY METHODS - SCREEN SPACE ===
    m_methodRegistry["addScreenText"] = [this](ScriptArgs const& args) { return ExecuteAddScreenText(args); };
    m_methodRegistry["addMessage"]    = [this](ScriptArgs const& args) { return ExecuteAddMessage(args); };
}

//----------------------------------------------------------------------------------------------------
std::vector<ScriptMethodInfo> DebugRenderSystemScriptInterface::GetAvailableMethods() const
{
    return {
        // === CONTROL METHODS ===
        ScriptMethodInfo("setVisible",
                         "Make debug rendering visible",
                         {},
                         "void"),

        ScriptMethodInfo("setHidden",
                         "Hide debug rendering",
                         {},
                         "void"),

        ScriptMethodInfo("clear",
                         "Clear all debug rendering objects",
                         {},
                         "void"),

        // === OUTPUT METHODS ===
        ScriptMethodInfo("beginFrame",
                         "Begin debug rendering frame",
                         {},
                         "void"),

        ScriptMethodInfo("renderWorld",
                         "Render world-space debug objects with specified camera",
                         {"number"},
                         "void"),

        ScriptMethodInfo("renderScreen",
                         "Render screen-space debug objects with specified camera",
                         {"number"},
                         "void"),

        ScriptMethodInfo("endFrame",
                         "End debug rendering frame",
                         {},
                         "void"),

        // === GEOMETRY METHODS - WORLD SPACE ===
        ScriptMethodInfo("addWorldPoint",
                         "Add debug point in world space (x, y, z, radius, duration, r, g, b, a, mode)",
                         {"number", "number", "number", "number", "number", "number", "number", "number", "number", "string"},
                         "void"),

        ScriptMethodInfo("addWorldLine",
                         "Add debug line in world space (x1, y1, z1, x2, y2, z2, radius, duration, r, g, b, a, mode)",
                         {"number", "number", "number", "number", "number", "number", "number", "number", "number", "number", "number", "number", "string"},
                         "void"),

        ScriptMethodInfo("addWorldCylinder",
                         "Add debug cylinder in world space (baseX, baseY, baseZ, topX, topY, topZ, radius, duration, isWireframe, r, g, b, a, mode)",
                         {"number", "number", "number", "number", "number", "number", "number", "number", "bool", "number", "number", "number", "number", "string"},
                         "void"),

        ScriptMethodInfo("addWorldWireSphere",
                         "Add debug wire sphere in world space (x, y, z, radius, duration, r, g, b, a, mode)",
                         {"number", "number", "number", "number", "number", "number", "number", "number", "number", "string"},
                         "void"),

        ScriptMethodInfo("addWorldArrow",
                         "Add debug arrow in world space (x1, y1, z1, x2, y2, z2, radius, duration, r, g, b, a, mode)",
                         {"number", "number", "number", "number", "number", "number", "number", "number", "number", "number", "number", "number", "string"},
                         "void"),

        ScriptMethodInfo("addWorldText",
                         "Add debug text in world space (text, transform[16], textHeight, alignX, alignY, duration, r, g, b, a, mode)",
                         {"string", "array", "number", "number", "number", "number", "number", "number", "number", "number", "string"},
                         "void"),

        ScriptMethodInfo("addBillboardText",
                         "Add billboard text in world space (text, x, y, z, textHeight, alignX, alignY, duration, r, g, b, a, mode)",
                         {"string", "number", "number", "number", "number", "number", "number", "number", "number", "number", "number", "number", "string"},
                         "void"),

        ScriptMethodInfo("addWorldBasis",
                         "Add debug coordinate basis in world space (transform[16], duration, mode)",
                         {"array", "number", "string"},
                         "void"),

        // === GEOMETRY METHODS - SCREEN SPACE ===
        ScriptMethodInfo("addScreenText",
                         "Add debug text in screen space (text, x, y, size, alignX, alignY, duration, r, g, b, a)",
                         {"string", "number", "number", "number", "number", "number", "number", "number", "number", "number", "number"},
                         "void"),

        ScriptMethodInfo("addMessage",
                         "Add debug message (text, duration, r, g, b, a)",
                         {"string", "number", "number", "number", "number", "number"},
                         "void")
    };
}

//----------------------------------------------------------------------------------------------------
StringList DebugRenderSystemScriptInterface::GetAvailableProperties() const
{
    return {
        // DebugRenderSystem doesn't currently expose properties
    };
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult DebugRenderSystemScriptInterface::CallMethod(String const& methodName, ScriptArgs const& args)
{
    try
    {
        auto it = m_methodRegistry.find(methodName);
        if (it != m_methodRegistry.end())
        {
            return it->second(args);
        }

        return ScriptMethodResult::Error("Unknown debug render method: " + methodName);
    }
    catch (std::exception const& e)
    {
        return ScriptMethodResult::Error("Debug render method execution failed: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
std::any DebugRenderSystemScriptInterface::GetProperty(String const& propertyName) const
{
    // No properties currently implemented
    UNUSED(propertyName)
    return std::any{};
}

//----------------------------------------------------------------------------------------------------
bool DebugRenderSystemScriptInterface::SetProperty(String const& propertyName, std::any const& value)
{
    // No properties currently implemented
    UNUSED(propertyName)
    UNUSED(value)
    return false;
}

//----------------------------------------------------------------------------------------------------
// CONTROL METHODS
//----------------------------------------------------------------------------------------------------

ScriptMethodResult DebugRenderSystemScriptInterface::ExecuteSetVisible(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 0, "setVisible");
    if (!result.success) return result;

    try
    {
        DebugRenderSetVisible();
        return ScriptMethodResult::Success("Debug rendering set to visible");
    }
    catch (const std::exception& e)
    {
        return ScriptMethodResult::Error("Failed to set visible: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult DebugRenderSystemScriptInterface::ExecuteSetHidden(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 0, "setHidden");
    if (!result.success) return result;

    try
    {
        DebugRenderSetHidden();
        return ScriptMethodResult::Success("Debug rendering set to hidden");
    }
    catch (const std::exception& e)
    {
        return ScriptMethodResult::Error("Failed to set hidden: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult DebugRenderSystemScriptInterface::ExecuteClear(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 0, "clear");
    if (!result.success) return result;

    try
    {
        DebugRenderClear();
        return ScriptMethodResult::Success("Debug rendering cleared");
    }
    catch (const std::exception& e)
    {
        return ScriptMethodResult::Error("Failed to clear: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
// OUTPUT METHODS
//----------------------------------------------------------------------------------------------------

ScriptMethodResult DebugRenderSystemScriptInterface::ExecuteBeginFrame(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 0, "beginFrame");
    if (!result.success) return result;

    try
    {
        DebugRenderBeginFrame();
        return ScriptMethodResult::Success("Debug render frame begun");
    }
    catch (const std::exception& e)
    {
        return ScriptMethodResult::Error("Failed to begin frame: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult DebugRenderSystemScriptInterface::ExecuteRenderWorld(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 1, "renderWorld");
    if (!result.success) return result;

    try
    {
        // Extract camera handle (pointer as number)
        double cameraHandle = ScriptTypeExtractor::ExtractDouble(args[0]);
        Camera* camera = reinterpret_cast<Camera*>(static_cast<uintptr_t>(cameraHandle));

        if (!camera)
        {
            return ScriptMethodResult::Error("Invalid camera handle");
        }

        DebugRenderWorld(*camera);
        return ScriptMethodResult::Success("Debug world rendered");
    }
    catch (const std::exception& e)
    {
        return ScriptMethodResult::Error("Failed to render world: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult DebugRenderSystemScriptInterface::ExecuteRenderScreen(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 1, "renderScreen");
    if (!result.success) return result;

    try
    {
        // Extract camera handle (pointer as number)
        double cameraHandle = ScriptTypeExtractor::ExtractDouble(args[0]);
        Camera* camera = reinterpret_cast<Camera*>(static_cast<uintptr_t>(cameraHandle));

        if (!camera)
        {
            return ScriptMethodResult::Error("Invalid camera handle");
        }

        DebugRenderScreen(*camera);
        return ScriptMethodResult::Success("Debug screen rendered");
    }
    catch (const std::exception& e)
    {
        return ScriptMethodResult::Error("Failed to render screen: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult DebugRenderSystemScriptInterface::ExecuteEndFrame(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 0, "endFrame");
    if (!result.success) return result;

    try
    {
        DebugRenderEndFrame();
        return ScriptMethodResult::Success("Debug render frame ended");
    }
    catch (const std::exception& e)
    {
        return ScriptMethodResult::Error("Failed to end frame: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
// GEOMETRY METHODS - WORLD SPACE
//----------------------------------------------------------------------------------------------------

ScriptMethodResult DebugRenderSystemScriptInterface::ExecuteAddWorldPoint(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 10, "addWorldPoint");
    if (!result.success) return result;

    try
    {
        float  x        = ScriptTypeExtractor::ExtractFloat(args[0]);
        float  y        = ScriptTypeExtractor::ExtractFloat(args[1]);
        float  z        = ScriptTypeExtractor::ExtractFloat(args[2]);
        float  radius   = ScriptTypeExtractor::ExtractFloat(args[3]);
        float  duration = ScriptTypeExtractor::ExtractFloat(args[4]);
        int    r        = ScriptTypeExtractor::ExtractInt(args[5]);
        int    g        = ScriptTypeExtractor::ExtractInt(args[6]);
        int    b        = ScriptTypeExtractor::ExtractInt(args[7]);
        int    a        = ScriptTypeExtractor::ExtractInt(args[8]);
        String modeStr  = ScriptTypeExtractor::ExtractString(args[9]);

        if (!ValidatePosition(x, y, z))
        {
            return ScriptMethodResult::Error("Invalid position coordinates");
        }
        if (!ValidateColor(r, g, b, a))
        {
            return ScriptMethodResult::Error("Color values must be between 0 and 255");
        }
        if (!ValidateDuration(duration))
        {
            return ScriptMethodResult::Error("Duration must be non-negative");
        }

        Vec3              pos(x, y, z);
        Rgba8             color(static_cast<unsigned char>(r), static_cast<unsigned char>(g), static_cast<unsigned char>(b), static_cast<unsigned char>(a));
        eDebugRenderMode  mode = static_cast<eDebugRenderMode>(StringToDebugRenderMode(modeStr));

        DebugAddWorldPoint(pos, radius, duration, color, color, mode);
        return ScriptMethodResult::Success("World point added");
    }
    catch (const std::exception& e)
    {
        return ScriptMethodResult::Error("Failed to add world point: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult DebugRenderSystemScriptInterface::ExecuteAddWorldLine(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 13, "addWorldLine");
    if (!result.success) return result;

    try
    {
        float  x1       = ScriptTypeExtractor::ExtractFloat(args[0]);
        float  y1       = ScriptTypeExtractor::ExtractFloat(args[1]);
        float  z1       = ScriptTypeExtractor::ExtractFloat(args[2]);
        float  x2       = ScriptTypeExtractor::ExtractFloat(args[3]);
        float  y2       = ScriptTypeExtractor::ExtractFloat(args[4]);
        float  z2       = ScriptTypeExtractor::ExtractFloat(args[5]);
        float  radius   = ScriptTypeExtractor::ExtractFloat(args[6]);
        float  duration = ScriptTypeExtractor::ExtractFloat(args[7]);
        int    r        = ScriptTypeExtractor::ExtractInt(args[8]);
        int    g        = ScriptTypeExtractor::ExtractInt(args[9]);
        int    b        = ScriptTypeExtractor::ExtractInt(args[10]);
        int    a        = ScriptTypeExtractor::ExtractInt(args[11]);
        String modeStr  = ScriptTypeExtractor::ExtractString(args[12]);

        if (!ValidatePosition(x1, y1, z1) || !ValidatePosition(x2, y2, z2))
        {
            return ScriptMethodResult::Error("Invalid position coordinates");
        }
        if (!ValidateColor(r, g, b, a))
        {
            return ScriptMethodResult::Error("Color values must be between 0 and 255");
        }
        if (!ValidateDuration(duration))
        {
            return ScriptMethodResult::Error("Duration must be non-negative");
        }

        Vec3              start(x1, y1, z1);
        Vec3              end(x2, y2, z2);
        Rgba8             color(static_cast<unsigned char>(r), static_cast<unsigned char>(g), static_cast<unsigned char>(b), static_cast<unsigned char>(a));
        eDebugRenderMode  mode = static_cast<eDebugRenderMode>(StringToDebugRenderMode(modeStr));

        DebugAddWorldLine(start, end, radius, duration, color, color, mode);
        return ScriptMethodResult::Success("World line added");
    }
    catch (const std::exception& e)
    {
        return ScriptMethodResult::Error("Failed to add world line: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult DebugRenderSystemScriptInterface::ExecuteAddWorldCylinder(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 14, "addWorldCylinder");
    if (!result.success) return result;

    try
    {
        float  baseX       = ScriptTypeExtractor::ExtractFloat(args[0]);
        float  baseY       = ScriptTypeExtractor::ExtractFloat(args[1]);
        float  baseZ       = ScriptTypeExtractor::ExtractFloat(args[2]);
        float  topX        = ScriptTypeExtractor::ExtractFloat(args[3]);
        float  topY        = ScriptTypeExtractor::ExtractFloat(args[4]);
        float  topZ        = ScriptTypeExtractor::ExtractFloat(args[5]);
        float  radius      = ScriptTypeExtractor::ExtractFloat(args[6]);
        float  duration    = ScriptTypeExtractor::ExtractFloat(args[7]);
        bool   isWireframe = ScriptTypeExtractor::ExtractBool(args[8]);
        int    r           = ScriptTypeExtractor::ExtractInt(args[9]);
        int    g           = ScriptTypeExtractor::ExtractInt(args[10]);
        int    b           = ScriptTypeExtractor::ExtractInt(args[11]);
        int    a           = ScriptTypeExtractor::ExtractInt(args[12]);
        String modeStr     = ScriptTypeExtractor::ExtractString(args[13]);

        if (!ValidatePosition(baseX, baseY, baseZ) || !ValidatePosition(topX, topY, topZ))
        {
            return ScriptMethodResult::Error("Invalid position coordinates");
        }
        if (!ValidateColor(r, g, b, a))
        {
            return ScriptMethodResult::Error("Color values must be between 0 and 255");
        }
        if (!ValidateDuration(duration))
        {
            return ScriptMethodResult::Error("Duration must be non-negative");
        }

        Vec3              base(baseX, baseY, baseZ);
        Vec3              top(topX, topY, topZ);
        Rgba8             color(static_cast<unsigned char>(r), static_cast<unsigned char>(g), static_cast<unsigned char>(b), static_cast<unsigned char>(a));
        eDebugRenderMode  mode = static_cast<eDebugRenderMode>(StringToDebugRenderMode(modeStr));

        DebugAddWorldCylinder(base, top, radius, duration, isWireframe, color, color, mode);
        return ScriptMethodResult::Success("World cylinder added");
    }
    catch (const std::exception& e)
    {
        return ScriptMethodResult::Error("Failed to add world cylinder: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult DebugRenderSystemScriptInterface::ExecuteAddWorldWireSphere(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 10, "addWorldWireSphere");
    if (!result.success) return result;

    try
    {
        float  x        = ScriptTypeExtractor::ExtractFloat(args[0]);
        float  y        = ScriptTypeExtractor::ExtractFloat(args[1]);
        float  z        = ScriptTypeExtractor::ExtractFloat(args[2]);
        float  radius   = ScriptTypeExtractor::ExtractFloat(args[3]);
        float  duration = ScriptTypeExtractor::ExtractFloat(args[4]);
        int    r        = ScriptTypeExtractor::ExtractInt(args[5]);
        int    g        = ScriptTypeExtractor::ExtractInt(args[6]);
        int    b        = ScriptTypeExtractor::ExtractInt(args[7]);
        int    a        = ScriptTypeExtractor::ExtractInt(args[8]);
        String modeStr  = ScriptTypeExtractor::ExtractString(args[9]);

        if (!ValidatePosition(x, y, z))
        {
            return ScriptMethodResult::Error("Invalid position coordinates");
        }
        if (!ValidateColor(r, g, b, a))
        {
            return ScriptMethodResult::Error("Color values must be between 0 and 255");
        }
        if (!ValidateDuration(duration))
        {
            return ScriptMethodResult::Error("Duration must be non-negative");
        }

        Vec3              center(x, y, z);
        Rgba8             color(static_cast<unsigned char>(r), static_cast<unsigned char>(g), static_cast<unsigned char>(b), static_cast<unsigned char>(a));
        eDebugRenderMode  mode = static_cast<eDebugRenderMode>(StringToDebugRenderMode(modeStr));

        DebugAddWorldWireSphere(center, radius, duration, color, color, mode);
        return ScriptMethodResult::Success("World wire sphere added");
    }
    catch (const std::exception& e)
    {
        return ScriptMethodResult::Error("Failed to add world wire sphere: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult DebugRenderSystemScriptInterface::ExecuteAddWorldArrow(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 13, "addWorldArrow");
    if (!result.success) return result;

    try
    {
        float  x1       = ScriptTypeExtractor::ExtractFloat(args[0]);
        float  y1       = ScriptTypeExtractor::ExtractFloat(args[1]);
        float  z1       = ScriptTypeExtractor::ExtractFloat(args[2]);
        float  x2       = ScriptTypeExtractor::ExtractFloat(args[3]);
        float  y2       = ScriptTypeExtractor::ExtractFloat(args[4]);
        float  z2       = ScriptTypeExtractor::ExtractFloat(args[5]);
        float  radius   = ScriptTypeExtractor::ExtractFloat(args[6]);
        float  duration = ScriptTypeExtractor::ExtractFloat(args[7]);
        int    r        = ScriptTypeExtractor::ExtractInt(args[8]);
        int    g        = ScriptTypeExtractor::ExtractInt(args[9]);
        int    b        = ScriptTypeExtractor::ExtractInt(args[10]);
        int    a        = ScriptTypeExtractor::ExtractInt(args[11]);
        String modeStr  = ScriptTypeExtractor::ExtractString(args[12]);

        if (!ValidatePosition(x1, y1, z1) || !ValidatePosition(x2, y2, z2))
        {
            return ScriptMethodResult::Error("Invalid position coordinates");
        }
        if (!ValidateColor(r, g, b, a))
        {
            return ScriptMethodResult::Error("Color values must be between 0 and 255");
        }
        if (!ValidateDuration(duration))
        {
            return ScriptMethodResult::Error("Duration must be non-negative");
        }

        Vec3              start(x1, y1, z1);
        Vec3              end(x2, y2, z2);
        Rgba8             color(static_cast<unsigned char>(r), static_cast<unsigned char>(g), static_cast<unsigned char>(b), static_cast<unsigned char>(a));
        eDebugRenderMode  mode = static_cast<eDebugRenderMode>(StringToDebugRenderMode(modeStr));

        DebugAddWorldArrow(start, end, radius, duration, color, color, mode);
        return ScriptMethodResult::Success("World arrow added");
    }
    catch (const std::exception& e)
    {
        return ScriptMethodResult::Error("Failed to add world arrow: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult DebugRenderSystemScriptInterface::ExecuteAddWorldText(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 11, "addWorldText");
    if (!result.success) return result;

    try
    {
        String text       = ScriptTypeExtractor::ExtractString(args[0]);
        // args[1] should be array of 16 numbers for transform matrix - simplified for now
        float  textHeight = ScriptTypeExtractor::ExtractFloat(args[2]);
        float  alignX     = ScriptTypeExtractor::ExtractFloat(args[3]);
        float  alignY     = ScriptTypeExtractor::ExtractFloat(args[4]);
        float  duration   = ScriptTypeExtractor::ExtractFloat(args[5]);
        int    r          = ScriptTypeExtractor::ExtractInt(args[6]);
        int    g          = ScriptTypeExtractor::ExtractInt(args[7]);
        int    b          = ScriptTypeExtractor::ExtractInt(args[8]);
        int    a          = ScriptTypeExtractor::ExtractInt(args[9]);
        String modeStr    = ScriptTypeExtractor::ExtractString(args[10]);

        if (!ValidateColor(r, g, b, a))
        {
            return ScriptMethodResult::Error("Color values must be between 0 and 255");
        }
        if (!ValidateDuration(duration))
        {
            return ScriptMethodResult::Error("Duration must be non-negative");
        }

        // For now use identity transform - full matrix parsing would be more complex
        Mat44             transform = Mat44();
        Vec2              alignment(alignX, alignY);
        Rgba8             color(static_cast<unsigned char>(r), static_cast<unsigned char>(g), static_cast<unsigned char>(b), static_cast<unsigned char>(a));
        eDebugRenderMode  mode = static_cast<eDebugRenderMode>(StringToDebugRenderMode(modeStr));

        DebugAddWorldText(text, transform, textHeight, alignment, duration, color, color, mode);
        return ScriptMethodResult::Success("World text added");
    }
    catch (const std::exception& e)
    {
        return ScriptMethodResult::Error("Failed to add world text: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult DebugRenderSystemScriptInterface::ExecuteAddBillboardText(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 12, "addBillboardText");
    if (!result.success) return result;

    try
    {
        String text       = ScriptTypeExtractor::ExtractString(args[0]);
        float  x          = ScriptTypeExtractor::ExtractFloat(args[1]);
        float  y          = ScriptTypeExtractor::ExtractFloat(args[2]);
        float  z          = ScriptTypeExtractor::ExtractFloat(args[3]);
        float  textHeight = ScriptTypeExtractor::ExtractFloat(args[4]);
        float  alignX     = ScriptTypeExtractor::ExtractFloat(args[5]);
        float  alignY     = ScriptTypeExtractor::ExtractFloat(args[6]);
        float  duration   = ScriptTypeExtractor::ExtractFloat(args[7]);
        int    r          = ScriptTypeExtractor::ExtractInt(args[8]);
        int    g          = ScriptTypeExtractor::ExtractInt(args[9]);
        int    b          = ScriptTypeExtractor::ExtractInt(args[10]);
        int    a          = ScriptTypeExtractor::ExtractInt(args[11]);
        String modeStr    = ScriptTypeExtractor::ExtractString(args[12]);

        if (!ValidatePosition(x, y, z))
        {
            return ScriptMethodResult::Error("Invalid position coordinates");
        }
        if (!ValidateColor(r, g, b, a))
        {
            return ScriptMethodResult::Error("Color values must be between 0 and 255");
        }
        if (!ValidateDuration(duration))
        {
            return ScriptMethodResult::Error("Duration must be non-negative");
        }

        Vec3              origin(x, y, z);
        Vec2              alignment(alignX, alignY);
        Rgba8             color(static_cast<unsigned char>(r), static_cast<unsigned char>(g), static_cast<unsigned char>(b), static_cast<unsigned char>(a));
        eDebugRenderMode  mode = static_cast<eDebugRenderMode>(StringToDebugRenderMode(modeStr));

        DebugAddBillboardText(text, origin, textHeight, alignment, duration, color, color, mode);
        return ScriptMethodResult::Success("Billboard text added");
    }
    catch (const std::exception& e)
    {
        return ScriptMethodResult::Error("Failed to add billboard text: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult DebugRenderSystemScriptInterface::ExecuteAddWorldBasis(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 3, "addWorldBasis");
    if (!result.success) return result;

    try
    {
        // args[0] should be array of 16 numbers for transform matrix - simplified for now
        float  duration = ScriptTypeExtractor::ExtractFloat(args[1]);
        String modeStr  = ScriptTypeExtractor::ExtractString(args[2]);

        if (!ValidateDuration(duration))
        {
            return ScriptMethodResult::Error("Duration must be non-negative");
        }

        // For now use identity transform - full matrix parsing would be more complex
        Mat44            transform = Mat44();
        eDebugRenderMode mode      = static_cast<eDebugRenderMode>(StringToDebugRenderMode(modeStr));

        DebugAddWorldBasis(transform, duration, mode);
        return ScriptMethodResult::Success("World basis added");
    }
    catch (const std::exception& e)
    {
        return ScriptMethodResult::Error("Failed to add world basis: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
// GEOMETRY METHODS - SCREEN SPACE
//----------------------------------------------------------------------------------------------------

ScriptMethodResult DebugRenderSystemScriptInterface::ExecuteAddScreenText(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 11, "addScreenText");
    if (!result.success) return result;

    try
    {
        String text     = ScriptTypeExtractor::ExtractString(args[0]);
        float  x        = ScriptTypeExtractor::ExtractFloat(args[1]);
        float  y        = ScriptTypeExtractor::ExtractFloat(args[2]);
        float  size     = ScriptTypeExtractor::ExtractFloat(args[3]);
        float  alignX   = ScriptTypeExtractor::ExtractFloat(args[4]);
        float  alignY   = ScriptTypeExtractor::ExtractFloat(args[5]);
        float  duration = ScriptTypeExtractor::ExtractFloat(args[6]);
        int    r        = ScriptTypeExtractor::ExtractInt(args[7]);
        int    g        = ScriptTypeExtractor::ExtractInt(args[8]);
        int    b        = ScriptTypeExtractor::ExtractInt(args[9]);
        int    a        = ScriptTypeExtractor::ExtractInt(args[10]);

        if (!ValidateColor(r, g, b, a))
        {
            return ScriptMethodResult::Error("Color values must be between 0 and 255");
        }
        if (!ValidateDuration(duration))
        {
            return ScriptMethodResult::Error("Duration must be non-negative");
        }

        Vec2  position(x, y);
        Vec2  alignment(alignX, alignY);
        Rgba8 color(static_cast<unsigned char>(r), static_cast<unsigned char>(g), static_cast<unsigned char>(b), static_cast<unsigned char>(a));

        DebugAddScreenText(text, position, size, alignment, duration, color, color);
        return ScriptMethodResult::Success("Screen text added");
    }
    catch (const std::exception& e)
    {
        return ScriptMethodResult::Error("Failed to add screen text: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult DebugRenderSystemScriptInterface::ExecuteAddMessage(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 6, "addMessage");
    if (!result.success) return result;

    try
    {
        String text     = ScriptTypeExtractor::ExtractString(args[0]);
        float  duration = ScriptTypeExtractor::ExtractFloat(args[1]);
        int    r        = ScriptTypeExtractor::ExtractInt(args[2]);
        int    g        = ScriptTypeExtractor::ExtractInt(args[3]);
        int    b        = ScriptTypeExtractor::ExtractInt(args[4]);
        int    a        = ScriptTypeExtractor::ExtractInt(args[5]);

        if (!ValidateColor(r, g, b, a))
        {
            return ScriptMethodResult::Error("Color values must be between 0 and 255");
        }
        if (!ValidateDuration(duration))
        {
            return ScriptMethodResult::Error("Duration must be non-negative");
        }

        Rgba8 color(static_cast<unsigned char>(r), static_cast<unsigned char>(g), static_cast<unsigned char>(b), static_cast<unsigned char>(a));

        DebugAddMessage(text, duration, color, color);
        return ScriptMethodResult::Success("Message added");
    }
    catch (const std::exception& e)
    {
        return ScriptMethodResult::Error("Failed to add message: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
// VALIDATION
//----------------------------------------------------------------------------------------------------

bool DebugRenderSystemScriptInterface::ValidateColor(int r, int g, int b, int a) const
{
    return (r >= 0 && r <= 255 && g >= 0 && g <= 255 && b >= 0 && b <= 255 && a >= 0 && a <= 255);
}

//----------------------------------------------------------------------------------------------------
bool DebugRenderSystemScriptInterface::ValidatePosition(float x, float y, float z) const
{
    return (std::isfinite(x) && std::isfinite(y) && std::isfinite(z) &&
        std::abs(x) < 10000.0f && std::abs(y) < 10000.0f && std::abs(z) < 10000.0f);
}

//----------------------------------------------------------------------------------------------------
bool DebugRenderSystemScriptInterface::ValidateDuration(float duration) const
{
    return (duration >= 0.0f && std::isfinite(duration));
}

//----------------------------------------------------------------------------------------------------
int DebugRenderSystemScriptInterface::StringToDebugRenderMode(String const& modeStr) const
{
    if (modeStr == "ALWAYS")
    {
        return static_cast<int>(eDebugRenderMode::ALWAYS);
    }
    else if (modeStr == "USE_DEPTH")
    {
        return static_cast<int>(eDebugRenderMode::USE_DEPTH);
    }
    else if (modeStr == "X_RAY")
    {
        return static_cast<int>(eDebugRenderMode::X_RAY);
    }

    // Default to USE_DEPTH
    return static_cast<int>(eDebugRenderMode::USE_DEPTH);
}
