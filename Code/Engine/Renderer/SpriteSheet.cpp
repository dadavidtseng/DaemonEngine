//----------------------------------------------------------------------------------------------------
// SpriteSheet.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Renderer/SpriteSheet.hpp"

#include "Texture.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/IntVec2.hpp"

//----------------------------------------------------------------------------------------------------
SpriteSheet::SpriteSheet(Texture const& texture, IntVec2 const& spriteCoords)
    : m_texture(&texture)
{
    int const totalSprites = spriteCoords.x * spriteCoords.y;
    m_spriteDefs.reserve(totalSprites);

    float const uvWidth  = 1.f / static_cast<float>(spriteCoords.x);
    float const uvHeight = 1.f / static_cast<float>(spriteCoords.y);

    // Get texture dimensions
    float const textureWidth  = static_cast<float>(texture.GetDimensions().x);
    float const textureHeight = static_cast<float>(texture.GetDimensions().y);

    // Calculate nudge values
    float const uNudge = 1.f / (128.f * textureWidth);
    float const vNudge = 1.f / (128.f * textureHeight);

    for (int row = 0; row < spriteCoords.y; ++row)
    {
        for (int col = 0; col < spriteCoords.x; ++col)
        {
            // Compute UV coordinates with nudge adjustment
            Vec2 const uvMins(static_cast<float>(col) * uvWidth + uNudge,
                              static_cast<float>(spriteCoords.y - row - 1) * uvHeight + vNudge);
            Vec2 const uvMaxs(static_cast<float>(col + 1) * uvWidth - uNudge,
                              static_cast<float>(spriteCoords.y - row) * uvHeight - vNudge);

            // Add the sprite definition
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
