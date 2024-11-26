//----------------------------------------------------------------------------------------------------
// SpriteDefinition.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include "Engine/Math/Vec2.hpp"

#include "Engine/Math/AABB2.hpp"

//----------------------------------------------------------------------------------------------------
class Texture;
class SpriteSheet;

//----------------------------------------------------------------------------------------------------
struct SpriteDefinition
{
    SpriteDefinition() = default;
    explicit SpriteDefinition(SpriteSheet const* spriteSheet, int spriteIndex, Vec2 const& uvAtMins, Vec2 const& uvAtMaxs);

    AABB2              GetUVs() const;
    SpriteSheet const* GetSpriteSheet() const;
    Texture const&     GetTexture() const;
    float              GetAspect() const;

    Vec2 GetUVsMins() const { return m_uvAtMins; }
    Vec2 GetUVsMaxs() const { return m_uvAtMaxs; }

protected:
    SpriteSheet const* m_spriteSheet;
    int                m_spriteIndex = -1;
    Vec2               m_uvAtMins    = Vec2::ZERO;
    Vec2               m_uvAtMaxs    = Vec2::ONE;
};
