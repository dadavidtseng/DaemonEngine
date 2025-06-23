//----------------------------------------------------------------------------------------------------
// Light.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Renderer/Light.hpp"

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
    : m_direction(direction.GetNormalized())
    , m_color(color)
    , m_intensity(intensity)
{
}

//------------------------------------------------------------------------------------------------
LightManager::LightManager()
{
    m_lightConstants = new LightConstants();
    // Initialize CBO - you'll need to adapt this to your engine's CBO creation method
    // m_lightCBO = renderer->CreateConstantBuffer(sizeof(LightConstants));
}

LightManager::~LightManager()
{
    delete m_lightConstants;
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
    // Clear the structure
    memset(m_lightConstants, 0, sizeof(LightConstants));

    // Set directional light (sun)
    if (m_directionalLight.IsEnabled())
    {
        Vec3 color = m_directionalLight.GetColor().GetAsFloats();
        m_lightConstants->sunColor = Vec4(color.x, color.y, color.z, m_directionalLight.GetIntensity());
        m_lightConstants->sunDirection = m_directionalLight.GetDirection();
    }
    else
    {
        m_lightConstants->sunColor = Vec4(0, 0, 0, 0);
    }

    m_lightConstants->ambientIntensity = m_ambientIntensity;

    // Set point/spot lights
    int activeLights = 0;
    for (int i = 0; i < (int)m_lights.size() && activeLights < MAX_LIGHTS; ++i)
    {
        Light const& light = m_lights[i];
        if (!light.IsEnabled()) continue;

        GPULightData& gpuLight = m_lightConstants->lightArray[activeLights];

        Vec3 color = light.GetColor().GetAsFloats();
        gpuLight.color = Vec4(color.x, color.y, color.z, light.GetIntensity());
        gpuLight.worldPosition = light.GetPosition();
        gpuLight.innerRadius = light.GetInnerRadius();
        gpuLight.outerRadius = light.GetOuterRadius();
        gpuLight.direction = light.GetDirection();
        gpuLight.innerConeAngle = light.GetInnerConeAngle();
        gpuLight.outerConeAngle = light.GetOuterConeAngle();
        gpuLight.lightType = (int)light.GetType();

        activeLights++;
    }

    m_lightConstants->numLights = activeLights;
}

void LightManager::BindLightConstants(Renderer* renderer, int slot)
{
    UpdateLightConstants();

    // Update CBO with new data - adapt this to your engine's method
    // renderer->CopyCPUToGPU(m_lightConstants, sizeof(LightConstants), m_lightCBO);
    // renderer->BindConstantBuffer(slot, m_lightCBO);
}

void LightManager::RenderDebugLights(Renderer* renderer, Camera const& camera)
{
    // Render debug visualization for lights
    // You can implement this to show light positions, radii, and cone angles
    for (Light const& light : m_lights)
    {
        if (!light.IsEnabled()) continue;

        // Draw light position as a colored sphere
        // renderer->DrawSphere(light.GetPosition(), 0.1f, light.GetColor());

        if (light.GetType() == LightType::POINT)
        {
            // Draw outer radius as wireframe sphere
            // renderer->DrawWireframeSphere(light.GetPosition(), light.GetOuterRadius(), Rgba8::YELLOW);
        }
        else if (light.GetType() == LightType::SPOT)
        {
            // Draw spotlight cone
            // renderer->DrawWireframeCone(light.GetPosition(), light.GetDirection(),
            //                           light.GetOuterRadius(), AcosDegrees(light.GetOuterConeAngle()) * 2.0f,
            //                           Rgba8::CYAN);
        }
    }
}