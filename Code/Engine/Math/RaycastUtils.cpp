//----------------------------------------------------------------------------------------------------
// RaycastUtils.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Math/RaycastUtils.hpp"

#include <corecrt_math.h>

#include "MathUtils.hpp"

//----------------------------------------------------------------------------------------------------
RaycastResult2D RaycastVsDisc2D(const Vec2& startPos,
                                const Vec2& fwdNormal,
                                const float maxDist,
                                const Vec2& discCenter,
                                const float discRadius)
{
	RaycastResult2D result;
	result.m_rayFwdNormal = fwdNormal;
	result.m_rayStartPos  = startPos;
	result.m_rayMaxLength = maxDist;

	// 計算射線與圓心的距離
	const Vec2  toCenter         = discCenter - startPos;
	const float projectionLength = DotProduct2D(toCenter, fwdNormal);

	if (projectionLength < 0 || projectionLength > maxDist)
	{
		// 圓形位於射線的後方或超出最大距離
		return result;
	}

	// 計算射線上離圓心最近的點
	const Vec2  closestPoint     = startPos + fwdNormal * projectionLength;
	const float distanceToCenter = (closestPoint - discCenter).GetLength();

	if (distanceToCenter > discRadius)
	{
		// 沒有交集
		return result;
	}

	// 計算從最近點到交點的距離
	float offsetDistance       = sqrtf(discRadius * discRadius - distanceToCenter * distanceToCenter);
	float intersectionDistance = projectionLength - offsetDistance;

	if (intersectionDistance < 0 || intersectionDistance > maxDist)
	{
		// 交點超出最大距離
		return result;
	}

	// 計算交點位置
	result.m_didImpact  = true;
	result.m_impactDist = intersectionDistance;
	result.m_impactPos  = startPos + fwdNormal * intersectionDistance;

	// 計算交點的法線
	result.m_impactNormal = (result.m_impactPos - discCenter).GetNormalized();

	return result;
}
