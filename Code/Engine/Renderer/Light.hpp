//----------------------------------------------------------------------------------------------------
// Light.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include <vector>

#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/Vec3.hpp"

class Renderer;

//----------------------------------------------------------------------------------------------------
enum class eLightType : int8_t
{
    INVALID = -1,
    DIRECTIONAL,
    POINT,
    SPOT
};

//----------------------------------------------------------------------------------------------------
struct sLightData
{
    float m_color[4]         = {};        // RGBA as floats       (16 bytes)
    float m_worldPosition[3] = {};        // World position       (12 bytes)
    float m_innerRadius      = 0.f;       // Inner radius         (4 bytes)
    float m_direction[3]     = {};        // Direction vector     (12 bytes)
    float m_outerRadius      = 0.f;       // Outer radius         (4 bytes)
    float m_innerConeAngle   = 0.f;       // Inner cone angle     (4 bytes)
    float m_outerConeAngle   = 0.f;       // Outer cone angle     (4 bytes)
    int   m_type             = 0;         // Light type           (4 bytes)
    float padding            = 0.f;       //                      (4 bytes)
};

//----------------------------------------------------------------------------------------------------
struct Light
{
    Light()  = default;
    ~Light() = default;

    // Setters
    Light& SetType(eLightType type);
    Light& SetWorldPosition(Vec3 const& worldPosition);
    Light& SetRadius(float innerRadius, float outerRadius);
    Light& SetColor();
    Light& SetColorWithIntensity(Vec4 const& rgba8);
    Light& SetDirection(Vec3 const& direction);
    Light& SetConeAngles(float innerAngleDegrees, float outerAngleDegrees);


    /// Getters
    // eLightType    GetType() const;
    // Vec3 const&   GetWorldPosition() const;
    // float         GetInnerRadius() const;
    // float         GetOuterRadius() const;
    // Rgba8 const&  GetColor() const;
    // unsigned char GetIntensity() const;
    // Vec3 const&   GetDirection() const;
    // float         GetInnerConeAngle() const;
    // float         GetOuterConeAngle() const;

    // Utility
    // bool IsEnabled() const { return m_isEnabled; }
    // void SetEnabled(bool enabled) { m_isEnabled = enabled; }

private:
    float m_color[4]         = {};
    float m_worldPosition[3] = {};
    float m_innerRadius      = 1.0f;
    float m_direction[3]     = {}; // Default pointing down
    float m_outerRadius      = 10.0f;
    // float      m_intensity      = 1.0f;
    float m_innerConeAngle = CosDegrees(15.0f);  // Cosine of 15 degrees
    float m_outerConeAngle = CosDegrees(30.0f);  // Cosine of 30 degrees
    int   m_type           = (int)eLightType::INVALID;
    float padding          = 0.f;
    // bool  m_isEnabled      = true;
};

//------------------------------------------------------------------------------------------------
// Light Manager class
//------------------------------------------------------------------------------------------------
class LightSubsystem
{
public:
    LightSubsystem();
    ~LightSubsystem();

    // Light management
    void   AddLight(Light const& light);
    void   RemoveLight(int index);
    void   ClearLights();
    Light* GetLight(int index);
    int    GetLightCount() const { return (int)m_lights.size(); }

    // Directional light
    // void              SetDirectionalLight(DirectionalLight const& light) { m_directionalLight = light; }
    // DirectionalLight& GetDirectionalLight() { return m_directionalLight; }

    // Ambient lighting
    void  SetAmbientIntensity(float intensity) { m_ambientIntensity = intensity; }
    float GetAmbientIntensity() const { return m_ambientIntensity; }

    // Update and bind
    void UpdateLightConstants();
    void BindLightConstants();

private:
    std::vector<Light> m_lights;
    // DirectionalLight   m_directionalLight;
    float m_ambientIntensity = 0.2f;

    // LightConstants* m_lightConstants = nullptr;
    // ConstantBuffer* m_lightCBO = nullptr;
};
