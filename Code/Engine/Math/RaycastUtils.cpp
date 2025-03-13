//----------------------------------------------------------------------------------------------------
// RaycastUtils.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Math/RaycastUtils.hpp"

#include <algorithm>
#include <cmath>

#include "Disc2.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/MathUtils.hpp"

//----------------------------------------------------------------------------------------------------
Ray2::Ray2(Vec2 const& startPosition, Vec2 const& normalDirection, float const maxLength)
    : m_startPosition(startPosition),
      m_normalDirection(normalDirection),
      m_maxLength(maxLength)
{
}

//----------------------------------------------------------------------------------------------------
Ray2::Ray2(Vec2 const& startPosition, Vec2 const& endPosition)
    : m_startPosition(startPosition),
      m_normalDirection((endPosition - startPosition).GetNormalized()),
      m_maxLength((endPosition - startPosition).GetLength())
{
}

//----------------------------------------------------------------------------------------------------
Ray2::Ray2(Vec2 const& startPosition, float const orientationDegrees, float const maxLength)
    : m_startPosition(startPosition),
      m_normalDirection((Vec2::MakeFromPolarDegrees(orientationDegrees, maxLength) - startPosition).GetNormalized()),
      m_maxLength(maxLength)
{
}

//----------------------------------------------------------------------------------------------------
RaycastResult2D RaycastVsDisc2D(Vec2 const& rayStartPosition,
                                Vec2 const& rayForwardNormal,
                                float const maxLength,
                                Vec2 const& discCenter,
                                float const discRadius)
{
    // 1. Initialize raycastResult2D.
    RaycastResult2D result;
    result.m_rayForwardNormal = rayForwardNormal;
    result.m_rayStartPosition = rayStartPosition;
    result.m_rayMaxLength     = maxLength;

    // 2. Calculate the startToCenterDirection ( SC ), jBasis, and SCj.
    Vec2 const  SC     = discCenter - rayStartPosition;
    Vec2 const  jBasis = rayForwardNormal.GetRotated90Degrees();
    float const SCj    = GetProjectedLength2D(SC, jBasis);

    // 3. If SCj is too far on the left / right, return with initial raycastResult2D.
    if (SCj >= discRadius ||
        SCj <= -discRadius)
    {
        return result;
    }

    float const SCi = GetProjectedLength2D(SC, rayForwardNormal);

    // 4. If SCi is not intersecting with the disc, return with initial raycastResult2D.
    if (SCi < -discRadius ||
        SCi > maxLength + discRadius)
    {
        return result;
    }

    // 5. Calculate the adjustedLength and the impactDistance
    float const adjustedLength = sqrtf(discRadius * discRadius - SCj * SCj);
    result.m_impactDistance    = SCi - adjustedLength;

    // Ensure the impact distance is within the ray's maximum distance
    if (result.m_impactDistance < 0 ||
        result.m_impactDistance > maxLength)
    {
        return result;
    }

    result.m_impactPosition        = rayStartPosition + rayForwardNormal * result.m_impactDistance;
    result.m_impactNormalDirection = (result.m_impactPosition - discCenter).GetNormalized();
    result.m_didImpact             = true;

    return result;
}

//----------------------------------------------------------------------------------------------------
RaycastResult2D RaycastVsDisc2D(Ray2 const& ray, Vec2 const& discCenter, float const discRadius)
{
    return RaycastVsDisc2D(ray.m_startPosition, ray.m_normalDirection, ray.m_maxLength, discCenter, discRadius);
}

//----------------------------------------------------------------------------------------------------
RaycastResult2D RaycastVsDisc2D(Vec2 const& rayStartPosition, Vec2 const& rayForwardNormal, float const maxLength, Disc2 const& disc)
{
    return RaycastVsDisc2D(rayStartPosition, rayForwardNormal, maxLength, disc.m_position, disc.m_radius);
}

//----------------------------------------------------------------------------------------------------
RaycastResult2D RaycastVsDisc2D(Ray2 const& ray, Disc2 const& disc)
{
    return RaycastVsDisc2D(ray.m_startPosition, ray.m_normalDirection, ray.m_maxLength, disc.m_position, disc.m_radius);
}

//----------------------------------------------------------------------------------------------------
RaycastResult2D RayCastVsLineSegment2D(Vec2 const& rayStartPosition,
                                       Vec2 const& rayForwardNormal,
                                       float const maxDist,
                                       Vec2 const& lineStartPos,
                                       Vec2 const& lineEndPos)
{
    RaycastResult2D result;
    result.m_rayForwardNormal = rayForwardNormal;
    result.m_rayStartPosition = rayStartPosition;
    result.m_rayMaxLength     = maxDist;

    Vec2 const  leftNormal = Vec2(rayForwardNormal.y, -rayForwardNormal.x);
    Vec2 const  SS         = lineStartPos - rayStartPosition;
    Vec2 const  SE         = lineEndPos - rayStartPosition;
    float const SSOnLeft   = DotProduct2D(SS, leftNormal);
    float const SEOnLeft   = DotProduct2D(SE, leftNormal);

    if ((SSOnLeft >= 0.f && SEOnLeft >= 0.f) || (SSOnLeft <= 0.f && SEOnLeft <= 0.f))
    {
        return result;
    }

    float const t              = SSOnLeft / (SSOnLeft - SEOnLeft);
    Vec2        lineSegForward = lineEndPos - lineStartPos;
    Vec2 const  impactPos      = lineStartPos + t * lineSegForward;
    Vec2 const  SI             = impactPos - rayStartPosition;
    float const impactLength   = DotProduct2D(SI, rayForwardNormal);
    if (impactLength <= 0.f || impactLength >= maxDist)
    {
        return result;
    }

    // hit
    lineSegForward.Normalize();
    result.m_impactNormalDirection = Vec2(lineSegForward.y, -lineSegForward.x);
    if (SSOnLeft > 0.f)
    {
        result.m_impactNormalDirection = -result.m_impactNormalDirection;
    }
    result.m_didImpact        = true;
    result.m_impactDistance   = impactLength;
    result.m_impactPosition   = impactPos;
    result.m_rayForwardNormal = rayForwardNormal;
    result.m_rayMaxLength     = maxDist;
    result.m_rayStartPosition = rayStartPosition;
    return result;
}

//----------------------------------------------------------------------------------------------------
RaycastResult2D RayCastVsAABB2D(Vec2 const&  rayStartPosition,
                                Vec2 const&  rayForwardNormal,
                                float        maxDist,
                                AABB2 const& aabb2)
{
    RaycastResult2D result;
    result.m_rayForwardNormal = rayForwardNormal;
    result.m_rayStartPosition = rayStartPosition;
    result.m_rayMaxLength     = maxDist;
    result.m_didImpact        = false; // Initialize as no impact

    Vec2 endPos = rayStartPosition + rayForwardNormal * maxDist;

    if (!DoAABB2sOverlap2D(AABB2(Vec2(std::min(rayStartPosition.x, endPos.x), std::min(rayStartPosition.y, endPos.y)), Vec2(std::max(rayStartPosition.x, endPos.x), std::max(rayStartPosition.y, endPos.y))), aabb2))
    {
        result.m_didImpact = false;
        return result;
    }

    if (aabb2.IsPointInside(rayStartPosition))
    {
        // ray from inside AABB2
        result.m_didImpact             = true;
        result.m_impactDistance        = 0.f;
        result.m_impactNormalDirection = -rayForwardNormal;
        result.m_impactPosition        = rayStartPosition;
        result.m_rayForwardNormal      = rayForwardNormal;
        result.m_rayMaxLength          = maxDist;
        result.m_rayStartPosition      = rayStartPosition;
        return result;
    }

    if (rayForwardNormal.x == 0)
    {
        if (rayStartPosition.x <= aabb2.m_mins.x || rayStartPosition.x >= aabb2.m_maxs.x)
        {
            // out side, not hit
            result.m_didImpact = false;
            return result;
        }
        float oneOverRangeY = 1.f / (endPos.y - rayStartPosition.y);
        float minYHitT      = (aabb2.m_mins.y - rayStartPosition.y) * oneOverRangeY;
        float maxYHitT      = (aabb2.m_maxs.y - rayStartPosition.y) * oneOverRangeY;
        if (minYHitT < maxYHitT && minYHitT >= 0.f)
        {
            // hit bottom
            result.m_didImpact             = true;
            result.m_impactDistance        = maxDist * minYHitT;
            result.m_impactNormalDirection = Vec2(0.f, -1.f);
            result.m_impactPosition        = rayStartPosition + rayForwardNormal * result.m_impactDistance;
            result.m_rayForwardNormal      = rayForwardNormal;
            result.m_rayMaxLength          = maxDist;
            result.m_rayStartPosition      = rayStartPosition;
            return result;
        }

        if (minYHitT >= maxYHitT && maxYHitT >= 0.f)
        {
            // hit top
            result.m_didImpact             = true;
            result.m_impactDistance        = maxDist * maxYHitT;
            result.m_impactNormalDirection = Vec2(0.f, 1.f);
            result.m_impactPosition        = rayStartPosition + rayForwardNormal * result.m_impactDistance;
            result.m_rayForwardNormal      = rayForwardNormal;
            result.m_rayMaxLength          = maxDist;
            result.m_rayStartPosition      = rayStartPosition;
            return result;
        }

        // not hit
        result.m_didImpact = false;
        return result;
    }

    if (rayForwardNormal.y == 0)
    {
        if (rayStartPosition.y <= aabb2.m_mins.y || rayStartPosition.y >= aabb2.m_maxs.y)
        {
            // out side, not hit
            result.m_didImpact = false;
            return result;
        }
        float oneOverRangeX = 1.f / (endPos.x - rayStartPosition.x);
        float minXHitT      = (aabb2.m_mins.x - rayStartPosition.x) * oneOverRangeX;
        float maxXHitT      = (aabb2.m_maxs.x - rayStartPosition.x) * oneOverRangeX;
        if (minXHitT < maxXHitT && minXHitT >= 0.f)
        {
            // hit left
            result.m_didImpact             = true;
            result.m_impactDistance        = maxDist * minXHitT;
            result.m_impactNormalDirection = Vec2(-1.f, 0.f);
            result.m_impactPosition        = rayStartPosition + rayForwardNormal * result.m_impactDistance;
            result.m_rayForwardNormal      = rayForwardNormal;
            result.m_rayMaxLength          = maxDist;
            result.m_rayStartPosition      = rayStartPosition;
            return result;
        }

        if (maxXHitT <= minXHitT && maxXHitT >= 0.f)
        {
            // hit right
            result.m_didImpact             = true;
            result.m_impactDistance        = maxDist * maxXHitT;
            result.m_impactNormalDirection = Vec2(1.f, 0.f);
            result.m_impactPosition        = rayStartPosition + rayForwardNormal * result.m_impactDistance;
            result.m_rayForwardNormal      = rayForwardNormal;
            result.m_rayMaxLength          = maxDist;
            result.m_rayStartPosition      = rayStartPosition;
            return result;
        }

        // not hit
        result.m_didImpact = false;
        return result;
    }

    float oneOverRangeX = 1.f / (endPos.x - rayStartPosition.x);
    float minXHitT      = (aabb2.m_mins.x - rayStartPosition.x) * oneOverRangeX;
    float maxXHitT      = (aabb2.m_maxs.x - rayStartPosition.x) * oneOverRangeX;
    float firstXHitT, secondXHitT;
    if (minXHitT < maxXHitT)
    {
        firstXHitT  = minXHitT;
        secondXHitT = maxXHitT;
    }
    else
    {
        firstXHitT  = maxXHitT;
        secondXHitT = minXHitT;
    }

    float oneOverRangeY = 1.f / (endPos.y - rayStartPosition.y);
    float minYHitT      = (aabb2.m_mins.y - rayStartPosition.y) * oneOverRangeY;
    float maxYHitT      = (aabb2.m_maxs.y - rayStartPosition.y) * oneOverRangeY;
    float firstYHitT, secondYHitT;
    if (minYHitT < maxYHitT)
    {
        firstYHitT  = minYHitT;
        secondYHitT = maxYHitT;
    }
    else
    {
        firstYHitT  = maxYHitT;
        secondYHitT = minYHitT;
    }

    if (firstXHitT < firstYHitT)
    {
        if (firstYHitT >= secondXHitT)
        {
            // not hit
            result.m_didImpact = false;
            return result;
        }

        if (firstYHitT <= 0.f || firstYHitT >= 1.f)
        {
            // out of range
            result.m_didImpact = false;
            return result;
        }

        // hit
        if (firstYHitT == minYHitT)
        {
            // hit bottom
            result.m_didImpact             = true;
            result.m_impactDistance        = maxDist * firstYHitT;
            result.m_impactNormalDirection = Vec2(0.f, -1.f);
            result.m_impactPosition        = rayStartPosition + rayForwardNormal * result.m_impactDistance;
            result.m_rayForwardNormal      = rayForwardNormal;
            result.m_rayMaxLength          = maxDist;
            result.m_rayStartPosition      = rayStartPosition;
            return result;
        }

        // hit top
        result.m_didImpact             = true;
        result.m_impactDistance        = maxDist * firstYHitT;
        result.m_impactNormalDirection = Vec2(0.f, 1.f);
        result.m_impactPosition        = rayStartPosition + rayForwardNormal * result.m_impactDistance;
        result.m_rayForwardNormal      = rayForwardNormal;
        result.m_rayMaxLength          = maxDist;
        result.m_rayStartPosition      = rayStartPosition;
        return result;
    }

    /*firstYHitT <= firstXHitT*/
    if (firstXHitT >= secondYHitT)
    {
        // not hit
        result.m_didImpact = false;
        return result;
    }

    if (firstXHitT <= 0.f || firstXHitT >= 1.f)
    {
        // out of range
        result.m_didImpact = false;
        return result;
    }
    // hit
    if (firstXHitT == minXHitT)
    {
        // hit left
        result.m_didImpact             = true;
        result.m_impactDistance        = maxDist * firstXHitT;
        result.m_impactNormalDirection = Vec2(-1.f, 0.f);
        result.m_impactPosition        = rayStartPosition + rayForwardNormal * result.m_impactDistance;
        result.m_rayForwardNormal      = rayForwardNormal;
        result.m_rayMaxLength          = maxDist;
        result.m_rayStartPosition      = rayStartPosition;
        return result;
    }

    // hit right
    result.m_didImpact             = true;
    result.m_impactDistance        = maxDist * firstXHitT;
    result.m_impactNormalDirection = Vec2(1.f, 0.f);
    result.m_impactPosition        = rayStartPosition + rayForwardNormal * result.m_impactDistance;
    result.m_rayForwardNormal      = rayForwardNormal;
    result.m_rayMaxLength          = maxDist;
    result.m_rayStartPosition      = rayStartPosition;
    return result;
}

RaycastResult3D RaycastVsAABB3D(Vec3 rayStartPosition, Vec3 rayForwardNormal, float rayLength, AABB3 box)
{
    return {};
}

RaycastResult3D RaycastVsSphere3D(Vec3 rayStartPosition, Vec3 rayForwardNormal, float rayLength, Vec3 sphereCenter, float sphereRadius)
{
    return {};
}

RaycastResult3D RaycastVsCylinderZ3D(Vec3 rayStartPosition, Vec3 rayForwardNormal, float rayLength, Vec3 const& centerXY, FloatRange const& minMaxZ, float radiusXY)
{
    return {};
}
