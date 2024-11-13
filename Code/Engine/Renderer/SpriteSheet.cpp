//----------------------------------------------------------------------------------------------------
// SpriteSheet.cpp
//----------------------------------------------------------------------------------------------------

#include "Engine/Renderer/SpriteSheet.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Renderer/SpriteDefinition.hpp"

//----------------------------------------------------------------------------------------------------
SpriteSheet::SpriteSheet(Texture* texture, IntVec2 const& simpleGridLayout)
    : m_texture(texture)
{
    int totalSprites = simpleGridLayout.x * simpleGridLayout.y;
    m_spriteDefs.reserve(totalSprites);

    float uvWidth  = 1.0f / static_cast<float>(simpleGridLayout.x);
    float uvHeight = 1.0f / static_cast<float>(simpleGridLayout.y);

    for (int row = 0; row < simpleGridLayout.y; ++row)
    {
        for (int col = 0; col < simpleGridLayout.x; ++col)
        {
            Vec2 uvMins(col * uvWidth, (simpleGridLayout.y - row - 1) * uvHeight);
            Vec2 uvMaxs((col + 1) * uvWidth, (simpleGridLayout.y - row) * uvHeight);
            int  spriteIndex = row * simpleGridLayout.x + col;

            m_spriteDefs.emplace_back(this, spriteIndex, uvMins, uvMaxs);

            printf("uvMins: (%f %f) | uvMaxs: (%f, %f)\n", uvMins.x, uvMins.y, uvMaxs.x, uvMaxs.y);
        }
    }
}

//----------------------------------------------------------------------------------------------------
Texture& SpriteSheet::GetTexture() const
{
    return *m_texture;
}

//----------------------------------------------------------------------------------------------------
int SpriteSheet::GetNumSprites() const
{
    return static_cast<int>(m_spriteDefs.size());
}

//----------------------------------------------------------------------------------------------------
SpriteDefinition const& SpriteSheet::GetSpriteDef(int const spriteIndex) const
{
    return m_spriteDefs[spriteIndex];
}

//----------------------------------------------------------------------------------------------------
void SpriteSheet::GetSpriteUVs(Vec2& out_uvAtMins, Vec2& out_uvAtMaxs, int spriteIndex) const
{
    out_uvAtMins = m_spriteDefs[spriteIndex].GetUVsMins();
    out_uvAtMaxs = m_spriteDefs[spriteIndex].GetUVsMaxs();
}

//----------------------------------------------------------------------------------------------------
AABB2 SpriteSheet::GetSpriteUVs(int spriteIndex) const
{
    return m_spriteDefs[spriteIndex].GetUVs();
}
