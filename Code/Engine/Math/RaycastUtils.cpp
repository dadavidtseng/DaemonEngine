//----------------------------------------------------------------------------------------------------
// RaycastUtils.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Math/RaycastUtils.hpp"

#include <algorithm>
#include <cmath>

#include "AABB2.hpp"
#include "Engine/Math/MathUtils.hpp"

//----------------------------------------------------------------------------------------------------
Ray2::Ray2(Vec2 const& origin, Vec2 const& direction, float maxDist)
    : m_origin(origin),
      m_direction(direction),
      m_maxDist(maxDist)
{
}

//----------------------------------------------------------------------------------------------------
RaycastResult2D RaycastVsDisc2D(Vec2 const& startPos,
                                Vec2 const& forwardNormal,
                                float       maxDist,
                                Vec2 const& discCenter,
                                float       discRadius)
{
    RaycastResult2D result;
    result.m_rayForwardNormal = forwardNormal;
    result.m_rayStartPos      = startPos;
    result.m_rayMaxLength     = maxDist;
    result.m_didImpact        = false; // Initialize as no impact

    Vec2 const  SC     = discCenter - startPos;
    Vec2 const  jBasic = forwardNormal.GetRotated90Degrees();
    float const SCj    = GetProjectedLength2D(SC, jBasic);

    // Check if the ray is outside the disc's influence
    if (SCj > discRadius ||
        SCj < -discRadius)
    {
        return result;
    }

    const float SCi = GetProjectedLength2D(SC, forwardNormal);

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

    result.m_impactPos    = startPos + forwardNormal * result.m_impactDist;
    result.m_impactNormal = (result.m_impactPos - discCenter).GetNormalized();
    result.m_didImpact    = true;

    return result;
}

RaycastResult2D RayCastVsLineSegment2D(Vec2 const& startPos, Vec2 const& forwardNormal, float maxDist, Vec2 const& lineStartPos, Vec2 const& lineEndPos)
{
    RaycastResult2D result;
    result.m_rayForwardNormal = forwardNormal;
    result.m_rayStartPos      = startPos;
    result.m_rayMaxLength     = maxDist;
    result.m_didImpact        = false; // Initialize as no impact

    Vec2  leftNormal = Vec2(forwardNormal.y, -forwardNormal.x);
    Vec2  SS         = lineStartPos - startPos;
    Vec2  SE         = lineEndPos - startPos;
    float SSOnLeft   = DotProduct2D(SS, leftNormal);
    float SEOnLeft   = DotProduct2D(SE, leftNormal);

    if ((SSOnLeft >= 0.f && SEOnLeft >= 0.f) || (SSOnLeft <= 0.f && SEOnLeft <= 0.f))
    {
        return result;
    }

    float t              = SSOnLeft / (SSOnLeft - SEOnLeft);
    Vec2  lineSegForward = lineEndPos - lineStartPos;
    Vec2  impactPos      = lineStartPos + t * lineSegForward;
    Vec2  SI             = impactPos - startPos;
    float impactLength   = DotProduct2D(SI, forwardNormal);
    if (impactLength <= 0.f || impactLength >= maxDist)
    {
        return result;
    }

    // hit
    lineSegForward.Normalize();
    result.m_impactNormal = Vec2(lineSegForward.y, -lineSegForward.x);
    if (SSOnLeft > 0.f)
    {
        result.m_impactNormal = -result.m_impactNormal;
    }
    result.m_didImpact        = true;
    result.m_impactDist       = impactLength;
    result.m_impactPos        = impactPos;
    result.m_rayForwardNormal = forwardNormal;
    result.m_rayMaxLength     = maxDist;
    result.m_rayStartPos      = startPos;
    return result;
}

RaycastResult2D RayCastVsAABB2D(Vec2 const& startPos, Vec2 const& forwardNormal, float maxDist, AABB2 const& aabb2)
{

    RaycastResult2D result;
    result.m_rayForwardNormal = forwardNormal;
    result.m_rayStartPos      = startPos;
    result.m_rayMaxLength     = maxDist;
    result.m_didImpact        = false; // Initialize as no impact

    Vec2 endPos = startPos + forwardNormal * maxDist;

    if (!DoAABB2sOverlap2D(AABB2(Vec2(std::min(startPos.x, endPos.x), std::min(startPos.y, endPos.y)), Vec2(std::max(startPos.x, endPos.x), std::max(startPos.y, endPos.y))), aabb2))
    {
        result.m_didImpact = false;
        return result;
    }
    else if (aabb2.IsPointInside(startPos))
    {
        // ray from inside AABB2
        result.m_didImpact        = true;
        result.m_impactDist       = 0.f;
        result.m_impactNormal     = -forwardNormal;
        result.m_impactPos        = startPos;
        result.m_rayForwardNormal = forwardNormal;
        result.m_rayMaxLength     = maxDist;
        result.m_rayStartPos      = startPos;
        return result;
    }
    else if (forwardNormal.x == 0)
    {
        if (startPos.x <= aabb2.m_mins.x || startPos.x >= aabb2.m_maxs.x)
        {
            // out side, not hit
            result.m_didImpact = false;
            return result;
        }
        float oneOverRangeY = 1.f / (endPos.y - startPos.y);
        float minYHitT      = (aabb2.m_mins.y - startPos.y) * oneOverRangeY;
        float maxYHitT      = (aabb2.m_maxs.y - startPos.y) * oneOverRangeY;
        if (minYHitT < maxYHitT && minYHitT >= 0.f)
        {
            // hit bottom
            result.m_didImpact        = true;
            result.m_impactDist       = maxDist * minYHitT;
            result.m_impactNormal     = Vec2(0.f, -1.f);
            result.m_impactPos        = startPos + forwardNormal * result.m_impactDist;
            result.m_rayForwardNormal = forwardNormal;
            result.m_rayMaxLength     = maxDist;
            result.m_rayStartPos      = startPos;
            return result;
        }
        else if (minYHitT >= maxYHitT && maxYHitT >= 0.f)
        {
            // hit top
            result.m_didImpact        = true;
            result.m_impactDist       = maxDist * maxYHitT;
            result.m_impactNormal     = Vec2(0.f, 1.f);
            result.m_impactPos        = startPos + forwardNormal * result.m_impactDist;
            result.m_rayForwardNormal = forwardNormal;
            result.m_rayMaxLength     = maxDist;
            result.m_rayStartPos      = startPos;
            return result;
        }
        else
        {
            // not hit
            result.m_didImpact = false;
            return result;
        }
    }
    else if (forwardNormal.y == 0)
    {
        if (startPos.y <= aabb2.m_mins.y || startPos.y >= aabb2.m_maxs.y)
        {
            // out side, not hit
            result.m_didImpact = false;
            return result;
        }
        float oneOverRangeX = 1.f / (endPos.x - startPos.x);
        float minXHitT      = (aabb2.m_mins.x - startPos.x) * oneOverRangeX;
        float maxXHitT      = (aabb2.m_maxs.x - startPos.x) * oneOverRangeX;
        if (minXHitT < maxXHitT && minXHitT >= 0.f)
        {
            // hit left
            result.m_didImpact        = true;
            result.m_impactDist       = maxDist * minXHitT;
            result.m_impactNormal     = Vec2(-1.f, 0.f);
            result.m_impactPos        = startPos + forwardNormal * result.m_impactDist;
            result.m_rayForwardNormal = forwardNormal;
            result.m_rayMaxLength     = maxDist;
            result.m_rayStartPos      = startPos;
            return result;
        }
        else if (maxXHitT <= minXHitT && maxXHitT >= 0.f)
        {
            // hit right
            result.m_didImpact        = true;
            result.m_impactDist       = maxDist * maxXHitT;
            result.m_impactNormal     = Vec2(1.f, 0.f);
            result.m_impactPos        = startPos + forwardNormal * result.m_impactDist;
            result.m_rayForwardNormal = forwardNormal;
            result.m_rayMaxLength     = maxDist;
            result.m_rayStartPos      = startPos;
            return result;
        }
        else
        {
            // not hit
            result.m_didImpact = false;
            return result;
        }
    }
    else
    {
        float oneOverRangeX = 1.f / (endPos.x - startPos.x);
        float minXHitT      = (aabb2.m_mins.x - startPos.x) * oneOverRangeX;
        float maxXHitT      = (aabb2.m_maxs.x - startPos.x) * oneOverRangeX;
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

        float oneOverRangeY = 1.f / (endPos.y - startPos.y);
        float minYHitT      = (aabb2.m_mins.y - startPos.y) * oneOverRangeY;
        float maxYHitT      = (aabb2.m_maxs.y - startPos.y) * oneOverRangeY;
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
            else
            {
                if (firstYHitT <= 0.f || firstYHitT >= 1.f)
                {
                    // out of range
                    result.m_didImpact = false;
                    return result;
                }
                // hit
                else if (firstYHitT == minYHitT)
                {
                    // hit bottom
                    result.m_didImpact        = true;
                    result.m_impactDist       = maxDist * firstYHitT;
                    result.m_impactNormal     = Vec2(0.f, -1.f);
                    result.m_impactPos        = startPos + forwardNormal * result.m_impactDist;
                    result.m_rayForwardNormal = forwardNormal;
                    result.m_rayMaxLength     = maxDist;
                    result.m_rayStartPos      = startPos;
                    return result;
                }
                else
                {
                    // hit top
                    result.m_didImpact        = true;
                    result.m_impactDist       = maxDist * firstYHitT;
                    result.m_impactNormal     = Vec2(0.f, 1.f);
                    result.m_impactPos        = startPos + forwardNormal * result.m_impactDist;
                    result.m_rayForwardNormal = forwardNormal;
                    result.m_rayMaxLength     = maxDist;
                    result.m_rayStartPos      = startPos;
                    return result;
                }
            }
        }
        else/*firstYHitT <= firstXHitT*/
        {
            if (firstXHitT >= secondYHitT)
            {
                // not hit
                result.m_didImpact = false;
                return result;
            }
            else
            {
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
                    result.m_didImpact        = true;
                    result.m_impactDist       = maxDist * firstXHitT;
                    result.m_impactNormal     = Vec2(-1.f, 0.f);
                    result.m_impactPos        = startPos + forwardNormal * result.m_impactDist;
                    result.m_rayForwardNormal = forwardNormal;
                    result.m_rayMaxLength     = maxDist;
                    result.m_rayStartPos      = startPos;
                    return result;
                }
                else
                {
                    // hit right
                    result.m_didImpact        = true;
                    result.m_impactDist       = maxDist * firstXHitT;
                    result.m_impactNormal     = Vec2(1.f, 0.f);
                    result.m_impactPos        = startPos + forwardNormal * result.m_impactDist;
                    result.m_rayForwardNormal = forwardNormal;
                    result.m_rayMaxLength     = maxDist;
                    result.m_rayStartPos      = startPos;
                    return result;
                }
            }
        }
    }
}
