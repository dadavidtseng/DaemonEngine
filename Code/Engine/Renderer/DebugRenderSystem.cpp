//----------------------------------------------------------------------------------------------------
// DebugRenderSystem.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Renderer/DebugRenderSystem.hpp"

#include <mutex>

#include "Engine/Core/Clock.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/Renderer.hpp"

//----------------------------------------------------------------------------------------------------
static DebugRenderConfig m_debugRenderConfig;
static BitmapFont*       m_debugRenderBitmapFont = nullptr;
static bool              m_debugRenderIsVisible  = true;
std::mutex               m_mutex;


//----------------------------------------------------------------------------------------------------
enum class DebugRenderObjectType
{
    WORLD_POINT,
    WORLD_LINE,
    WORLD_WIRE_CYLINDER,
    WORLD_WIRE_SPHERE,
    WORLD_ARROW,
    WORLD_TEXT,
    WORLD_BILLBOARD_TEXT,
    SCREEN_TEXT,
    MESSAGE,
    COUNT
};

//----------------------------------------------------------------------------------------------------
struct DebugRenderObject
{
    DebugRenderObjectType m_type;
    VertexList            m_vertices;
    Vec3                  m_startPosition;
    Vec3                  m_endPosition;
    float                 m_radius;
    float                 m_elapsedTime = 0.f;
    float                 m_maxElapsedTime;
    Rgba8                 m_startColor;
    Rgba8                 m_endColor;
    String                m_text;
    float                 m_textHeight;
    Vec2                  m_alignment;
    Mat44                 m_m2wTransform;
    DebugRenderMode       m_mode;
    bool                  m_isWireFrame = false;
};

//----------------------------------------------------------------------------------------------------
struct DebugRenderScreenMessage
{
    String m_text;
    float  m_elapsedTime = 0.f;
    float  m_maxElapsedTime;
    Rgba8  m_startColor;
    Rgba8  m_endColor;
};

//----------------------------------------------------------------------------------------------------
static std::vector<DebugRenderObject*>        m_debugRenderObjectList;
static std::vector<DebugRenderScreenMessage*> m_debugRenderScreenMessageList;

//----------------------------------------------------------------------------------------------------
void DebugRenderSystemStartup(DebugRenderConfig const& config)
{
    m_debugRenderConfig.m_renderer = config.m_renderer;
    m_debugRenderConfig.m_fontName = config.m_fontName;
    m_debugRenderBitmapFont        = config.m_renderer->CreateOrGetBitmapFontFromFile(("Data/Fonts/" + config.m_fontName).c_str());
    g_theEventSystem->SubscribeEventCallbackFunction("Command_DebugRenderClear", Command_DebugRenderClear);
    g_theEventSystem->SubscribeEventCallbackFunction("Command_DebugRenderToggle", Command_DebugRenderToggle);
}

//----------------------------------------------------------------------------------------------------
void DebugRenderSystemShutdown()
{
    DebugRenderClear();
}

//----------------------------------------------------------------------------------------------------
void DebugRenderSetVisible()
{
    m_mutex.lock();
    m_debugRenderIsVisible = true;
    m_mutex.unlock();
}

//----------------------------------------------------------------------------------------------------
void DebugRenderSetHidden()
{
    m_mutex.lock();
    m_debugRenderIsVisible = false;
    m_mutex.unlock();
}

//----------------------------------------------------------------------------------------------------
void DebugRenderClear()
{
    m_mutex.lock();

    for (DebugRenderObject const* object : m_debugRenderObjectList)
    {
        delete object;
        object = nullptr;
    }

    for (DebugRenderScreenMessage const* message : m_debugRenderScreenMessageList)
    {
        delete message;
        message = nullptr;
    }

    m_mutex.unlock();
}

//----------------------------------------------------------------------------------------------------
void DebugRenderBeginFrame()
{
    float const deltaSeconds = static_cast<float>(Clock::GetSystemClock().GetDeltaSeconds());

    for (std::vector<DebugRenderObject*>::iterator it = m_debugRenderObjectList.begin(); it != m_debugRenderObjectList.end();)
    {
        DebugRenderObject* object = *it;
        if (object != nullptr)
        {
            object->m_elapsedTime += deltaSeconds;

            if (object->m_elapsedTime >= object->m_maxElapsedTime && object->m_maxElapsedTime > -1.f)
            {
                it = m_debugRenderObjectList.erase(it);  // 安全移除元素，返回下一個有效迭代器
                continue;  // 防止 `it++` 跳過元素
            }
        }
        ++it;  // 只有當元素未被移除時才前進
    }


    for (std::vector<DebugRenderScreenMessage*>::iterator it = m_debugRenderScreenMessageList.begin(); it != m_debugRenderScreenMessageList.end();)
    {
        DebugRenderScreenMessage* object = *it;
        if (object != nullptr)
        {
            object->m_elapsedTime += deltaSeconds;

            if (object->m_elapsedTime >= object->m_maxElapsedTime && object->m_maxElapsedTime > -1.f)
            {
                it = m_debugRenderScreenMessageList.erase(it);  // 安全移除元素，返回下一個有效迭代器
                continue;  // 防止 `it++` 跳過元素
            }
        }
        ++it;  // 只有當元素未被移除時才前進
    }
}

//----------------------------------------------------------------------------------------------------
Rgba8 const DebugRenderGetDebugObjectCurrentColor(DebugRenderObject const* object)
{
    if (object->m_maxElapsedTime <= 0.f)
    {
        return object->m_startColor;
    }

    Rgba8 currentColor = Interpolate(object->m_startColor, object->m_endColor, object->m_elapsedTime / object->m_maxElapsedTime);

    if (object->m_mode == DebugRenderMode::X_RAY)
    {
        currentColor.r = static_cast<unsigned char>(GetClamped(currentColor.r + 50, 0, 255));
        currentColor.g = static_cast<unsigned char>(GetClamped(currentColor.g + 50, 0, 255));
        currentColor.b = static_cast<unsigned char>(GetClamped(currentColor.b + 50, 0, 255));
        currentColor.a = static_cast<unsigned char>(GetClamped(currentColor.a - 100, 0, 255));
    }

    return currentColor;
}

//----------------------------------------------------------------------------------------------------
Rgba8 const DebugRenderGetDebugScreenMessageCurrentColor(DebugRenderScreenMessage const* message)
{
    if (message->m_maxElapsedTime <= 0.f)
    {
        return message->m_startColor;
    }

    return Interpolate(message->m_startColor, message->m_endColor, message->m_elapsedTime / message->m_maxElapsedTime);
}

//----------------------------------------------------------------------------------------------------
void DebugRenderWorld(Camera const& camera)
{
    m_mutex.lock();
    if (m_debugRenderIsVisible == false)
    {
        m_mutex.unlock();
        return;
    }

    m_debugRenderConfig.m_renderer->BeginCamera(camera);
    m_debugRenderConfig.m_renderer->SetSamplerMode(SamplerMode::POINT_CLAMP);

    for (DebugRenderObject const* object : m_debugRenderObjectList)
    {
        if (object == nullptr)
        {
            continue;
        }
        else if (object->m_type == DebugRenderObjectType::WORLD_POINT)
        {
            m_debugRenderConfig.m_renderer->BindTexture(nullptr);
            m_debugRenderConfig.m_renderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
            if (object->m_mode == DebugRenderMode::ALWAYS)
            {
                m_debugRenderConfig.m_renderer->SetModelConstants(Mat44(), DebugRenderGetDebugObjectCurrentColor(object));
                m_debugRenderConfig.m_renderer->SetBlendMode(BlendMode::OPAQUE);
                m_debugRenderConfig.m_renderer->SetDepthMode(DepthMode::DISABLED);
                m_debugRenderConfig.m_renderer->DrawVertexArray(static_cast<int>(object->m_vertices.size()), object->m_vertices.data());
            }
            else if (object->m_mode == DebugRenderMode::USE_DEPTH)
            {
                m_debugRenderConfig.m_renderer->SetModelConstants(Mat44(), DebugRenderGetDebugObjectCurrentColor(object));
                m_debugRenderConfig.m_renderer->SetBlendMode(BlendMode::OPAQUE);
                m_debugRenderConfig.m_renderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
                m_debugRenderConfig.m_renderer->DrawVertexArray(static_cast<int>(object->m_vertices.size()), object->m_vertices.data());
            }
        }
        else if (object->m_type == DebugRenderObjectType::WORLD_LINE)
        {
            m_debugRenderConfig.m_renderer->BindTexture(nullptr);
            m_debugRenderConfig.m_renderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
            if (object->m_mode == DebugRenderMode::ALWAYS)
            {
                m_debugRenderConfig.m_renderer->SetModelConstants(Mat44(), DebugRenderGetDebugObjectCurrentColor(object));
                m_debugRenderConfig.m_renderer->SetBlendMode(BlendMode::OPAQUE);
                m_debugRenderConfig.m_renderer->SetDepthMode(DepthMode::DISABLED);
                m_debugRenderConfig.m_renderer->DrawVertexArray(static_cast<int>(object->m_vertices.size()), object->m_vertices.data());
            }
            else if (object->m_mode == DebugRenderMode::USE_DEPTH)
            {
                m_debugRenderConfig.m_renderer->SetModelConstants(Mat44(), DebugRenderGetDebugObjectCurrentColor(object));
                m_debugRenderConfig.m_renderer->SetBlendMode(BlendMode::OPAQUE);
                m_debugRenderConfig.m_renderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
                m_debugRenderConfig.m_renderer->DrawVertexArray(static_cast<int>(object->m_vertices.size()), object->m_vertices.data());
            }
        }
        else if (object->m_type == DebugRenderObjectType::WORLD_WIRE_CYLINDER)
        {
            m_debugRenderConfig.m_renderer->BindTexture(nullptr);
            m_debugRenderConfig.m_renderer->SetModelConstants(Mat44(), DebugRenderGetDebugObjectCurrentColor(object));
            if (object->m_isWireFrame)
            {
                m_debugRenderConfig.m_renderer->SetRasterizerMode(RasterizerMode::WIREFRAME_CULL_BACK);
            }
            else
            {
                m_debugRenderConfig.m_renderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
            }
            if (object->m_mode == DebugRenderMode::ALWAYS)
            {
                m_debugRenderConfig.m_renderer->SetBlendMode(BlendMode::OPAQUE);
                m_debugRenderConfig.m_renderer->SetDepthMode(DepthMode::DISABLED);
            }
            else if (object->m_mode == DebugRenderMode::USE_DEPTH)
            {
                m_debugRenderConfig.m_renderer->SetBlendMode(BlendMode::OPAQUE);
                m_debugRenderConfig.m_renderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
            }
            m_debugRenderConfig.m_renderer->DrawVertexArray(static_cast<int>(object->m_vertices.size()), object->m_vertices.data());
        }
        else if (object->m_type == DebugRenderObjectType::WORLD_WIRE_SPHERE)
        {
            m_debugRenderConfig.m_renderer->BindTexture(nullptr);
            m_debugRenderConfig.m_renderer->SetModelConstants(object->m_m2wTransform, DebugRenderGetDebugObjectCurrentColor(object));
            if (object->m_isWireFrame)
            {
                m_debugRenderConfig.m_renderer->SetRasterizerMode(RasterizerMode::WIREFRAME_CULL_BACK);
            }
            else
            {
                m_debugRenderConfig.m_renderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
            }
            if (object->m_mode == DebugRenderMode::ALWAYS)
            {
                m_debugRenderConfig.m_renderer->SetBlendMode(BlendMode::OPAQUE);
                m_debugRenderConfig.m_renderer->SetDepthMode(DepthMode::DISABLED);
            }
            else if (object->m_mode == DebugRenderMode::USE_DEPTH)
            {
                m_debugRenderConfig.m_renderer->SetBlendMode(BlendMode::OPAQUE);
                m_debugRenderConfig.m_renderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
            }
            m_debugRenderConfig.m_renderer->DrawVertexArray(static_cast<int>(object->m_vertices.size()), object->m_vertices.data());
        }
        else if (object->m_type == DebugRenderObjectType::WORLD_ARROW)
        {
            m_debugRenderConfig.m_renderer->BindTexture(nullptr);
            m_debugRenderConfig.m_renderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
            if (object->m_mode == DebugRenderMode::ALWAYS)
            {
                m_debugRenderConfig.m_renderer->SetModelConstants(Mat44(), DebugRenderGetDebugObjectCurrentColor(object));
                m_debugRenderConfig.m_renderer->SetBlendMode(BlendMode::OPAQUE);
                m_debugRenderConfig.m_renderer->SetDepthMode(DepthMode::DISABLED);
                m_debugRenderConfig.m_renderer->DrawVertexArray(static_cast<int>(object->m_vertices.size()), object->m_vertices.data());
            }
            else if (object->m_mode == DebugRenderMode::USE_DEPTH)
            {
                m_debugRenderConfig.m_renderer->SetModelConstants(Mat44(), DebugRenderGetDebugObjectCurrentColor(object));
                m_debugRenderConfig.m_renderer->SetBlendMode(BlendMode::OPAQUE);
                m_debugRenderConfig.m_renderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
                m_debugRenderConfig.m_renderer->DrawVertexArray(static_cast<int>(object->m_vertices.size()), object->m_vertices.data());
            }
        }
        else if (object->m_type == DebugRenderObjectType::WORLD_TEXT)
        {
            m_debugRenderConfig.m_renderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
            m_debugRenderConfig.m_renderer->BindTexture(&m_debugRenderBitmapFont->GetTexture());
            if (object->m_mode == DebugRenderMode::ALWAYS)
            {
                m_debugRenderConfig.m_renderer->SetModelConstants(object->m_m2wTransform, DebugRenderGetDebugObjectCurrentColor(object));
                m_debugRenderConfig.m_renderer->SetBlendMode(BlendMode::ALPHA);
                m_debugRenderConfig.m_renderer->SetDepthMode(DepthMode::DISABLED);
                m_debugRenderConfig.m_renderer->DrawVertexArray(static_cast<int>(object->m_vertices.size()), object->m_vertices.data());
            }
            else if (object->m_mode == DebugRenderMode::USE_DEPTH)
            {
                m_debugRenderConfig.m_renderer->SetModelConstants(object->m_m2wTransform, DebugRenderGetDebugObjectCurrentColor(object));
                m_debugRenderConfig.m_renderer->SetBlendMode(BlendMode::ALPHA);
                m_debugRenderConfig.m_renderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
                m_debugRenderConfig.m_renderer->DrawVertexArray(static_cast<int>(object->m_vertices.size()), object->m_vertices.data());
            }
        }
        else if (object->m_type == DebugRenderObjectType::WORLD_BILLBOARD_TEXT)
        {
            m_debugRenderConfig.m_renderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
            m_debugRenderConfig.m_renderer->BindTexture(&m_debugRenderBitmapFont->GetTexture());
            if (object->m_mode == DebugRenderMode::ALWAYS)
            {
                m_debugRenderConfig.m_renderer->SetModelConstants(GetBillboardMatrix(eBillboardType::FULL_OPPOSING, camera.GetCameraToWorldTransform(), object->m_startPosition), DebugRenderGetDebugObjectCurrentColor(object));
                m_debugRenderConfig.m_renderer->SetBlendMode(BlendMode::ALPHA);
                m_debugRenderConfig.m_renderer->SetDepthMode(DepthMode::DISABLED);
                m_debugRenderConfig.m_renderer->DrawVertexArray(static_cast<int>(object->m_vertices.size()), object->m_vertices.data());
            }
            else if (object->m_mode == DebugRenderMode::USE_DEPTH)
            {
                m_debugRenderConfig.m_renderer->SetModelConstants(GetBillboardMatrix(eBillboardType::FULL_OPPOSING, camera.GetCameraToWorldTransform(), object->m_startPosition), DebugRenderGetDebugObjectCurrentColor(object));
                m_debugRenderConfig.m_renderer->SetBlendMode(BlendMode::ALPHA);
                m_debugRenderConfig.m_renderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
                m_debugRenderConfig.m_renderer->DrawVertexArray(static_cast<int>(object->m_vertices.size()), object->m_vertices.data());
            }
        }
    }

    for (DebugRenderObject* object : m_debugRenderObjectList)
    {
        if (object == nullptr)
        {
            continue;
        }
        else if (object->m_type == DebugRenderObjectType::WORLD_POINT)
        {
            m_debugRenderConfig.m_renderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
            m_debugRenderConfig.m_renderer->BindTexture(nullptr);
            if (object->m_mode == DebugRenderMode::X_RAY)
            {
                m_debugRenderConfig.m_renderer->SetModelConstants(Mat44(), DebugRenderGetDebugObjectCurrentColor(object));
                m_debugRenderConfig.m_renderer->SetBlendMode(BlendMode::ALPHA);
                m_debugRenderConfig.m_renderer->SetDepthMode(DepthMode::DISABLED);
                m_debugRenderConfig.m_renderer->DrawVertexArray(static_cast<int>(object->m_vertices.size()), object->m_vertices.data());
                m_debugRenderConfig.m_renderer->SetModelConstants(Mat44(), DebugRenderGetDebugObjectCurrentColor(object));
                m_debugRenderConfig.m_renderer->SetBlendMode(BlendMode::OPAQUE);
                m_debugRenderConfig.m_renderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
                m_debugRenderConfig.m_renderer->DrawVertexArray(static_cast<int>(object->m_vertices.size()), object->m_vertices.data());
            }
        }
        else if (object->m_type == DebugRenderObjectType::WORLD_LINE)
        {
            m_debugRenderConfig.m_renderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
            m_debugRenderConfig.m_renderer->BindTexture(nullptr);
            if (object->m_mode == DebugRenderMode::X_RAY)
            {
                m_debugRenderConfig.m_renderer->SetModelConstants(Mat44(), DebugRenderGetDebugObjectCurrentColor(object));
                m_debugRenderConfig.m_renderer->SetBlendMode(BlendMode::ALPHA);
                m_debugRenderConfig.m_renderer->SetDepthMode(DepthMode::DISABLED);
                m_debugRenderConfig.m_renderer->DrawVertexArray(static_cast<int>(object->m_vertices.size()), object->m_vertices.data());
                m_debugRenderConfig.m_renderer->SetModelConstants(Mat44(), DebugRenderGetDebugObjectCurrentColor(object));
                m_debugRenderConfig.m_renderer->SetBlendMode(BlendMode::OPAQUE);
                m_debugRenderConfig.m_renderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
                m_debugRenderConfig.m_renderer->DrawVertexArray(static_cast<int>(object->m_vertices.size()), object->m_vertices.data());
            }
        }
        else if (object->m_type == DebugRenderObjectType::WORLD_WIRE_CYLINDER)
        {
            m_debugRenderConfig.m_renderer->BindTexture(nullptr);
            if (object->m_isWireFrame)
            {
                m_debugRenderConfig.m_renderer->SetRasterizerMode(RasterizerMode::WIREFRAME_CULL_BACK);
            }
            else
            {
                m_debugRenderConfig.m_renderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
            }
            if (object->m_mode == DebugRenderMode::X_RAY)
            {
                m_debugRenderConfig.m_renderer->SetModelConstants(Mat44(), DebugRenderGetDebugObjectCurrentColor(object));
                m_debugRenderConfig.m_renderer->SetBlendMode(BlendMode::ALPHA);
                m_debugRenderConfig.m_renderer->SetDepthMode(DepthMode::DISABLED);
                m_debugRenderConfig.m_renderer->DrawVertexArray(static_cast<int>(object->m_vertices.size()), object->m_vertices.data());
                m_debugRenderConfig.m_renderer->SetModelConstants(Mat44(), DebugRenderGetDebugObjectCurrentColor(object));
                m_debugRenderConfig.m_renderer->SetBlendMode(BlendMode::OPAQUE);
                m_debugRenderConfig.m_renderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
                m_debugRenderConfig.m_renderer->DrawVertexArray(static_cast<int>(object->m_vertices.size()), object->m_vertices.data());
            }
        }
        else if (object->m_type == DebugRenderObjectType::WORLD_WIRE_SPHERE)
        {
            m_debugRenderConfig.m_renderer->BindTexture(nullptr);
            if (object->m_isWireFrame)
            {
                m_debugRenderConfig.m_renderer->SetRasterizerMode(RasterizerMode::WIREFRAME_CULL_BACK);
            }
            else
            {
                m_debugRenderConfig.m_renderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
            }
            if (object->m_mode == DebugRenderMode::X_RAY)
            {
                m_debugRenderConfig.m_renderer->SetModelConstants(Mat44(), DebugRenderGetDebugObjectCurrentColor(object));
                m_debugRenderConfig.m_renderer->SetBlendMode(BlendMode::ALPHA);
                m_debugRenderConfig.m_renderer->SetDepthMode(DepthMode::DISABLED);
                m_debugRenderConfig.m_renderer->DrawVertexArray(static_cast<int>(object->m_vertices.size()), object->m_vertices.data());
                m_debugRenderConfig.m_renderer->SetModelConstants(Mat44(), DebugRenderGetDebugObjectCurrentColor(object));
                m_debugRenderConfig.m_renderer->SetBlendMode(BlendMode::OPAQUE);
                m_debugRenderConfig.m_renderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
                m_debugRenderConfig.m_renderer->DrawVertexArray(static_cast<int>(object->m_vertices.size()), object->m_vertices.data());
            }
        }
        else if (object->m_type == DebugRenderObjectType::WORLD_ARROW)
        {
            m_debugRenderConfig.m_renderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
            m_debugRenderConfig.m_renderer->BindTexture(nullptr);
            if (object->m_mode == DebugRenderMode::X_RAY)
            {
                m_debugRenderConfig.m_renderer->SetModelConstants(Mat44(), DebugRenderGetDebugObjectCurrentColor(object));
                m_debugRenderConfig.m_renderer->SetBlendMode(BlendMode::ALPHA);
                m_debugRenderConfig.m_renderer->SetDepthMode(DepthMode::DISABLED);
                m_debugRenderConfig.m_renderer->DrawVertexArray(static_cast<int>(object->m_vertices.size()), object->m_vertices.data());
                m_debugRenderConfig.m_renderer->SetModelConstants(Mat44(), DebugRenderGetDebugObjectCurrentColor(object));
                m_debugRenderConfig.m_renderer->SetBlendMode(BlendMode::OPAQUE);
                m_debugRenderConfig.m_renderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
                m_debugRenderConfig.m_renderer->DrawVertexArray(static_cast<int>(object->m_vertices.size()), object->m_vertices.data());
            }
        }
        else if (object->m_type == DebugRenderObjectType::WORLD_TEXT)
        {
            m_debugRenderConfig.m_renderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
            m_debugRenderConfig.m_renderer->BindTexture(&m_debugRenderBitmapFont->GetTexture());
            if (object->m_mode == DebugRenderMode::X_RAY)
            {
                m_debugRenderConfig.m_renderer->SetModelConstants(object->m_m2wTransform, DebugRenderGetDebugObjectCurrentColor(object));
                m_debugRenderConfig.m_renderer->SetBlendMode(BlendMode::ALPHA);
                m_debugRenderConfig.m_renderer->SetDepthMode(DepthMode::DISABLED);
                m_debugRenderConfig.m_renderer->DrawVertexArray(static_cast<int>(object->m_vertices.size()), object->m_vertices.data());
                m_debugRenderConfig.m_renderer->SetModelConstants(object->m_m2wTransform, DebugRenderGetDebugObjectCurrentColor(object));
                m_debugRenderConfig.m_renderer->SetBlendMode(BlendMode::OPAQUE);
                m_debugRenderConfig.m_renderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
                m_debugRenderConfig.m_renderer->DrawVertexArray(static_cast<int>(object->m_vertices.size()), object->m_vertices.data());
            }
        }
        else if (object->m_type == DebugRenderObjectType::WORLD_BILLBOARD_TEXT)
        {
            m_debugRenderConfig.m_renderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
            m_debugRenderConfig.m_renderer->BindTexture(&m_debugRenderBitmapFont->GetTexture());
            if (object->m_mode == DebugRenderMode::X_RAY)
            {
                m_debugRenderConfig.m_renderer->SetModelConstants(GetBillboardMatrix(eBillboardType::FULL_OPPOSING, camera.GetCameraToWorldTransform(), object->m_startPosition), DebugRenderGetDebugObjectCurrentColor(object));
                m_debugRenderConfig.m_renderer->SetBlendMode(BlendMode::ALPHA);
                m_debugRenderConfig.m_renderer->SetDepthMode(DepthMode::DISABLED);
                m_debugRenderConfig.m_renderer->DrawVertexArray(static_cast<int>(object->m_vertices.size()), object->m_vertices.data());
                m_debugRenderConfig.m_renderer->SetModelConstants(GetBillboardMatrix(eBillboardType::FULL_OPPOSING, camera.GetCameraToWorldTransform(), object->m_startPosition), DebugRenderGetDebugObjectCurrentColor(object));
                m_debugRenderConfig.m_renderer->SetBlendMode(BlendMode::OPAQUE);
                m_debugRenderConfig.m_renderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
                m_debugRenderConfig.m_renderer->DrawVertexArray(static_cast<int>(object->m_vertices.size()), object->m_vertices.data());
            }
        }
    }
    m_debugRenderConfig.m_renderer->EndCamera(camera);
    m_mutex.unlock();
}

//----------------------------------------------------------------------------------------------------
void DebugRenderScreen(Camera const& camera)
{
    m_mutex.lock();
    if (m_debugRenderIsVisible == false)
    {
        m_mutex.unlock();
        return;
    }
    m_debugRenderConfig.m_renderer->BeginCamera(camera);
    std::vector<Vertex_PCU> verts;
    verts.reserve(1000);
    m_debugRenderConfig.m_renderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
    for (DebugRenderObject* object : m_debugRenderObjectList)
    {
        if (object == nullptr)
        {
            continue;
        }
        else if (object->m_type == DebugRenderObjectType::SCREEN_TEXT)
        {
            verts.clear();
            m_debugRenderBitmapFont->AddVertsForTextInBox2D(verts, object->m_text, AABB2(Vec2(object->m_startPosition.x, object->m_startPosition.y), Vec2(object->m_startPosition.x, object->m_startPosition.y) + Vec2((float)object->m_text.size() * object->m_textHeight * 0.618f, object->m_textHeight)),
                                                            object->m_textHeight, DebugRenderGetDebugObjectCurrentColor(object), 0.618f, object->m_alignment, OVERRUN);
            m_debugRenderConfig.m_renderer->SetBlendMode(BlendMode::ALPHA);
            m_debugRenderConfig.m_renderer->BindTexture(&m_debugRenderBitmapFont->GetTexture());
            m_debugRenderConfig.m_renderer->SetModelConstants();
            m_debugRenderConfig.m_renderer->DrawVertexArray(static_cast<int>(verts.size()), verts.data());
        }
    }

    float lineHeight = (camera.GetOrthographicTopRight().y - camera.GetOrthographicBottomLeft().y) / 40.f;
    float curHeight  = camera.GetOrthographicTopRight().y - lineHeight;

    for (DebugRenderScreenMessage* message : m_debugRenderScreenMessageList)
    {
        if (message == nullptr)
        {
            continue;
        }

        if (message->m_maxElapsedTime == -1.f)
        {
            verts.clear();
            m_debugRenderBitmapFont->AddVertsForText2D(verts, message->m_text, Vec2(0.f, curHeight), lineHeight, DebugRenderGetDebugScreenMessageCurrentColor(message), 0.618f);
            m_debugRenderConfig.m_renderer->SetBlendMode(BlendMode::ALPHA);
            m_debugRenderConfig.m_renderer->BindTexture(&m_debugRenderBitmapFont->GetTexture());
            m_debugRenderConfig.m_renderer->SetModelConstants();
            m_debugRenderConfig.m_renderer->DrawVertexArray(static_cast<int>(verts.size()), verts.data());
            curHeight -= lineHeight;
        }
    }
    for (DebugRenderScreenMessage* message : m_debugRenderScreenMessageList)
    {
        if (message == nullptr)
        {
            continue;
        }
        if (message->m_maxElapsedTime != -1.f)
        {
            verts.clear();
            m_debugRenderBitmapFont->AddVertsForText2D(verts, message->m_text, Vec2(0.f, curHeight), lineHeight, DebugRenderGetDebugScreenMessageCurrentColor(message), 0.618f);
            m_debugRenderConfig.m_renderer->SetBlendMode(BlendMode::ALPHA);
            m_debugRenderConfig.m_renderer->BindTexture(&m_debugRenderBitmapFont->GetTexture());
            m_debugRenderConfig.m_renderer->SetModelConstants();
            m_debugRenderConfig.m_renderer->DrawVertexArray(static_cast<int>(verts.size()), verts.data());
            curHeight -= lineHeight;
        }
    }
    m_debugRenderConfig.m_renderer->EndCamera(camera);
    m_mutex.unlock();
}

//----------------------------------------------------------------------------------------------------
void DebugRenderEndFrame()
{
}

//----------------------------------------------------------------------------------------------------
void DebugRenderAddObjectToList(DebugRenderObject* objectToAdd)
{
    for (DebugRenderObject const* object : m_debugRenderObjectList)
    {
        if (object == nullptr)
        {
            return;
        }
    }

    m_debugRenderObjectList.push_back(objectToAdd);
}

//----------------------------------------------------------------------------------------------------
void DebugAddWorldPoint(Vec3 const& pos, float radius, float duration, Rgba8 const& startColor, Rgba8 const& endColor, DebugRenderMode mode)
{
}


//----------------------------------------------------------------------------------------------------
void DebugAddWorldLine(Vec3 const& start, Vec3 const& end, float radius, float duration, Rgba8 const& startColor, Rgba8 const& endColor, DebugRenderMode mode)
{
    DebugRenderObject* object = new DebugRenderObject;
    object->m_type            = DebugRenderObjectType::WORLD_LINE;
    object->m_startPosition   = start;
    object->m_endPosition     = end;
    object->m_maxElapsedTime  = duration;
    object->m_radius          = radius;
    object->m_startColor      = startColor;
    object->m_endColor        = endColor;
    object->m_mode            = mode;
    AddVertsForCylinder3D(object->m_vertices, start, end, radius);
    m_mutex.lock();
    DebugRenderAddObjectToList(object);
    m_mutex.unlock();
}

void DebugAddWorldWireCylinder(Vec3 const& base, Vec3 const& top, float radius, float duration, Rgba8 const& startColor, Rgba8 const& endColor, DebugRenderMode mode)
{
}

void DebugAddWorldWireSphere(Vec3 const& center, float radius, float duration, Rgba8 const& startColor, Rgba8 const& endColor, DebugRenderMode mode)
{
    DebugRenderObject* object = new DebugRenderObject;
    object->m_type            = DebugRenderObjectType::WORLD_WIRE_SPHERE;
    object->m_startPosition   = center;
    object->m_maxElapsedTime  = duration;
    object->m_radius          = radius;
    object->m_startColor      = startColor;
    object->m_endColor        = endColor;
    object->m_isWireFrame     = true;
    object->m_mode            = mode;
    object->m_m2wTransform.SetTranslation3D(object->m_startPosition);
    AddVertsForSphere3D(object->m_vertices, radius, object->m_startColor);
    m_mutex.lock();
    DebugRenderAddObjectToList(object);
    m_mutex.unlock();
}

void DebugAddWorldArrow(Vec3 const& start, Vec3 const& end, float radius, float duration, Rgba8 const& startColor, Rgba8 const& endColor, DebugRenderMode mode)
{
}

void DebugAddWorldText(String const& text, Mat44 const& transform, float textHeight, Vec2 const& alignment, float duration, Rgba8 const& startColor, Rgba8 const& endColor, DebugRenderMode mode)
{
}

void DebugAddBillboardText(String const& text, Vec3 const& origin, float textHeight, Vec2 const& alignment, float duration, Rgba8 const& startColor, Rgba8 const& endColor, DebugRenderMode mode)
{
}

void DebugAddWorldBasis(Mat44 const& transform, float duration, DebugRenderMode mode)
{
}

void DebugAddScreenText(String const& text, Vec2 const& position, float size, Vec2 const& alignment, float duration, Rgba8 const& startColor, Rgba8 const& endColor, DebugRenderMode mode)
{
}

void DebugAddMessage(String const& text, float duration, Rgba8 const& startColor, Rgba8 const& endColor)
{
}

//----------------------------------------------------------------------------------------------------
bool Command_DebugRenderClear(EventArgs& args)
{
    UNUSED(args)
    DebugRenderClear();
    return true;
}

//----------------------------------------------------------------------------------------------------
bool Command_DebugRenderToggle(EventArgs& args)
{
    UNUSED(args)
    m_debugRenderIsVisible = !m_debugRenderIsVisible;
    return true;
}
