//----------------------------------------------------------------------------------------------------
// SpriteDefinition.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Renderer/SpriteDefinition.hpp"

#include "Engine/Math/AABB2.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"

//----------------------------------------------------------------------------------------------------
SpriteDefinition::SpriteDefinition(SpriteSheet const* spriteSheet, Vec2 const& uvAtMins, Vec2 const& uvAtMaxs)
    : m_spriteSheet(spriteSheet),
      m_uvAtMins(uvAtMins),
      m_uvAtMaxs(uvAtMaxs)
{
}

//----------------------------------------------------------------------------------------------------
AABB2 SpriteDefinition::GetUVs() const
{
    return AABB2(m_uvAtMins, m_uvAtMaxs);
}

//----------------------------------------------------------------------------------------------------
SpriteSheet const* SpriteDefinition::GetSpriteSheet() const
{
    return m_spriteSheet;
}

//----------------------------------------------------------------------------------------------------
Texture const& SpriteDefinition::GetTexture() const
{
    return m_spriteSheet->GetTexture();
}

//----------------------------------------------------------------------------------------------------
float SpriteDefinition::GetAspect() const
{
    return (m_uvAtMaxs.x - m_uvAtMins.x) / (m_uvAtMaxs.y - m_uvAtMins.y);
}
