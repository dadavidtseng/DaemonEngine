//-----------------------------------------------------------------------------------------------
// Camera.cpp
//

//-----------------------------------------------------------------------------------------------
#include "Engine/Renderer/Camera.hpp"

//-----------------------------------------------------------------------------------------------
void Camera::SetOrthoView(Vec2 const& bottomLeft, Vec2 const& topRight)
{
	m_bottomLeft = bottomLeft;
	m_topRight   = topRight;
}

//-----------------------------------------------------------------------------------------------
Vec2 Camera::GetOrthoBottomLeft() const
{
	return m_bottomLeft;
}

//-----------------------------------------------------------------------------------------------
Vec2 Camera::GetOrthoTopRight() const
{
	return m_topRight;
}

//-----------------------------------------------------------------------------------------------
void Camera::Translate2D(Vec2 const& translation)
{
	m_bottomLeft += translation;
	m_topRight += translation;
}
