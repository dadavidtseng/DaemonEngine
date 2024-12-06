//----------------------------------------------------------------------------------------------------
// SpriteSheet.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include <vector>

#include "Engine/Renderer/SpriteDefinition.hpp"

//----------------------------------------------------------------------------------------------------
class Texture;
struct IntVec2;
struct AABB2;

//----------------------------------------------------------------------------------------------------
class SpriteSheet
{
public:
    explicit SpriteSheet(Texture const& texture, IntVec2 const& spriteCoords);

    Texture const&          GetTexture() const;
    SpriteDefinition const& GetSpriteDef(int spriteIndex) const;
    AABB2                   GetSpriteUVs(int spriteIndex) const;
    int                     GetTotalSpritesNum() const;

protected:
    Texture const*                m_texture = nullptr;
    std::vector<SpriteDefinition> m_spriteDefs;
};
