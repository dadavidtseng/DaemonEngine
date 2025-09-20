//----------------------------------------------------------------------------------------------------
// LightSubsystem.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Renderer/LightSubsystem.hpp"

#include "Engine/Renderer/Light.hpp"
#include "Engine/Renderer/RenderCommon.hpp"
#include "Engine/Renderer/Renderer.hpp"

//------------------------------------------------------------------------------------------------
LightSubsystem::LightSubsystem()
{
    // Initialize CBO - you'll need to adapt this to your engine's CBO creation method
}

LightSubsystem::LightSubsystem(sLightSubsystemConfig const config)
    : m_config(config)
{
}

void LightSubsystem::StartUp()
{
    Light* light1 = new Light();
    light1->SetType(eLightType::SPOT)
          .SetWorldPosition(Vec3(2.f, 2.f, 5.f))
          .SetRadius(0.5f, 15.f)
          .SetColor(Rgba8::CYAN.GetAsVec3())
          .SetIntensity(8.f)
          .SetDirection(-Vec3::Z_BASIS)
          .SetConeAngles(CosDegrees(5.f), CosDegrees(25.f));

    Light* light2 = new Light();
    light2->SetType(eLightType::SPOT)
          .SetWorldPosition(Vec3(4, 4, 5))
          .SetRadius(0.5f, 15.f)
          .SetColorWithIntensity(Vec4(1.f, 0.f, 1.f, 8.f))
          .SetDirection(-Vec3::Z_BASIS)
          .SetConeAngles(CosDegrees(5.f), CosDegrees(25.f));

    Light* light3 = new Light();
    light3->SetType(eLightType::DIRECTIONAL)
          .SetColor(Rgba8::WHITE.GetAsVec3())
          .SetIntensity(1.f)
          .SetDirection(Vec3(2.f, 1.f, -1.f).GetNormalized());


    AddLight(light1);
    AddLight(light2);
    AddLight(light3);
}

void LightSubsystem::BeginFrame(Renderer* renderer)
{
    if (renderer != nullptr)
    {
        renderer->SetLightConstants(m_lights, GetLightCount());
    }
}

void LightSubsystem::Update()
{
}

void LightSubsystem::Render()
{
}

void LightSubsystem::EndFrame()
{
}

void LightSubsystem::ShutDown()
{
    for (Light* light : m_lights)
    {
        delete light;
        light = nullptr;
    }
}

void LightSubsystem::AddLight(Light* light)
{
    if (m_lights.size() < MAX_LIGHTS)
    {
        if (light == nullptr) return;
        m_lights.push_back(light);
    }
}

void LightSubsystem::RemoveLight(int index)
{
    if (index >= 0 && index < (int)m_lights.size())
    {
        m_lights.erase(m_lights.begin() + index);
    }
}

void LightSubsystem::ClearLights()
{
    m_lights.clear();
}

Light* LightSubsystem::GetLight(int index)
{
    if (index >= 0 && index < (int)m_lights.size())
    {
        return m_lights[index];
    }
    return nullptr;
}

int LightSubsystem::GetLightCount() const
{
    return (int)m_lights.size();
}