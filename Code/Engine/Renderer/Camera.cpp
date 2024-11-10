//-----------------------------------------------------------------------------------------------
// Camera.cpp
//

//-----------------------------------------------------------------------------------------------
#include "Engine/Renderer/Camera.hpp"

//-----------------------------------------------------------------------------------------------
void SpriteSheet::SetOrthoView(Vec2 const& bottomLeft, Vec2 const& topRight)
{
	m_bottomLeft = bottomLeft;
	m_topRight   = topRight;
}

//-----------------------------------------------------------------------------------------------
Vec2 SpriteSheet::GetOrthoBottomLeft() const
{
	return m_bottomLeft;
}

//-----------------------------------------------------------------------------------------------
Vec2 SpriteSheet::GetOrthoTopRight() const
{
	return m_topRight;
}

//-----------------------------------------------------------------------------------------------
void SpriteSheet::Translate2D(Vec2 const& translation)
{
	m_bottomLeft += translation;
	m_topRight += translation;
}
