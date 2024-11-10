//----------------------------------------------------------------------------------------------------
// RaycastUtils.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include "Vec2.hpp"

//----------------------------------------------------------------------------------------------------
struct RaycastResult2D
{
	// Basic raycast result information (required)
	bool  m_didImpact  = false;
	float m_impactDist = 0.f;
	Vec2  m_impactPos;
	Vec2  m_impactNormal;

	// Original raycast information (optional)
	Vec2  m_rayFwdNormal;
	Vec2  m_rayStartPos;
	float m_rayMaxLength = 1.f;
};

RaycastResult2D RaycastVsDisc2D(const Vec2& startPos, const Vec2& fwdNormal, float maxDist, const Vec2& discCenter, float discRadius);
