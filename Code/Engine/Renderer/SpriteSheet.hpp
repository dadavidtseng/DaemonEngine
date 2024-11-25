//----------------------------------------------------------------------------------------------------
// SpriteSheet.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include <vector>

#include "Engine/Renderer/SpriteDefinition.hpp"
#include "Engine/Math/AABB2.hpp"

//----------------------------------------------------------------------------------------------------
// class SpriteDefinition;
class Texture;
struct IntVec2;

//----------------------------------------------------------------------------------------------------
class SpriteSheet
{
public:
    explicit SpriteSheet(Texture& texture, IntVec2 const& spriteCoords);

    Texture const&          GetTexture() const;
    SpriteDefinition const& GetSpriteDef(int spriteIndex) const;
    AABB2                   GetSpriteUVs(int spriteIndex) const;
    int                     GetTotalSpritesNum() const;

protected:
    Texture const&                m_texture;	// reference members must be set in constructor's initializer list
    std::vector<SpriteDefinition> m_spriteDefs;
};
