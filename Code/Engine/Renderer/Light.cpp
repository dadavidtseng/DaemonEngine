//----------------------------------------------------------------------------------------------------
// Light.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Renderer/Light.hpp"

#include "Renderer.hpp"

Light& Light::SetType(eLightType const type)
{
    m_type = static_cast<int>(type);
    return *this;
}

Light& Light::SetWorldPosition(Vec3 const& worldPosition)
{
    m_worldPosition[0] = worldPosition.x;
    m_worldPosition[1] = worldPosition.y;
    m_worldPosition[2] = worldPosition.z;

    return *this;
}

Light& Light::SetRadius(float const innerRadius,
                        float const outerRadius)
{
    m_innerRadius = innerRadius;
    m_outerRadius = outerRadius;

    return *this;
}

Light& Light::SetColor(Vec3 const& color)
{
    m_color[0] = color.x;
    m_color[1] = color.y;
    m_color[2] = color.z;

    return *this;
}


Light& Light::SetIntensity(float const intensity)
{
    m_color[3] = intensity;

    return *this;
}

Light& Light::SetColorWithIntensity(Vec4 const& rgba8)
{
    m_color[0] = rgba8.x;
    m_color[1] = rgba8.y;
    m_color[2] = rgba8.z;
    m_color[3] = rgba8.w;

    return *this;
}


Light& Light::SetDirection(Vec3 const& direction)
{
    m_direction[0] = direction.x;
    m_direction[1] = direction.y;
    m_direction[2] = direction.z;

    return *this;
}

Light& Light::SetConeAngles(float const innerAngleDegrees,
                            float const outerAngleDegrees)
{
    m_innerConeAngle = innerAngleDegrees;
    m_outerConeAngle = outerAngleDegrees;

    return *this;
}


// eLightType Light::GetType() const
// {
//     return m_type;
// }
//
// Vec3 const& Light::GetWorldPosition() const
// {
//     return m_worldPosition;
// }
//
// float Light::GetInnerRadius() const
// {
//     return m_innerRadius;
// }
//
// float Light::GetOuterRadius() const
// {
//     return m_outerRadius;
// }
//
// Rgba8 const& Light::GetColor() const
// {
//     return m_color;
// }
//
// unsigned char Light::GetIntensity() const
// {
//     return m_color.a;
// }
//
// Vec3 const& Light::GetDirection() const
// {
//     return m_direction;
// }
//
// float Light::GetInnerConeAngle() const
// {
//     return m_innerConeAngle;
// }
//
// float Light::GetOuterConeAngle() const
// {
//     return m_outerConeAngle;
// }


