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
	result.m_didImpact    = false; // Initialize as no impact

	const Vec2  SC     = discCenter - startPos;
	const Vec2  jBasic = fwdNormal.GetRotated90Degrees();
	const float SCj    = GetProjectedLength2D(SC, jBasic);

	// Check if the ray is outside the disc's influence
	if (SCj > discRadius ||
		SCj < -discRadius)
	{
		return result;
	}

	const float SCi = GetProjectedLength2D(SC, fwdNormal);

	// Check if the ray is too far away to intersect
	if (SCi < -discRadius ||
		SCi > maxDist + discRadius)
	{
		return result;
	}

	const float adjust  = sqrtf(discRadius * discRadius - SCj * SCj);
	result.m_impactDist = SCi - adjust;

	// Ensure the impact distance is within the ray's maximum distance
	if (result.m_impactDist < 0 ||
		result.m_impactDist > maxDist)
	{
		return result;
	}

	result.m_impactPos    = startPos + fwdNormal * result.m_impactDist;
	result.m_impactNormal = (result.m_impactPos - discCenter).GetNormalized();
	result.m_didImpact    = true;

	return result;
}
