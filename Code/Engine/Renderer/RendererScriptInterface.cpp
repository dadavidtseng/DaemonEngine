//----------------------------------------------------------------------------------------------------
// RendererScriptInterface.cpp
//----------------------------------------------------------------------------------------------------
#include "Engine/Renderer/RendererScriptInterface.hpp"
//----------------------------------------------------------------------------------------------------
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/LogSubsystem.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Math/Mat44.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/VertexUtils.hpp"
#include "Engine/Resource/ResourceSubsystem.hpp"
#include "Engine/Script/ScriptTypeExtractor.hpp"

//----------------------------------------------------------------------------------------------------
RendererScriptInterface::RendererScriptInterface(Renderer* renderer)
    : m_renderer(renderer)
{
    if (!m_renderer)
    {
        ERROR_AND_DIE("RendererScriptInterface: Renderer pointer cannot be null");
    }
    RendererScriptInterface::InitializeMethodRegistry();
}

//----------------------------------------------------------------------------------------------------
void RendererScriptInterface::InitializeMethodRegistry()
{
    m_methodRegistry["setModelConstants"]   = [this](ScriptArgs const& args) { return ExecuteSetModelConstants(args); };
    m_methodRegistry["setBlendMode"]        = [this](ScriptArgs const& args) { return ExecuteSetBlendMode(args); };
    m_methodRegistry["setRasterizerMode"]   = [this](ScriptArgs const& args) { return ExecuteSetRasterizerMode(args); };
    m_methodRegistry["setSamplerMode"]      = [this](ScriptArgs const& args) { return ExecuteSetSamplerMode(args); };
    m_methodRegistry["setDepthMode"]        = [this](ScriptArgs const& args) { return ExecuteSetDepthMode(args); };
    m_methodRegistry["bindTextureCPP"]         = [this](ScriptArgs const& args) { return ExecuteBindTexture(args); };
    m_methodRegistry["bindShader"]          = [this](ScriptArgs const& args) { return ExecuteBindShader(args); };
    m_methodRegistry["drawVertexArray"]     = [this](ScriptArgs const& args) { return ExecuteDrawVertexArray(args); };
    m_methodRegistry["createVertexArrayCPP"]   = [this](ScriptArgs const& args) { return ExecuteCreateVertexArray(args); };
    m_methodRegistry["addVertex"]           = [this](ScriptArgs const& args) { return ExecuteAddVertex(args); };
    m_methodRegistry["addVertexBatch"]      = [this](ScriptArgs const& args) { return ExecuteAddVertexBatch(args); };
}

//----------------------------------------------------------------------------------------------------
std::vector<ScriptMethodInfo> RendererScriptInterface::GetAvailableMethods() const
{
    return {
        ScriptMethodInfo("setModelConstants",
                         "Set model transform and color (x,y,z, yaw,pitch,roll, r,g,b,a)",
                         {"number", "number", "number", "number", "number", "number", "number", "number", "number", "number"},
                         "void"),

        ScriptMethodInfo("setBlendMode",
                         "Set blend mode (OPAQUE, ALPHA, ADDITIVE)",
                         {"string"},
                         "void"),

        ScriptMethodInfo("setRasterizerMode",
                         "Set rasterizer mode (SOLID_CULL_NONE, SOLID_CULL_BACK, SOLID_CULL_FRONT, WIREFRAME_CULL_NONE, WIREFRAME_CULL_BACK)",
                         {"string"},
                         "void"),

        ScriptMethodInfo("setSamplerMode",
                         "Set sampler mode (POINT_CLAMP, BILINEAR_CLAMP)",
                         {"string"},
                         "void"),

        ScriptMethodInfo("setDepthMode",
                         "Set depth mode (DISABLED, READ_ONLY_ALWAYS, READ_ONLY_LESS_EQUAL, READ_WRITE_LESS_EQUAL)",
                         {"string"},
                         "void"),

        ScriptMethodInfo("bindTextureCPP",
                         "Bind texture by name (null for no texture)",
                         {"string"},
                         "void"),

        ScriptMethodInfo("bindShader",
                         "Bind shader by file path",
                         {"string"},
                         "void"),

        ScriptMethodInfo("drawVertexArray",
                         "Draw vertex array by handle ID",
                         {"string"},
                         "void"),

        ScriptMethodInfo("createVertexArrayCPP",
                         "Create new vertex array and return handle ID",
                         {},
                         "string"),

        ScriptMethodInfo("addVertex",
                         "Add vertex to current vertex array (x, y, z, r, g, b, a, u, v)",
                         {"number", "number", "number", "number", "number", "number", "number", "number", "number"},
                         "void"),

        ScriptMethodInfo("addVertexBatch",
                         "Add multiple vertices from JavaScript array [x,y,z,r,g,b,a,u,v, ...]",
                         {"array"},
                         "void"),
    };
}

//----------------------------------------------------------------------------------------------------
std::vector<String> RendererScriptInterface::GetAvailableProperties() const
{
    return {};  // No properties for now
}

//----------------------------------------------------------------------------------------------------
std::any RendererScriptInterface::GetProperty(String const& propertyName) const
{
    UNUSED(propertyName);
    return std::any{};
}

//----------------------------------------------------------------------------------------------------
bool RendererScriptInterface::SetProperty(String const& propertyName, std::any const& value)
{
    UNUSED(propertyName);
    UNUSED(value);
    return false;
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult RendererScriptInterface::CallMethod(String const& methodName, ScriptArgs const& args)
{
    if (methodName == "setModelConstants")       return ExecuteSetModelConstants(args);
    if (methodName == "setBlendMode")            return ExecuteSetBlendMode(args);
    if (methodName == "setRasterizerMode")       return ExecuteSetRasterizerMode(args);
    if (methodName == "setSamplerMode")          return ExecuteSetSamplerMode(args);
    if (methodName == "setDepthMode")            return ExecuteSetDepthMode(args);
    if (methodName == "bindTextureCPP")             return ExecuteBindTexture(args);
    if (methodName == "bindShader")              return ExecuteBindShader(args);
    if (methodName == "drawVertexArray")         return ExecuteDrawVertexArray(args);
    if (methodName == "createVertexArrayCPP")       return ExecuteCreateVertexArray(args);
    if (methodName == "addVertex")               return ExecuteAddVertex(args);
    if (methodName == "addVertexBatch")         return ExecuteAddVertexBatch(args);

    return ScriptMethodResult::Error("Unknown method: " + methodName);
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult RendererScriptInterface::ExecuteSetModelConstants(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 10, "setModelConstants");
    if (!result.success) return result;

    try
    {
        // Extract position (x, y, z)
        Vec3 position = ScriptTypeExtractor::ExtractVec3(args, 0);

        // Extract orientation (yaw, pitch, roll in degrees)
        float yaw   = ScriptTypeExtractor::ExtractFloat(args[3]);
        float pitch = ScriptTypeExtractor::ExtractFloat(args[4]);
        float roll  = ScriptTypeExtractor::ExtractFloat(args[5]);

        // Extract color (r, g, b, a)
        unsigned char r = static_cast<unsigned char>(ScriptTypeExtractor::ExtractInt(args[6]));
        unsigned char g = static_cast<unsigned char>(ScriptTypeExtractor::ExtractInt(args[7]));
        unsigned char b = static_cast<unsigned char>(ScriptTypeExtractor::ExtractInt(args[8]));
        unsigned char a = static_cast<unsigned char>(ScriptTypeExtractor::ExtractInt(args[9]));
        Rgba8 color(r, g, b, a);

        // Build transform matrix exactly like Entity::GetModelToWorldTransform()
        EulerAngles orientation(yaw, pitch, roll);
        Mat44 transform;
        transform.SetTranslation3D(position);
        transform.Append(orientation.GetAsMatrix_IFwd_JLeft_KUp());

        m_renderer->SetModelConstants(transform, color);
        return ScriptMethodResult::Success();
    }
    catch (std::exception const& e)
    {
        DAEMON_LOG(LogRenderer, eLogVerbosity::Error,
            Stringf("RendererScriptInterface::setModelConstants ERROR: %s", e.what()));
        return ScriptMethodResult::Error("SetModelConstants failed: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult RendererScriptInterface::ExecuteSetBlendMode(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 1, "setBlendMode");
    if (!result.success) return result;

    try
    {
        String modeStr = ScriptTypeExtractor::ExtractString(args[0]);
        int mode = StringToBlendMode(modeStr);
        m_renderer->SetBlendMode(static_cast<eBlendMode>(mode));
        return ScriptMethodResult::Success();
    }
    catch (std::exception const& e)
    {
        return ScriptMethodResult::Error("SetBlendMode failed: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult RendererScriptInterface::ExecuteSetRasterizerMode(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 1, "setRasterizerMode");
    if (!result.success) return result;

    try
    {
        String modeStr = ScriptTypeExtractor::ExtractString(args[0]);
        int mode = StringToRasterizerMode(modeStr);
        m_renderer->SetRasterizerMode(static_cast<eRasterizerMode>(mode));
        return ScriptMethodResult::Success();
    }
    catch (std::exception const& e)
    {
        return ScriptMethodResult::Error("SetRasterizerMode failed: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult RendererScriptInterface::ExecuteSetSamplerMode(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 1, "setSamplerMode");
    if (!result.success) return result;

    try
    {
        String modeStr = ScriptTypeExtractor::ExtractString(args[0]);
        int mode = StringToSamplerMode(modeStr);
        m_renderer->SetSamplerMode(static_cast<eSamplerMode>(mode));
        return ScriptMethodResult::Success();
    }
    catch (std::exception const& e)
    {
        return ScriptMethodResult::Error("SetSamplerMode failed: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult RendererScriptInterface::ExecuteSetDepthMode(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 1, "setDepthMode");
    if (!result.success) return result;

    try
    {
        String modeStr = ScriptTypeExtractor::ExtractString(args[0]);
        int mode = StringToDepthMode(modeStr);
        m_renderer->SetDepthMode(static_cast<eDepthMode>(mode));
        return ScriptMethodResult::Success();
    }
    catch (std::exception const& e)
    {
        return ScriptMethodResult::Error("SetDepthMode failed: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult RendererScriptInterface::ExecuteBindTexture(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 1, "bindTextureCPP");
    if (!result.success) return result;

    try
    {
        String texturePath = ScriptTypeExtractor::ExtractString(args[0]);

        if (texturePath == "null" || texturePath.empty())
        {
            m_renderer->BindTexture(nullptr);
        }
        else
        {
            Texture const* texture = g_resourceSubsystem->CreateOrGetTextureFromFile(texturePath);
            m_renderer->BindTexture(texture);
        }

        return ScriptMethodResult::Success();
    }
    catch (std::exception const& e)
    {
        return ScriptMethodResult::Error("BindTexture failed: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult RendererScriptInterface::ExecuteBindShader(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 1, "bindShader");
    if (!result.success) return result;

    try
    {
        String shaderPath = ScriptTypeExtractor::ExtractString(args[0]);
        Shader* shader = m_renderer->CreateOrGetShaderFromFile(shaderPath.c_str(), eVertexType::VERTEX_PCU);
        m_renderer->BindShader(shader);
        return ScriptMethodResult::Success();
    }
    catch (std::exception const& e)
    {
        return ScriptMethodResult::Error("BindShader failed: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult RendererScriptInterface::ExecuteDrawVertexArray(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 1, "drawVertexArray");
    if (!result.success) return result;

    try
    {
        String handle = ScriptTypeExtractor::ExtractString(args[0]);

        auto it = m_vertexArrays.find(handle);
        if (it == m_vertexArrays.end())
        {
            DAEMON_LOG(LogRenderer, eLogVerbosity::Error,
                Stringf("RendererScriptInterface::drawVertexArray ERROR: Vertex array not found: %s", handle.c_str()));
            return ScriptMethodResult::Error("Vertex array not found: " + handle);
        }

        std::vector<Vertex_PCU> const& vertices = it->second;

        m_renderer->DrawVertexArray(static_cast<int>(vertices.size()), vertices.data());

        return ScriptMethodResult::Success();
    }
    catch (std::exception const& e)
    {
        DAEMON_LOG(LogRenderer, eLogVerbosity::Error,
            Stringf("RendererScriptInterface::drawVertexArray ERROR: %s", e.what()));
        return ScriptMethodResult::Error("DrawVertexArray failed: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult RendererScriptInterface::ExecuteCreateVertexArray(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 0, "createVertexArrayCPP");
    if (!result.success) return result;

    try
    {
        String handle = "vertexArray_" + std::to_string(m_nextVertexArrayId++);
        m_vertexArrays[handle] = std::vector<Vertex_PCU>();
        m_currentVertexArrayHandle = handle;

        return ScriptMethodResult::Success(handle);
    }
    catch (std::exception const& e)
    {
        return ScriptMethodResult::Error("CreateVertexArray failed: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult RendererScriptInterface::ExecuteAddVertex(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 9, "addVertex");
    if (!result.success) return result;

    try
    {
        if (m_currentVertexArrayHandle.empty())
        {
            return ScriptMethodResult::Error("No vertex array created. Call createVertexArray() first.");
        }

        float x = static_cast<float>(ScriptTypeExtractor::ExtractDouble(args[0]));
        float y = static_cast<float>(ScriptTypeExtractor::ExtractDouble(args[1]));
        float z = static_cast<float>(ScriptTypeExtractor::ExtractDouble(args[2]));
        unsigned char r = static_cast<unsigned char>(ScriptTypeExtractor::ExtractInt(args[3]));
        unsigned char g = static_cast<unsigned char>(ScriptTypeExtractor::ExtractInt(args[4]));
        unsigned char b = static_cast<unsigned char>(ScriptTypeExtractor::ExtractInt(args[5]));
        unsigned char a = static_cast<unsigned char>(ScriptTypeExtractor::ExtractInt(args[6]));
        float u = static_cast<float>(ScriptTypeExtractor::ExtractDouble(args[7]));
        float v = static_cast<float>(ScriptTypeExtractor::ExtractDouble(args[8]));

        Vertex_PCU vertex;
        vertex.m_position = Vec3(x, y, z);
        vertex.m_color = Rgba8(r, g, b, a);
        vertex.m_uvTexCoords = Vec2(u, v);

        m_vertexArrays[m_currentVertexArrayHandle].push_back(vertex);

        return ScriptMethodResult::Success();
    }
    catch (std::exception const& e)
    {
        return ScriptMethodResult::Error("AddVertex failed: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
// Enum conversion helpers
//----------------------------------------------------------------------------------------------------
int RendererScriptInterface::StringToBlendMode(String const& modeStr) const
{
    if (modeStr == "OPAQUE" || modeStr == "opaque") return static_cast<int>(eBlendMode::OPAQUE);
    if (modeStr == "ALPHA" || modeStr == "alpha") return static_cast<int>(eBlendMode::ALPHA);
    if (modeStr == "ADDITIVE" || modeStr == "additive") return static_cast<int>(eBlendMode::ADDITIVE);

    DAEMON_LOG(LogScript, eLogVerbosity::Warning, StringFormat("Unknown blend mode: {}, defaulting to OPAQUE", modeStr));
    return static_cast<int>(eBlendMode::OPAQUE);
}

//----------------------------------------------------------------------------------------------------
int RendererScriptInterface::StringToRasterizerMode(String const& modeStr) const
{
    if (modeStr == "SOLID_CULL_BACK" || modeStr == "solid_cull_back") return static_cast<int>(eRasterizerMode::SOLID_CULL_BACK);
    if (modeStr == "SOLID_CULL_NONE" || modeStr == "solid_cull_none") return static_cast<int>(eRasterizerMode::SOLID_CULL_NONE);
    if (modeStr == "WIREFRAME_CULL_BACK" || modeStr == "wireframe_cull_back") return static_cast<int>(eRasterizerMode::WIREFRAME_CULL_BACK);
    if (modeStr == "WIREFRAME_CULL_NONE" || modeStr == "wireframe_cull_none") return static_cast<int>(eRasterizerMode::WIREFRAME_CULL_NONE);

    DAEMON_LOG(LogScript, eLogVerbosity::Warning, StringFormat("Unknown rasterizer mode: {}, defaulting to SOLID_CULL_BACK", modeStr));
    return static_cast<int>(eRasterizerMode::SOLID_CULL_BACK);
}

//----------------------------------------------------------------------------------------------------
int RendererScriptInterface::StringToSamplerMode(String const& modeStr) const
{
    if (modeStr == "POINT_CLAMP" || modeStr == "point_clamp") return static_cast<int>(eSamplerMode::POINT_CLAMP);
    if (modeStr == "BILINEAR_CLAMP" || modeStr == "bilinear_clamp") return static_cast<int>(eSamplerMode::BILINEAR_CLAMP);

    DAEMON_LOG(LogScript, eLogVerbosity::Warning, StringFormat("Unknown sampler mode: {}, defaulting to POINT_CLAMP", modeStr));
    return static_cast<int>(eSamplerMode::POINT_CLAMP);
}

//----------------------------------------------------------------------------------------------------
int RendererScriptInterface::StringToDepthMode(String const& modeStr) const
{
    if (modeStr == "DISABLED" || modeStr == "disabled") return static_cast<int>(eDepthMode::DISABLED);
    if (modeStr == "READ_ONLY_ALWAYS" || modeStr == "read_only_always") return static_cast<int>(eDepthMode::READ_ONLY_ALWAYS);
    if (modeStr == "READ_ONLY_LESS_EQUAL" || modeStr == "read_only_less_equal") return static_cast<int>(eDepthMode::READ_ONLY_LESS_EQUAL);
    if (modeStr == "READ_WRITE_LESS_EQUAL" || modeStr == "read_write_less_equal") return static_cast<int>(eDepthMode::READ_WRITE_LESS_EQUAL);

    DAEMON_LOG(LogScript, eLogVerbosity::Warning, StringFormat("Unknown depth mode: {}, defaulting to READ_WRITE_LESS_EQUAL", modeStr));
    return static_cast<int>(eDepthMode::READ_WRITE_LESS_EQUAL);
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult RendererScriptInterface::ExecuteAddVertexBatch(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 1, "addVertexBatch");
    if (!result.success) return result;

    try
    {
        if (m_currentVertexArrayHandle.empty())
        {
            return ScriptMethodResult::Error("No vertex array created. Call createVertexArray() first.");
        }

        // Expect a single argument containing an array of vertex data
        // Format: [x,y,z,r,g,b,a,u,v, x,y,z,r,g,b,a,u,v, ...]
        if (args[0].type() != typeid(std::vector<std::any>))
        {
            return ScriptMethodResult::Error("addVertexBatch expects an array argument");
        }

        std::vector<std::any> const& vertexData = std::any_cast<std::vector<std::any> const&>(args[0]);

        // Each vertex needs 9 values (x,y,z, r,g,b,a, u,v)
        if (vertexData.size() % 9 != 0)
        {
            return ScriptMethodResult::Error(
                "addVertexBatch: array size must be multiple of 9 (got " +
                std::to_string(vertexData.size()) + ")");
        }

        size_t numVertices = vertexData.size() / 9;
        std::vector<Vertex_PCU>& vertices = m_vertexArrays[m_currentVertexArrayHandle];
        vertices.reserve(vertices.size() + numVertices);

        for (size_t i = 0; i < numVertices; ++i)
        {
            size_t baseIdx = i * 9;

            // PERFORMANCE OPTIMIZATION: Direct any_cast<double> instead of ScriptTypeExtractor
            // JavaScript always sends numbers as doubles, avoiding exception-based type checking
            float x = static_cast<float>(std::any_cast<double>(vertexData[baseIdx + 0]));
            float y = static_cast<float>(std::any_cast<double>(vertexData[baseIdx + 1]));
            float z = static_cast<float>(std::any_cast<double>(vertexData[baseIdx + 2]));
            unsigned char r = static_cast<unsigned char>(std::any_cast<double>(vertexData[baseIdx + 3]));
            unsigned char g = static_cast<unsigned char>(std::any_cast<double>(vertexData[baseIdx + 4]));
            unsigned char b = static_cast<unsigned char>(std::any_cast<double>(vertexData[baseIdx + 5]));
            unsigned char a = static_cast<unsigned char>(std::any_cast<double>(vertexData[baseIdx + 6]));
            float u = static_cast<float>(std::any_cast<double>(vertexData[baseIdx + 7]));
            float v = static_cast<float>(std::any_cast<double>(vertexData[baseIdx + 8]));

            // PERFORMANCE OPTIMIZATION: emplace_back for in-place construction
            vertices.emplace_back(Vec3(x, y, z), Rgba8(r, g, b, a), Vec2(u, v));
        }

        return ScriptMethodResult::Success();
    }
    catch (std::bad_any_cast const& e)
    {
        return ScriptMethodResult::Error(
            "AddVertexBatch type error: Expected double values from JavaScript. " + String(e.what()));
    }
    catch (std::exception const& e)
    {
        return ScriptMethodResult::Error("AddVertexBatch failed: " + String(e.what()));
    }
}
