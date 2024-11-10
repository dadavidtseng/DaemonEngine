//----------------------------------------------------------------------------------------------------
// SpriteSheet.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include <vector>
#include <Engine/Math/Vec2.hpp>

#include "Engine/Math/AABB2.hpp"

class SpriteDefinition;
struct IntVec2;
class Texture;

//-----------------------------------------------------------------------------------------------
class SpriteSheet
{
public:
	explicit SpriteSheet(Texture& texture, IntVec2 const& simpleGridLayout);

	Texture&                GetTexture() const;
	int                     GetNumSprites() const;
	SpriteDefinition const& GetSpriteDef(int spriteIndex) const;
	void                    GetSpriteUVs(Vec2& out_uvAtMins, Vec2& out_uvAtMaxs, int spriteIndex) const;
	AABB2                   GetSpriteUVs(int spriteIndex) const;

protected:
	Texture&                      m_texture;	// reference members must be set in constructor's initializer list
	std::vector<SpriteDefinition> m_spriteDefs;
};
