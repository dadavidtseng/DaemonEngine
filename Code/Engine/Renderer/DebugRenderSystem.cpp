//----------------------------------------------------------------------------------------------------
// DebugRenderSystem.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Renderer/DebugRenderSystem.hpp"

#include <mutex>

#include "BitmapFont.hpp"
#include "Renderer.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/EngineCommon.hpp"

static DebugRenderConfig m_debugRenderConfig;
static BitmapFont*       m_debugRenderBitmapFont = nullptr;
static bool              m_debugRenderIsVisible  = true;
std::mutex               m_mutex;


//----------------------------------------------------------------------------------------------------
enum class DebugRenderObjectType
{
    POINT,
    LINE,
};

//----------------------------------------------------------------------------------------------------
struct DebugRenderObject
{
    DebugRenderObjectType m_type;
    VertexList            m_vertices;
};

std::vector<DebugRenderObject*> m_debugRenderObjectList;

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
}

//----------------------------------------------------------------------------------------------------
void DebugRenderBeginFrame()
{
    float deltaSeconds = static_cast<float>(Clock::GetSystemClock().GetDeltaSeconds());
    float totalSeconds = static_cast<float>(Clock::GetSystemClock().GetTotalSeconds());

    for (int i = 0; i < (int)m_debugRenderObjectList.size(); i++)
    {
        if (m_debugRenderObjectList[i] != nullptr)
        {
            if (totalSeconds > 5.f)
            {
                delete m_debugRenderObjectList[i];
                m_debugRenderObjectList[i] = nullptr;
            }
        }
    }
}

//----------------------------------------------------------------------------------------------------
void DebugRenderWorld(Camera const& camera)
{
    Renderer* renderer = m_debugRenderConfig.m_renderer;

    m_mutex.lock();
    renderer->BeginCamera(camera);

    for (int i = 0; i < (int)m_debugRenderObjectList.size(); i++)
    {
        if (m_debugRenderObjectList[i] == nullptr)
        {
            continue;
        }

        renderer->SetBlendMode(BlendMode::OPAQUE); //AL
        renderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);  //SOLID_CULL_NONE
        renderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
        renderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);  //DISABLE
        renderer->BindTexture(nullptr);
        renderer->DrawVertexArray((int)m_debugRenderObjectList[i]->m_vertices.size(), m_debugRenderObjectList[i]->m_vertices.data());
    }

    renderer->EndCamera(camera);
    m_mutex.unlock();
}

//----------------------------------------------------------------------------------------------------
void DebugRenderScreen(Camera const& camera)
{
}

//----------------------------------------------------------------------------------------------------
void DebugRenderEndFrame()
{
}

//----------------------------------------------------------------------------------------------------
void DebugAddWorldPoint(Vec3 const& pos, float radius, float duration, Rgba8 const& startColor, Rgba8 const& endColor, DebugRenderMode mode)
{
}

//----------------------------------------------------------------------------------------------------
void DebugAddWorldLine(Vec3 const& start, Vec3 const& end, float radius, float duration, Rgba8 const& startColor, Rgba8 const& endColor, DebugRenderMode mode)
{
    DebugRenderObject* obj = new DebugRenderObject;
    // obj->m_type = DebugRenderObjectType::
    AddVertsForCylinder3D(obj->m_vertices, start, end, radius);

    m_mutex.lock();
    for (int i = 0; i < (int)m_debugRenderObjectList.size(); i++)
    {
        if (m_debugRenderObjectList[i] == nullptr)
        {
            m_debugRenderObjectList[i] = obj;
            m_mutex.unlock();
            return;
        }
    }

    m_debugRenderObjectList.push_back(obj);
    m_mutex.unlock();
}

void DebugAddWorldWireCylinder(Vec3 const& base, Vec3 const& top, float radius, float duration, Rgba8 const& startColor, Rgba8 const& endColor, DebugRenderMode mode)
{
}

void DebugAddWorldWireSphere(Vec3 const& center, float radius, float duration, Rgba8 const& startColor, Rgba8 const& endColor, DebugRenderMode mode)
{
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
