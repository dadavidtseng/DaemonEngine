//----------------------------------------------------------------------------------------------------
// SpriteSheet.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Renderer/SpriteSheet.hpp"

#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/IntVec2.hpp"

//----------------------------------------------------------------------------------------------------
SpriteSheet::SpriteSheet(Texture const& texture, IntVec2 const& spriteCoords)
    : m_texture(&texture)
{
    int const totalSprites = spriteCoords.x * spriteCoords.y;
    m_spriteDefs.reserve(totalSprites);

    float const uvWidth  = 1.0f / static_cast<float>(spriteCoords.x);
    float const uvHeight = 1.0f / static_cast<float>(spriteCoords.y);

    for (int row = 0; row < spriteCoords.y; ++row)
    {
        for (int col = 0; col < spriteCoords.x; ++col)
        {
            Vec2 uvMins(static_cast<float>(col) * uvWidth, static_cast<float>(spriteCoords.y - row - 1) * uvHeight);
            Vec2 uvMaxs(static_cast<float>(col + 1) * uvWidth, static_cast<float>(spriteCoords.y - row) * uvHeight);

            m_spriteDefs.emplace_back(this, uvMins, uvMaxs);
        }
    }
}

//----------------------------------------------------------------------------------------------------
Texture const& SpriteSheet::GetTexture() const
{
    return *m_texture;
}

//----------------------------------------------------------------------------------------------------
int SpriteSheet::GetTotalSpritesNum() const
{
    return static_cast<int>(m_spriteDefs.size());
}

//----------------------------------------------------------------------------------------------------
SpriteDefinition const& SpriteSheet::GetSpriteDef(int const spriteIndex) const
{
    return m_spriteDefs[spriteIndex];
}

//----------------------------------------------------------------------------------------------------
AABB2 SpriteSheet::GetSpriteUVs(int const spriteIndex) const
{
    return m_spriteDefs[spriteIndex].GetUVs();
}
