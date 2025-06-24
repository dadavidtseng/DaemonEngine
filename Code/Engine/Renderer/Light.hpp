//----------------------------------------------------------------------------------------------------
// Light.hpp
//----------------------------------------------------------------------------------------------------

#pragma once
#include <vector>

#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"

class Renderer;
//------------------------------------------------------------------------------------------------
// GPU Light Data Structure (matches HLSL)
//------------------------------------------------------------------------------------------------
struct GPULightData
{
    float color[4];           // RGBA as floats
    float worldPosition[3];   // World position
    float innerRadius;        // Inner radius
    float outerRadius;        // Outer radius
    float direction[3];       // Direction vector
    float innerConeAngle;     // Inner cone angle
    float outerConeAngle;     // Outer cone angle
    int   lightType;          // Light type
    float padding[3];         // 改為3個float以達到16字節對齊
};

enum class LightType
{
    DIRECTIONAL = -1,  // Sun light
    POINT = 0,
    SPOT = 1
};

//------------------------------------------------------------------------------------------------
// Light class for Point and Spot lights
//------------------------------------------------------------------------------------------------
class Light
{
public:
    Light();
    Light(LightType type, Vec3 const& position, Rgba8 const& color, float intensity = 1.0f);
    ~Light();

    // Setters
    void SetType(LightType type) { m_type = type; }
    void SetPosition(Vec3 const& position) { m_position = position; }
    void SetColor(Rgba8 const& color) { m_color = color; }
    void SetIntensity(float intensity) { m_intensity = intensity; }
    void SetRadii(float innerRadius, float outerRadius);
    void SetDirection(Vec3 const& direction) { m_direction = direction.GetNormalized(); }
    void SetConeAngles(float innerAngleDegrees, float outerAngleDegrees);

    // Getters
    LightType    GetType() const { return m_type; }
    Vec3 const&  GetPosition() const { return m_position; }
    Rgba8 const& GetColor() const { return m_color; }
    float        GetIntensity() const { return m_intensity; }
    float        GetInnerRadius() const { return m_innerRadius; }
    float        GetOuterRadius() const { return m_outerRadius; }
    Vec3 const&  GetDirection() const { return m_direction; }
    float        GetInnerConeAngle() const { return m_innerConeAngle; }
    float        GetOuterConeAngle() const { return m_outerConeAngle; }

    // Utility
    bool IsEnabled() const { return m_isEnabled; }
    void SetEnabled(bool enabled) { m_isEnabled = enabled; }

private:
    LightType m_type           = LightType::POINT;
    Vec3      m_position       = Vec3::ZERO;
    Rgba8     m_color          = Rgba8::WHITE;
    float     m_intensity      = 1.0f;
    float     m_innerRadius    = 1.0f;
    float     m_outerRadius    = 10.0f;
    Vec3      m_direction      = Vec3(0.0f, 0.0f, -1.0f); // Default pointing down
    float     m_innerConeAngle = CosDegrees(15.0f);  // Cosine of 15 degrees
    float     m_outerConeAngle = CosDegrees(30.0f);  // Cosine of 30 degrees
    bool      m_isEnabled      = true;
};

//------------------------------------------------------------------------------------------------
// Directional Light (Sun) class
//------------------------------------------------------------------------------------------------
class DirectionalLight
{
public:
    DirectionalLight();
    DirectionalLight(Vec3 const& direction, Rgba8 const& color, float intensity = 1.0f);

    // Setters
    void SetDirection(Vec3 const& direction) { m_direction = direction.GetNormalized(); }
    void SetColor(Rgba8 const& color) { m_color = color; }
    void SetIntensity(float intensity) { m_intensity = intensity; }
    void SetEnabled(bool enabled) { m_isEnabled = enabled; }

    // Getters
    Vec3 const&  GetDirection() const { return m_direction; }
    Rgba8 const& GetColor() const { return m_color; }
    float        GetIntensity() const { return m_intensity; }
    bool         IsEnabled() const { return m_isEnabled; }

private:
    Vec3  m_direction = Vec3(1.0f, 0.5f, -1.0f).GetNormalized();
    Rgba8 m_color     = Rgba8::WHITE;
    float m_intensity = 1.0f;
    bool  m_isEnabled = true;
};

//------------------------------------------------------------------------------------------------
// Light Manager class
//------------------------------------------------------------------------------------------------
class LightManager
{
public:
    LightManager();
    ~LightManager();

    // Light management
    void   AddLight(Light const& light);
    void   RemoveLight(int index);
    void   ClearLights();
    Light* GetLight(int index);
    int    GetLightCount() const { return (int)m_lights.size(); }

    // Directional light
    void              SetDirectionalLight(DirectionalLight const& light) { m_directionalLight = light; }
    DirectionalLight& GetDirectionalLight() { return m_directionalLight; }

    // Ambient lighting
    void  SetAmbientIntensity(float intensity) { m_ambientIntensity = intensity; }
    float GetAmbientIntensity() const { return m_ambientIntensity; }

    // Update and bind
    void UpdateLightConstants();
    void BindLightConstants();

private:
    std::vector<Light> m_lights;
    DirectionalLight   m_directionalLight;
    float              m_ambientIntensity = 0.2f;

    // LightConstants* m_lightConstants = nullptr;
    // ConstantBuffer* m_lightCBO = nullptr;
};
