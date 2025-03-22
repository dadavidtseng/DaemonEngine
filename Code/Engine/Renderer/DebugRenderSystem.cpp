//----------------------------------------------------------------------------------------------------
// DebugRenderSystem.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Renderer/DebugRenderSystem.hpp"

#include <algorithm>
#include <mutex>

#include "Engine/Core/Clock.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Game/GameCommon.hpp"

//----------------------------------------------------------------------------------------------------
static DebugRenderConfig m_debugRenderConfig;
static BitmapFont*       m_debugRenderBitmapFont = nullptr;
static bool              m_debugRenderIsVisible  = true;
std::mutex               m_mutex;

//----------------------------------------------------------------------------------------------------
enum class DebugRenderObjectType : int8_t
{
    WORLD_POINT,
    WORLD_LINE,
    WORLD_WIRE_CYLINDER,
    WORLD_WIRE_SPHERE,
    WORLD_ARROW,
    WORLD_TEXT,
    WORLD_BILLBOARD_TEXT,
    SCREEN_TEXT,
    SCREEN_MESSAGE,
    COUNT
};

//----------------------------------------------------------------------------------------------------
struct DebugRenderObject
{
    DebugRenderObjectType m_type;
    VertexList_PCU            m_vertices;
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
static std::vector<DebugRenderObject*> m_debugRenderObjectList;

//----------------------------------------------------------------------------------------------------
void DebugRenderSystemStartup(DebugRenderConfig const& config)
{
    m_debugRenderConfig.m_renderer = config.m_renderer;
    m_debugRenderConfig.m_fontName = config.m_fontName;
    m_debugRenderBitmapFont        = config.m_renderer->CreateOrGetBitmapFontFromFile(("Data/Fonts/" + config.m_fontName).c_str());
    g_theEventSystem->SubscribeEventCallbackFunction("DebugRenderClear", OnDebugRenderClear);
    g_theEventSystem->SubscribeEventCallbackFunction("DebugRenderToggle", OnDebugRenderToggle);
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
    std::lock_guard lock(m_mutex);

    for (DebugRenderObject*& object : m_debugRenderObjectList)
    {
        if (object != nullptr)
        {
            delete object;
            object = nullptr;
        }
    }

    m_debugRenderObjectList.clear();
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
                it = m_debugRenderObjectList.erase(it);     // Safely remove the element and get the next valid iterator
                continue;       // Prevent `it++` from skipping an element
            }
        }
        ++it;       // Advance the iterator only if the element was not removed
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
void DebugRenderWorld(Camera const& camera)
{
    // 1. Lock the m_mutex.
    m_mutex.lock();

    // 2. If m_debugRenderIsVisible is set to false, unlock the m_mutex, and then return.
    if (m_debugRenderIsVisible == false)
    {
        m_mutex.unlock();

        return;
    }

    //-Start-of-Debug-Render-Camera-------------------------------------------------------------------

    m_debugRenderConfig.m_renderer->BeginCamera(camera);
    m_debugRenderConfig.m_renderer->SetSamplerMode(eSamplerMode::POINT_CLAMP);

    for (DebugRenderObject const* object : m_debugRenderObjectList)
    {
        if (object == nullptr)
        {
            continue;
        }

        // Render WORLD_POINT
        if (object->m_type == DebugRenderObjectType::WORLD_POINT)
        {
            m_debugRenderConfig.m_renderer->BindTexture(nullptr);
            m_debugRenderConfig.m_renderer->SetBlendMode(eBlendMode::OPAQUE);
            m_debugRenderConfig.m_renderer->SetRasterizerMode(eRasterizerMode::SOLID_CULL_BACK);
            m_debugRenderConfig.m_renderer->SetSamplerMode(eSamplerMode::POINT_CLAMP);

            if (object->m_mode == DebugRenderMode::ALWAYS)
            {
                m_debugRenderConfig.m_renderer->SetModelConstants(Mat44(), DebugRenderGetDebugObjectCurrentColor(object));
                m_debugRenderConfig.m_renderer->SetDepthMode(eDepthMode::DISABLED);
                m_debugRenderConfig.m_renderer->DrawVertexArray(static_cast<int>(object->m_vertices.size()), object->m_vertices.data());
            }

            if (object->m_mode == DebugRenderMode::USE_DEPTH)
            {
                m_debugRenderConfig.m_renderer->SetModelConstants(Mat44(), DebugRenderGetDebugObjectCurrentColor(object));
                m_debugRenderConfig.m_renderer->SetDepthMode(eDepthMode::READ_WRITE_LESS_EQUAL);
                m_debugRenderConfig.m_renderer->DrawVertexArray(static_cast<int>(object->m_vertices.size()), object->m_vertices.data());
            }

            if (object->m_mode == DebugRenderMode::X_RAY)
            {
                m_debugRenderConfig.m_renderer->SetModelConstants(Mat44(), DebugRenderGetDebugObjectCurrentColor(object));
                m_debugRenderConfig.m_renderer->SetBlendMode(eBlendMode::ALPHA);
                m_debugRenderConfig.m_renderer->SetDepthMode(eDepthMode::READ_ONLY_ALWAYS);
                m_debugRenderConfig.m_renderer->DrawVertexArray(static_cast<int>(object->m_vertices.size()), object->m_vertices.data());
                m_debugRenderConfig.m_renderer->SetModelConstants(Mat44(), DebugRenderGetDebugObjectCurrentColor(object));
                m_debugRenderConfig.m_renderer->SetBlendMode(eBlendMode::OPAQUE);
                m_debugRenderConfig.m_renderer->SetDepthMode(eDepthMode::READ_WRITE_LESS_EQUAL);
                m_debugRenderConfig.m_renderer->DrawVertexArray(static_cast<int>(object->m_vertices.size()), object->m_vertices.data());
            }
        }

        // Render WORLD_LINE
        if (object->m_type == DebugRenderObjectType::WORLD_LINE)
        {
            m_debugRenderConfig.m_renderer->BindTexture(nullptr);
            m_debugRenderConfig.m_renderer->SetBlendMode(eBlendMode::OPAQUE);
            m_debugRenderConfig.m_renderer->SetRasterizerMode(eRasterizerMode::SOLID_CULL_BACK);

            if (object->m_mode == DebugRenderMode::ALWAYS)
            {
                m_debugRenderConfig.m_renderer->SetModelConstants(Mat44(), DebugRenderGetDebugObjectCurrentColor(object));
                m_debugRenderConfig.m_renderer->SetDepthMode(eDepthMode::DISABLED);
                m_debugRenderConfig.m_renderer->DrawVertexArray(static_cast<int>(object->m_vertices.size()), object->m_vertices.data());
            }

            if (object->m_mode == DebugRenderMode::USE_DEPTH)
            {
                m_debugRenderConfig.m_renderer->SetModelConstants(Mat44(), DebugRenderGetDebugObjectCurrentColor(object));
                m_debugRenderConfig.m_renderer->SetDepthMode(eDepthMode::READ_WRITE_LESS_EQUAL);
                m_debugRenderConfig.m_renderer->DrawVertexArray(static_cast<int>(object->m_vertices.size()), object->m_vertices.data());
            }

            if (object->m_mode == DebugRenderMode::X_RAY)
            {
                m_debugRenderConfig.m_renderer->SetModelConstants(Mat44(), DebugRenderGetDebugObjectCurrentColor(object));
                m_debugRenderConfig.m_renderer->SetBlendMode(eBlendMode::ALPHA);
                m_debugRenderConfig.m_renderer->SetDepthMode(eDepthMode::READ_ONLY_ALWAYS);
                m_debugRenderConfig.m_renderer->DrawVertexArray(static_cast<int>(object->m_vertices.size()), object->m_vertices.data());
                m_debugRenderConfig.m_renderer->SetModelConstants(Mat44(), DebugRenderGetDebugObjectCurrentColor(object));
                m_debugRenderConfig.m_renderer->SetBlendMode(eBlendMode::OPAQUE);
                m_debugRenderConfig.m_renderer->SetDepthMode(eDepthMode::READ_WRITE_LESS_EQUAL);
                m_debugRenderConfig.m_renderer->DrawVertexArray(static_cast<int>(object->m_vertices.size()), object->m_vertices.data());
            }
        }

        // Render WORLD_WIRE_CYLINDER
        if (object->m_type == DebugRenderObjectType::WORLD_WIRE_CYLINDER)
        {
            m_debugRenderConfig.m_renderer->BindTexture(nullptr);
            m_debugRenderConfig.m_renderer->SetModelConstants(Mat44(), DebugRenderGetDebugObjectCurrentColor(object));

            if (object->m_isWireFrame)
            {
                m_debugRenderConfig.m_renderer->SetRasterizerMode(eRasterizerMode::WIREFRAME_CULL_BACK);
            }
            else
            {
                m_debugRenderConfig.m_renderer->SetRasterizerMode(eRasterizerMode::SOLID_CULL_BACK);
            }

            if (object->m_mode == DebugRenderMode::ALWAYS)
            {
                m_debugRenderConfig.m_renderer->SetBlendMode(eBlendMode::OPAQUE);
                m_debugRenderConfig.m_renderer->SetDepthMode(eDepthMode::DISABLED);
                m_debugRenderConfig.m_renderer->DrawVertexArray(static_cast<int>(object->m_vertices.size()), object->m_vertices.data());
            }

            if (object->m_mode == DebugRenderMode::USE_DEPTH)
            {
                m_debugRenderConfig.m_renderer->SetBlendMode(eBlendMode::OPAQUE);
                m_debugRenderConfig.m_renderer->SetDepthMode(eDepthMode::READ_WRITE_LESS_EQUAL);
                m_debugRenderConfig.m_renderer->DrawVertexArray(static_cast<int>(object->m_vertices.size()), object->m_vertices.data());
            }

            if (object->m_mode == DebugRenderMode::X_RAY)
            {
                m_debugRenderConfig.m_renderer->SetModelConstants(Mat44(), DebugRenderGetDebugObjectCurrentColor(object));
                m_debugRenderConfig.m_renderer->SetBlendMode(eBlendMode::ALPHA);
                m_debugRenderConfig.m_renderer->SetDepthMode(eDepthMode::READ_ONLY_ALWAYS);
                m_debugRenderConfig.m_renderer->DrawVertexArray(static_cast<int>(object->m_vertices.size()), object->m_vertices.data());
                m_debugRenderConfig.m_renderer->SetModelConstants(Mat44(), DebugRenderGetDebugObjectCurrentColor(object));
                m_debugRenderConfig.m_renderer->SetBlendMode(eBlendMode::OPAQUE);
                m_debugRenderConfig.m_renderer->SetDepthMode(eDepthMode::READ_WRITE_LESS_EQUAL);
                m_debugRenderConfig.m_renderer->DrawVertexArray(static_cast<int>(object->m_vertices.size()), object->m_vertices.data());
            }
        }

        // Render WORLD_WIRE_SPHERE
        if (object->m_type == DebugRenderObjectType::WORLD_WIRE_SPHERE)
        {
            m_debugRenderConfig.m_renderer->BindTexture(nullptr);
            m_debugRenderConfig.m_renderer->SetModelConstants(object->m_m2wTransform, DebugRenderGetDebugObjectCurrentColor(object));

            if (object->m_isWireFrame)
            {
                m_debugRenderConfig.m_renderer->SetRasterizerMode(eRasterizerMode::WIREFRAME_CULL_BACK);
            }
            else
            {
                m_debugRenderConfig.m_renderer->SetRasterizerMode(eRasterizerMode::SOLID_CULL_BACK);
            }

            if (object->m_mode == DebugRenderMode::ALWAYS)
            {
                m_debugRenderConfig.m_renderer->SetBlendMode(eBlendMode::OPAQUE);
                m_debugRenderConfig.m_renderer->SetDepthMode(eDepthMode::DISABLED);
                m_debugRenderConfig.m_renderer->DrawVertexArray(static_cast<int>(object->m_vertices.size()), object->m_vertices.data());
            }

            if (object->m_mode == DebugRenderMode::USE_DEPTH)
            {
                m_debugRenderConfig.m_renderer->SetBlendMode(eBlendMode::OPAQUE);
                m_debugRenderConfig.m_renderer->SetDepthMode(eDepthMode::READ_WRITE_LESS_EQUAL);
                m_debugRenderConfig.m_renderer->DrawVertexArray(static_cast<int>(object->m_vertices.size()), object->m_vertices.data());
            }

            if (object->m_mode == DebugRenderMode::X_RAY)
            {
                m_debugRenderConfig.m_renderer->SetModelConstants(Mat44(), DebugRenderGetDebugObjectCurrentColor(object));
                m_debugRenderConfig.m_renderer->SetBlendMode(eBlendMode::ALPHA);
                m_debugRenderConfig.m_renderer->SetDepthMode(eDepthMode::READ_ONLY_ALWAYS);
                m_debugRenderConfig.m_renderer->DrawVertexArray(static_cast<int>(object->m_vertices.size()), object->m_vertices.data());
                m_debugRenderConfig.m_renderer->SetModelConstants(Mat44(), DebugRenderGetDebugObjectCurrentColor(object));
                m_debugRenderConfig.m_renderer->SetBlendMode(eBlendMode::OPAQUE);
                m_debugRenderConfig.m_renderer->SetDepthMode(eDepthMode::READ_WRITE_LESS_EQUAL);
                m_debugRenderConfig.m_renderer->DrawVertexArray(static_cast<int>(object->m_vertices.size()), object->m_vertices.data());
            }
        }

        // Render WORLD_ARROW
        if (object->m_type == DebugRenderObjectType::WORLD_ARROW)
        {
            m_debugRenderConfig.m_renderer->BindTexture(nullptr);
            m_debugRenderConfig.m_renderer->SetRasterizerMode(eRasterizerMode::SOLID_CULL_BACK);

            if (object->m_mode == DebugRenderMode::ALWAYS)
            {
                m_debugRenderConfig.m_renderer->SetModelConstants(Mat44(), DebugRenderGetDebugObjectCurrentColor(object));
                m_debugRenderConfig.m_renderer->SetBlendMode(eBlendMode::OPAQUE);
                m_debugRenderConfig.m_renderer->SetDepthMode(eDepthMode::DISABLED);
                m_debugRenderConfig.m_renderer->DrawVertexArray(static_cast<int>(object->m_vertices.size()), object->m_vertices.data());
            }

            if (object->m_mode == DebugRenderMode::USE_DEPTH)
            {
                m_debugRenderConfig.m_renderer->SetModelConstants(Mat44(), DebugRenderGetDebugObjectCurrentColor(object));
                m_debugRenderConfig.m_renderer->SetBlendMode(eBlendMode::OPAQUE);
                m_debugRenderConfig.m_renderer->SetDepthMode(eDepthMode::READ_WRITE_LESS_EQUAL);
                m_debugRenderConfig.m_renderer->DrawVertexArray(static_cast<int>(object->m_vertices.size()), object->m_vertices.data());
            }

            if (object->m_mode == DebugRenderMode::X_RAY)
            {
                m_debugRenderConfig.m_renderer->SetModelConstants(Mat44(), DebugRenderGetDebugObjectCurrentColor(object));
                m_debugRenderConfig.m_renderer->SetBlendMode(eBlendMode::ALPHA);
                m_debugRenderConfig.m_renderer->SetDepthMode(eDepthMode::READ_ONLY_ALWAYS);
                m_debugRenderConfig.m_renderer->DrawVertexArray(static_cast<int>(object->m_vertices.size()), object->m_vertices.data());
                m_debugRenderConfig.m_renderer->SetModelConstants(Mat44(), DebugRenderGetDebugObjectCurrentColor(object));
                m_debugRenderConfig.m_renderer->SetBlendMode(eBlendMode::OPAQUE);
                m_debugRenderConfig.m_renderer->SetDepthMode(eDepthMode::READ_WRITE_LESS_EQUAL);
                m_debugRenderConfig.m_renderer->DrawVertexArray(static_cast<int>(object->m_vertices.size()), object->m_vertices.data());
            }
        }

        // Render WORLD_TEXT
        if (object->m_type == DebugRenderObjectType::WORLD_TEXT)
        {
            m_debugRenderConfig.m_renderer->SetRasterizerMode(eRasterizerMode::SOLID_CULL_NONE);
            m_debugRenderConfig.m_renderer->BindTexture(&m_debugRenderBitmapFont->GetTexture());

            if (object->m_mode == DebugRenderMode::ALWAYS)
            {
                m_debugRenderConfig.m_renderer->SetModelConstants(object->m_m2wTransform, DebugRenderGetDebugObjectCurrentColor(object));
                m_debugRenderConfig.m_renderer->SetBlendMode(eBlendMode::ALPHA);
                m_debugRenderConfig.m_renderer->SetDepthMode(eDepthMode::DISABLED);
                m_debugRenderConfig.m_renderer->DrawVertexArray(static_cast<int>(object->m_vertices.size()), object->m_vertices.data());
            }

            if (object->m_mode == DebugRenderMode::USE_DEPTH)
            {
                m_debugRenderConfig.m_renderer->SetModelConstants(object->m_m2wTransform, DebugRenderGetDebugObjectCurrentColor(object));
                m_debugRenderConfig.m_renderer->SetBlendMode(eBlendMode::ALPHA);
                m_debugRenderConfig.m_renderer->SetDepthMode(eDepthMode::READ_WRITE_LESS_EQUAL);
                m_debugRenderConfig.m_renderer->DrawVertexArray(static_cast<int>(object->m_vertices.size()), object->m_vertices.data());
            }

            if (object->m_mode == DebugRenderMode::X_RAY)
            {
                m_debugRenderConfig.m_renderer->SetModelConstants(Mat44(), DebugRenderGetDebugObjectCurrentColor(object));
                m_debugRenderConfig.m_renderer->SetBlendMode(eBlendMode::ALPHA);
                m_debugRenderConfig.m_renderer->SetDepthMode(eDepthMode::READ_ONLY_ALWAYS);
                m_debugRenderConfig.m_renderer->DrawVertexArray(static_cast<int>(object->m_vertices.size()), object->m_vertices.data());
                m_debugRenderConfig.m_renderer->SetModelConstants(Mat44(), DebugRenderGetDebugObjectCurrentColor(object));
                m_debugRenderConfig.m_renderer->SetBlendMode(eBlendMode::OPAQUE);
                m_debugRenderConfig.m_renderer->SetDepthMode(eDepthMode::READ_WRITE_LESS_EQUAL);
                m_debugRenderConfig.m_renderer->DrawVertexArray(static_cast<int>(object->m_vertices.size()), object->m_vertices.data());
            }
        }

        // Render WORLD_BILLBOARD_TEXT
        if (object->m_type == DebugRenderObjectType::WORLD_BILLBOARD_TEXT)
        {
            m_debugRenderConfig.m_renderer->SetRasterizerMode(eRasterizerMode::SOLID_CULL_NONE);
            m_debugRenderConfig.m_renderer->BindTexture(&m_debugRenderBitmapFont->GetTexture());

            if (object->m_mode == DebugRenderMode::ALWAYS)
            {
                m_debugRenderConfig.m_renderer->SetModelConstants(GetBillboardMatrix(eBillboardType::FULL_OPPOSING, camera.GetCameraToWorldTransform(), object->m_startPosition), DebugRenderGetDebugObjectCurrentColor(object));
                m_debugRenderConfig.m_renderer->SetBlendMode(eBlendMode::ALPHA);
                m_debugRenderConfig.m_renderer->SetDepthMode(eDepthMode::DISABLED);
                m_debugRenderConfig.m_renderer->DrawVertexArray(static_cast<int>(object->m_vertices.size()), object->m_vertices.data());
            }

            if (object->m_mode == DebugRenderMode::USE_DEPTH)
            {
                m_debugRenderConfig.m_renderer->SetModelConstants(GetBillboardMatrix(eBillboardType::FULL_OPPOSING, camera.GetCameraToWorldTransform(), object->m_startPosition), DebugRenderGetDebugObjectCurrentColor(object));
                m_debugRenderConfig.m_renderer->SetBlendMode(eBlendMode::ALPHA);
                m_debugRenderConfig.m_renderer->SetDepthMode(eDepthMode::READ_WRITE_LESS_EQUAL);
                m_debugRenderConfig.m_renderer->DrawVertexArray(static_cast<int>(object->m_vertices.size()), object->m_vertices.data());
            }

            if (object->m_mode == DebugRenderMode::X_RAY)
            {
                m_debugRenderConfig.m_renderer->SetModelConstants(Mat44(), DebugRenderGetDebugObjectCurrentColor(object));
                m_debugRenderConfig.m_renderer->SetBlendMode(eBlendMode::ALPHA);
                m_debugRenderConfig.m_renderer->SetDepthMode(eDepthMode::READ_ONLY_ALWAYS);
                m_debugRenderConfig.m_renderer->DrawVertexArray(static_cast<int>(object->m_vertices.size()), object->m_vertices.data());
                m_debugRenderConfig.m_renderer->SetModelConstants(Mat44(), DebugRenderGetDebugObjectCurrentColor(object));
                m_debugRenderConfig.m_renderer->SetBlendMode(eBlendMode::OPAQUE);
                m_debugRenderConfig.m_renderer->SetDepthMode(eDepthMode::READ_WRITE_LESS_EQUAL);
                m_debugRenderConfig.m_renderer->DrawVertexArray(static_cast<int>(object->m_vertices.size()), object->m_vertices.data());
            }
        }
    }

    m_debugRenderConfig.m_renderer->EndCamera(camera);

    //-End-of-Debug-Render-Camera---------------------------------------------------------------------

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

    m_debugRenderConfig.m_renderer->SetRasterizerMode(eRasterizerMode::SOLID_CULL_NONE);

    std::sort(m_debugRenderObjectList.begin(), m_debugRenderObjectList.end(), [](DebugRenderObject const* a, DebugRenderObject const* b)
    {
        return a->m_maxElapsedTime < b->m_maxElapsedTime;
    });

    float const lineHeight = (camera.GetOrthographicTopRight().y - camera.GetOrthographicBottomLeft().y) / 40.f;
    float       curHeight  = camera.GetOrthographicTopRight().y - lineHeight;

    for (DebugRenderObject* object : m_debugRenderObjectList)
    {
        if (object == nullptr)
        {
            continue;
        }

        if (object->m_type == DebugRenderObjectType::SCREEN_TEXT)
        {
            object->m_vertices.clear();
            m_debugRenderBitmapFont->AddVertsForTextInBox2D(object->m_vertices,
                                                            object->m_text,
                                                            AABB2(Vec2(object->m_startPosition.x, object->m_startPosition.y), Vec2(object->m_startPosition.x, object->m_startPosition.y) + Vec2((float)object->m_text.size() * object->m_textHeight, object->m_textHeight)),
                                                            object->m_textHeight,
                                                            DebugRenderGetDebugObjectCurrentColor(object),
                                                            1.f,
                                                            object->m_alignment,
                                                            OVERRUN);
            m_debugRenderConfig.m_renderer->SetBlendMode(eBlendMode::ALPHA);
            m_debugRenderConfig.m_renderer->BindTexture(&m_debugRenderBitmapFont->GetTexture());
            m_debugRenderConfig.m_renderer->SetModelConstants();
            m_debugRenderConfig.m_renderer->DrawVertexArray(static_cast<int>(object->m_vertices.size()), object->m_vertices.data());
        }

        if (object->m_type == DebugRenderObjectType::SCREEN_MESSAGE)
        {
            object->m_vertices.clear();
            m_debugRenderBitmapFont->AddVertsForText2D(object->m_vertices,
                                                       object->m_text,
                                                       Vec2(0.f, curHeight),
                                                       lineHeight,
                                                       DebugRenderGetDebugObjectCurrentColor(object),
                                                       1.f);
            m_debugRenderConfig.m_renderer->SetBlendMode(eBlendMode::ALPHA);
            m_debugRenderConfig.m_renderer->BindTexture(&m_debugRenderBitmapFont->GetTexture());
            m_debugRenderConfig.m_renderer->SetModelConstants();
            m_debugRenderConfig.m_renderer->DrawVertexArray(static_cast<int>(object->m_vertices.size()), object->m_vertices.data());
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
void DebugAddWorldPoint(Vec3 const&           pos,
                        float const           radius,
                        float const           duration,
                        Rgba8 const&          startColor,
                        Rgba8 const&          endColor,
                        DebugRenderMode const mode)
{
    DebugRenderObject* object = new DebugRenderObject;
    object->m_type            = DebugRenderObjectType::WORLD_POINT;
    object->m_startPosition   = pos;
    object->m_maxElapsedTime  = duration;
    object->m_radius          = radius;
    object->m_startColor      = startColor;
    object->m_endColor        = endColor;
    object->m_isWireFrame     = true;
    object->m_mode            = mode;

    AddVertsForSphere3D(object->m_vertices, object->m_startPosition, radius);
    m_mutex.lock();
    DebugRenderAddObjectToList(object);
    m_mutex.unlock();
}

//----------------------------------------------------------------------------------------------------
void DebugAddWorldLine(Vec3 const&           startPosition,
                       Vec3 const&           endPosition,
                       float const           radius,
                       float const           duration,
                       Rgba8 const&          startColor,
                       Rgba8 const&          endColor,
                       DebugRenderMode const mode)
{
    DebugRenderObject* object = new DebugRenderObject;
    object->m_type            = DebugRenderObjectType::WORLD_LINE;
    object->m_startPosition   = startPosition;
    object->m_endPosition     = endPosition;
    object->m_maxElapsedTime  = duration;
    object->m_radius          = radius;
    object->m_startColor      = startColor;
    object->m_endColor        = endColor;
    object->m_mode            = mode;

    AddVertsForCylinder3D(object->m_vertices, startPosition, endPosition, radius);
    m_mutex.lock();
    DebugRenderAddObjectToList(object);
    m_mutex.unlock();
}

//----------------------------------------------------------------------------------------------------
void DebugAddWorldWireCylinder(Vec3 const&           base,
                               Vec3 const&           top,
                               float const           radius,
                               float const           duration,
                               Rgba8 const&          startColor,
                               Rgba8 const&          endColor,
                               DebugRenderMode const mode)
{
    DebugRenderObject* object = new DebugRenderObject;
    object->m_type            = DebugRenderObjectType::WORLD_WIRE_CYLINDER;
    object->m_startPosition   = base;
    object->m_endPosition     = top;
    object->m_maxElapsedTime  = duration;
    object->m_radius          = radius;
    object->m_startColor      = startColor;
    object->m_endColor        = endColor;
    object->m_isWireFrame     = true;
    object->m_mode            = mode;

    AddVertsForCylinder3D(object->m_vertices, base, top, radius);
    m_mutex.lock();
    DebugRenderAddObjectToList(object);
    m_mutex.unlock();
}

//----------------------------------------------------------------------------------------------------
void DebugAddWorldWireSphere(Vec3 const&           center,
                             float const           radius,
                             float const           duration,
                             Rgba8 const&          startColor,
                             Rgba8 const&          endColor,
                             DebugRenderMode const mode)
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

    AddVertsForSphere3D(object->m_vertices, object->m_startPosition, radius);
    m_mutex.lock();
    DebugRenderAddObjectToList(object);
    m_mutex.unlock();
}

//----------------------------------------------------------------------------------------------------
void DebugAddWorldArrow(Vec3 const&           start,
                        Vec3 const&           end,
                        float const           radius,
                        float const           duration,
                        Rgba8 const&          startColor,
                        Rgba8 const&          endColor,
                        DebugRenderMode const mode)
{
    DebugRenderObject* object = new DebugRenderObject;
    object->m_type            = DebugRenderObjectType::WORLD_ARROW;
    object->m_startPosition   = start;
    object->m_endPosition     = end;
    object->m_maxElapsedTime  = duration;
    object->m_radius          = radius;
    object->m_startColor      = startColor;
    object->m_endColor        = endColor;
    object->m_isWireFrame     = true;
    object->m_mode            = mode;

    AddVertsForArrow3D(object->m_vertices, object->m_startPosition, object->m_endPosition * 2.f, 0.6f, 0.25f, 0.4f, Rgba8::RED);
    m_mutex.lock();
    DebugRenderAddObjectToList(object);
    m_mutex.unlock();
}

//----------------------------------------------------------------------------------------------------
void DebugAddWorldText(String const&         text,
                       Mat44 const&          transform,
                       float const           textHeight,
                       Vec2 const&           alignment,
                       float const           duration,
                       Rgba8 const&          startColor,
                       Rgba8 const&          endColor,
                       DebugRenderMode const mode)
{
    DebugRenderObject* object = new DebugRenderObject;
    object->m_type            = DebugRenderObjectType::WORLD_TEXT;
    object->m_text            = text;
    object->m_m2wTransform    = transform;
    object->m_textHeight      = textHeight;
    object->m_alignment       = alignment;
    object->m_maxElapsedTime  = duration;
    object->m_startColor      = startColor;
    object->m_endColor        = endColor;
    object->m_mode            = mode;

    g_theBitmapFont->AddVertsForText3DAtOriginXForward(object->m_vertices, text, textHeight, startColor, 1.f, alignment);
    m_mutex.lock();
    DebugRenderAddObjectToList(object);
    m_mutex.unlock();
}

//----------------------------------------------------------------------------------------------------
void DebugAddBillboardText(String const&         text,
                           Vec3 const&           origin,
                           float const           textHeight,
                           Vec2 const&           alignment,
                           float const           duration,
                           Rgba8 const&          startColor,
                           Rgba8 const&          endColor,
                           DebugRenderMode const mode)
{
    DebugRenderObject* object = new DebugRenderObject;
    object->m_type            = DebugRenderObjectType::WORLD_BILLBOARD_TEXT;
    object->m_startPosition   = origin;
    object->m_maxElapsedTime  = duration;
    object->m_startColor      = startColor;
    object->m_endColor        = endColor;
    object->m_isWireFrame     = true;
    object->m_mode            = mode;

    g_theBitmapFont->AddVertsForText3DAtOriginXForward(object->m_vertices, text, textHeight, startColor, 1.f, alignment);
    m_mutex.lock();
    DebugRenderAddObjectToList(object);
    m_mutex.unlock();
}

//----------------------------------------------------------------------------------------------------
void DebugAddWorldBasis(Mat44 const&          transform,
                        float const           duration,
                        DebugRenderMode const mode)
{
    DebugRenderObject* object = new DebugRenderObject;
    Vec3 const         origin = transform.GetTranslation3D();
    Vec3 const         iBasis = transform.GetIBasis3D();
    Vec3 const         jBasis = transform.GetJBasis3D();
    Vec3 const         kBasis = transform.GetKBasis3D();

    object->m_type           = DebugRenderObjectType::WORLD_ARROW;
    object->m_maxElapsedTime = duration;
    object->m_mode           = mode;

    AddVertsForArrow3D(object->m_vertices, origin, origin + iBasis, 0.5f, 0.15f, 0.3f, Rgba8::RED);
    AddVertsForArrow3D(object->m_vertices, origin, origin + jBasis, 0.5f, 0.15f, 0.3f, Rgba8::GREEN);
    AddVertsForArrow3D(object->m_vertices, origin, origin + kBasis, 0.5f, 0.15f, 0.3f, Rgba8::BLUE);

    m_mutex.lock();
    DebugRenderAddObjectToList(object);
    m_mutex.unlock();
}

//----------------------------------------------------------------------------------------------------
void DebugAddScreenText(String const&         text,
                        Vec2 const&           position,
                        float const           size,
                        Vec2 const&           alignment,
                        float const           duration,
                        Rgba8 const&          startColor,
                        Rgba8 const&          endColor,
                        DebugRenderMode const mode)
{
    DebugRenderObject* object = new DebugRenderObject;
    object->m_type            = DebugRenderObjectType::SCREEN_TEXT;
    object->m_startPosition   = Vec3(position.x, position.y, 0.f);
    object->m_text            = text;
    object->m_textHeight      = size;
    object->m_alignment       = alignment;
    object->m_maxElapsedTime  = duration;
    object->m_startColor      = startColor;
    object->m_endColor        = endColor;
    object->m_mode            = mode;

    m_mutex.lock();
    DebugRenderAddObjectToList(object);
    m_mutex.unlock();
}

//----------------------------------------------------------------------------------------------------
void DebugAddMessage(String const& text,
                     float const   duration,
                     Rgba8 const&  startColor,
                     Rgba8 const&  endColor)
{
    DebugRenderObject* object = new DebugRenderObject;
    object->m_type            = DebugRenderObjectType::SCREEN_MESSAGE;
    object->m_text            = text;
    object->m_maxElapsedTime  = duration;
    object->m_startColor      = startColor;
    object->m_endColor        = endColor;

    m_mutex.lock();
    DebugRenderAddObjectToList(object);
    m_mutex.unlock();
}

//----------------------------------------------------------------------------------------------------
bool OnDebugRenderClear(EventArgs& args)
{
    UNUSED(args)
    DebugRenderClear();
    return true;
}

//----------------------------------------------------------------------------------------------------
bool OnDebugRenderToggle(EventArgs& args)
{
    UNUSED(args)
    m_debugRenderIsVisible = !m_debugRenderIsVisible;
    return true;
}
