//----------------------------------------------------------------------------------------------------
// Light.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Renderer/Light.hpp"

#include "Renderer.hpp"
#include "Game/Framework/GameCommon.hpp"

Light::Light()
{
}

Light::Light(LightType type, Vec3 const& position, Rgba8 const& color, float intensity)
    : m_type(type)
      , m_position(position)
      , m_color(color)
      , m_intensity(intensity)
{
}

Light::~Light()
{
}

void Light::SetRadii(float innerRadius, float outerRadius)
{
    m_innerRadius = innerRadius;
    m_outerRadius = outerRadius;
}

void Light::SetConeAngles(float innerAngleDegrees, float outerAngleDegrees)
{
    m_innerConeAngle = CosDegrees(innerAngleDegrees);
    m_outerConeAngle = CosDegrees(outerAngleDegrees);
}

//------------------------------------------------------------------------------------------------
DirectionalLight::DirectionalLight()
{
}

DirectionalLight::DirectionalLight(Vec3 const& direction, Rgba8 const& color, float intensity)
    : m_direction(direction.GetNormalized()),
      m_color(color),
      m_intensity(intensity)
{
}

//------------------------------------------------------------------------------------------------
LightManager::LightManager()
{
    // Initialize CBO - you'll need to adapt this to your engine's CBO creation method
    // m_lightCBO = renderer->CreateConstantBuffer(sizeof(LightConstants));
}

LightManager::~LightManager()
{
    // delete m_lightCBO; // Or however your engine handles CBO cleanup
}

void LightManager::AddLight(Light const& light)
{
    if (m_lights.size() < MAX_LIGHTS)
    {
        m_lights.push_back(light);
    }
}

void LightManager::RemoveLight(int index)
{
    if (index >= 0 && index < (int)m_lights.size())
    {
        m_lights.erase(m_lights.begin() + index);
    }
}

void LightManager::ClearLights()
{
    m_lights.clear();
}

Light* LightManager::GetLight(int index)
{
    if (index >= 0 && index < (int)m_lights.size())
    {
        return &m_lights[index];
    }
    return nullptr;
}

void LightManager::UpdateLightConstants()
{
    Light light = m_lights[0];
    g_theRenderer->SetLightConstants(light.GetColor(), light.GetDirection(), light.GetIntensity(), m_lights.size());
}

void LightManager::BindLightConstants()
{
}
