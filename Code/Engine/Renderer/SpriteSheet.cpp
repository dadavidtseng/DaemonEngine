//----------------------------------------------------------------------------------------------------
// SpriteSheet.cpp
//----------------------------------------------------------------------------------------------------

#include "Engine/Renderer/SpriteSheet.hpp"
#include "SpriteDefinition.hpp"
#include "Engine/Math/IntVec2.hpp"

//----------------------------------------------------------------------------------------------------
SpriteSheet::SpriteSheet(Texture& texture, IntVec2 const& simpleGridLayout)
	: m_texture(texture)
{
	int totalSprites = simpleGridLayout.x * simpleGridLayout.y;
	m_spriteDefs.resize(totalSprites);

	for (int i = 0; i < totalSprites; ++i)
	{
		int  row    = i / simpleGridLayout.x;
		int  col    = i % simpleGridLayout.x;
		Vec2 uvMins = Vec2(col / static_cast<float>(simpleGridLayout.x), row / static_cast<float>(simpleGridLayout.y));
		Vec2 uvMaxs = Vec2((col + 1) / static_cast<float>(simpleGridLayout.x), (row + 1) / static_cast<float>(simpleGridLayout.y));

		m_spriteDefs[i] = SpriteDefinition(this, i, uvMins, uvMaxs); // Pass 'this' as a pointer
	}
}

//----------------------------------------------------------------------------------------------------
Texture& SpriteSheet::GetTexture() const
{
	return m_texture;
}

//----------------------------------------------------------------------------------------------------
int SpriteSheet::GetNumSprites() const
{
	return static_cast<int>(m_spriteDefs.size());
}

//----------------------------------------------------------------------------------------------------
SpriteDefinition const& SpriteSheet::GetSpriteDef(int spriteIndex) const
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
