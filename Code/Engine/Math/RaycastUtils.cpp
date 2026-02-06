//----------------------------------------------------------------------------------------------------
// RaycastUtils.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Math/RaycastUtils.hpp"
//----------------------------------------------------------------------------------------------------
#include "ConvexHull2.hpp"
#include "Plane2.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Math/Mat44.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/OBB3.hpp"
#include "Engine/Math/Plane3.hpp"

//----------------------------------------------------------------------------------------------------
Ray2::Ray2(Vec2 const& startPosition,
           Vec2 const& forwardNormal,
           float const maxLength)
    : m_startPosition(startPosition),
      m_forwardNormal(forwardNormal),
      m_maxLength(maxLength)
{
}

//----------------------------------------------------------------------------------------------------
Ray2::Ray2(Vec2 const& startPosition,
           Vec2 const& endPosition)
    : m_startPosition(startPosition),
      m_forwardNormal((endPosition - startPosition).GetNormalized()),
      m_maxLength((endPosition - startPosition).GetLength())
{
}

//----------------------------------------------------------------------------------------------------
Ray2::Ray2(Vec2 const& startPosition,
           float const orientationDegrees,
           float const maxLength)
    : m_startPosition(startPosition),
      m_forwardNormal((Vec2::MakeFromPolarDegrees(orientationDegrees, maxLength) - startPosition).GetNormalized()),
      m_maxLength(maxLength)
{
}

//----------------------------------------------------------------------------------------------------
Ray3::Ray3(Vec3 const& startPosition,
           Vec3 const& forwardNormal,
           float const maxLength)
    : m_startPosition(startPosition),
      m_forwardNormal(forwardNormal),
      m_maxLength(maxLength)
{
}

//----------------------------------------------------------------------------------------------------
Ray3::Ray3(Vec3 const& startPosition,
           Vec3 const& endPosition)
    : m_startPosition(startPosition),
      m_forwardNormal((endPosition - startPosition).GetNormalized()),
      m_maxLength((endPosition - startPosition).GetLength())
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
    result.m_rayStartPosition = rayStartPosition;
    result.m_rayForwardNormal = rayForwardNormal;
    result.m_rayMaxLength     = maxLength;

    // 2. Calculate the startToCenterDirection ( SC ), jBasis, and SCj.
    Vec2 const  SC     = discCenter - rayStartPosition;
    Vec2 const  jBasis = Vec2(rayForwardNormal.y, -rayForwardNormal.x);
    float const SCj    = GetProjectedLength2D(SC, jBasis);

    // 3. If SCj is too far on the left / right, return with initial raycastResult2D.
    if (SCj >= discRadius ||
        SCj <= -discRadius)
    {
        return result;
    }

    // 4. Calculate SCi.
    float const SCi = GetProjectedLength2D(SC, rayForwardNormal);

    // 5. If SCi is not intersecting with the disc, return with initial raycastResult2D.
    if (SCi < -discRadius ||
        SCi > maxLength + discRadius)
    {
        return result;
    }

    // 6. Calculate the adjustedLength and the impactDistance.
    float const adjustedLength = sqrtf(discRadius * discRadius - SCj * SCj);
    result.m_impactLength      = SCi - adjustedLength;

    // 7. If the ray hits the disc before the rayStartPosition or after the ray's maxLength,
    // return with no hit. (In steps 5, there are some rare cases where they might pass the checkpoints.)
    if (result.m_impactLength < 0 ||
        result.m_impactLength > maxLength)
    {
        return result;
    }

    // RAY HIT
    result.m_didImpact      = true;
    result.m_impactPosition = rayStartPosition + rayForwardNormal * result.m_impactLength;
    result.m_impactNormal   = (result.m_impactPosition - discCenter).GetNormalized();

    return result;
}

//----------------------------------------------------------------------------------------------------
RaycastResult2D RaycastVsLineSegment2D(Vec2 const& rayStartPosition,
                                       Vec2 const& rayForwardNormal,
                                       float const maxLength,
                                       Vec2 const& lineStartPosition,
                                       Vec2 const& lineEndPosition)
{
    // 1. Initialize raycastResult2D.
    RaycastResult2D result;
    result.m_rayStartPosition = rayStartPosition;
    result.m_rayForwardNormal = rayForwardNormal;
    result.m_rayMaxLength     = maxLength;

    // 2. Calculate the jBasis; rayStartToLineStart ( RS ), rayStartToLineEnd ( RE ), and their jBasis ( RSj and REj ).
    Vec2 const  jBasis = Vec2(rayForwardNormal.y, -rayForwardNormal.x);
    Vec2 const  RS     = lineStartPosition - rayStartPosition;
    Vec2 const  RE     = lineEndPosition - rayStartPosition;
    float const RSj    = GetProjectedLength2D(RS, jBasis);
    float const REj    = GetProjectedLength2D(RE, jBasis);

    // 3. If the ray fails to straddle with the line, return with no hit.
    if (RSj * REj >= 0)
    {
        return result;
    }

    // 4. Calculate RSi and REi.
    float const RSi = GetProjectedLength2D(RS, rayForwardNormal);
    float const REi = GetProjectedLength2D(RE, rayForwardNormal);

    // 5. If the ray hits the line after the ray's maxLength, return with not hit.
    if (RSi >= maxLength && REi >= maxLength)
    {
        return result;
    }

    // 6. If the ray hits the line before the rayStartPosition, return with no hit.
    if (RSi <= 0.f && REi <= 0.f)
    {
        return result;
    }

    // 7. Calculate the impact position using the rule of similar triangles.
    float const t              = RSj / (RSj - REj);
    Vec2 const  SE             = lineEndPosition - lineStartPosition;
    Vec2 const  impactPosition = lineStartPosition + t * SE;

    // 7. Calculate rayStartToImpact ( RI ) and the impactLength.
    Vec2 const  RI           = impactPosition - rayStartPosition;
    float const impactLength = DotProduct2D(RI, rayForwardNormal);

    // 8. If the ray hits the line before the rayStartPosition or after the ray's maxLength,
    // return with no hit. (In steps 5 and 6, there are some rare cases where they might pass the checkpoints.)
    if (impactLength <= 0.f || impactLength >= maxLength)
    {
        return result;
    }

    // RAY HIT
    // 9. Calculate the impactNormal.
    result.m_impactNormal = Vec2(SE.GetNormalized().y, -SE.GetNormalized().x);

    // 10. If RSj is positive, it means SE is in the opposite direction of jBasis,
    // and thus the impactNormal will also be in the opposite direction, so flip the impactNormal.
    if (RSj > 0.f)
    {
        result.m_impactNormal = -result.m_impactNormal;
    }

    result.m_didImpact      = true;
    result.m_impactPosition = impactPosition;
    result.m_impactLength   = impactLength;

    return result;
}

//----------------------------------------------------------------------------------------------------
RaycastResult2D RaycastVsAABB2D(Vec2 const& rayStartPosition,
                                Vec2 const& rayForwardNormal,
                                float const maxLength,
                                Vec2 const& aabb2Mins,
                                Vec2 const& aabb2Maxs)
{
    // 1. Initialize raycastResult2D.
    RaycastResult2D result;
    result.m_rayStartPosition = rayStartPosition;
    result.m_rayForwardNormal = rayForwardNormal;
    result.m_rayMaxLength     = maxLength;

    // 2. Calculate the rayEndPosition and the rayAABB2.
    Vec2 const  rayEndPosition = rayStartPosition + rayForwardNormal * maxLength;
    float const rayAABB2MinsX  = std::min(rayStartPosition.x, rayEndPosition.x);
    float const rayAABB2MinsY  = std::min(rayStartPosition.y, rayEndPosition.y);
    float const rayAABB2MaxsX  = std::max(rayStartPosition.x, rayEndPosition.x);
    float const rayAABB2MaxsY  = std::max(rayStartPosition.y, rayEndPosition.y);
    AABB2 const rayAABB2       = AABB2(Vec2(rayAABB2MinsX, rayAABB2MinsY), Vec2(rayAABB2MaxsX, rayAABB2MaxsY));

    // 3. If the rayAABB2 and the AABB2 are not overlapping, return with no hit.
    if (DoAABB2sOverlap2D(rayAABB2, AABB2(aabb2Mins, aabb2Maxs)) == false)
    {
        return result;
    }

    // 4. If the rayStartPosition is inside the AABB2, return hit.
    if (IsPointInsideAABB2D(rayStartPosition, aabb2Mins, aabb2Maxs) == true)
    {
        result.m_didImpact      = true;
        result.m_impactPosition = rayStartPosition;
        result.m_impactNormal   = -rayForwardNormal;
        result.m_impactLength   = 0.f;

        return result;
    }

    // 5. Calculate minXHitT, maxXHitT, minYHitT and maxYHitT.
    float const oneOverRayRangeX = 1.f / (rayEndPosition.x - rayStartPosition.x);
    float const oneOverRayRangeY = 1.f / (rayEndPosition.y - rayStartPosition.y);
    float const minXHitT         = (aabb2Mins.x - rayStartPosition.x) * oneOverRayRangeX;
    float const maxXHitT         = (aabb2Maxs.x - rayStartPosition.x) * oneOverRayRangeX;
    float const minYHitT         = (aabb2Mins.y - rayStartPosition.y) * oneOverRayRangeY;
    float const maxYHitT         = (aabb2Maxs.y - rayStartPosition.y) * oneOverRayRangeY;

    // 6. If the ray is vertical,
    if (rayForwardNormal.x == 0.f)
    {
        // 7. If the ray is outside the AABB2, return with no hit.
        if (rayStartPosition.x <= aabb2Mins.x ||
            rayStartPosition.x >= aabb2Maxs.x)
        {
            return result;
        }

        // RAY HIT BOTTOM
        if (minYHitT < maxYHitT)
        {
            result.m_didImpact      = true;
            result.m_impactLength   = maxLength * minYHitT;
            result.m_impactPosition = rayStartPosition + rayForwardNormal * result.m_impactLength;
            result.m_impactNormal   = Vec2(0.f, -1.f);

            return result;
        }

        // RAY HIT TOP
        if (minYHitT >= maxYHitT)
        {
            result.m_didImpact      = true;
            result.m_impactLength   = maxLength * maxYHitT;
            result.m_impactPosition = rayStartPosition + rayForwardNormal * result.m_impactLength;
            result.m_impactNormal   = Vec2(0.f, 1.f);

            return result;
        }

        return result;
    }

    // 8. If the ray is vertical,
    if (rayForwardNormal.y == 0.f)
    {
        // 9. If the ray is outside the AABB2, return with no hit.
        if (rayStartPosition.y <= aabb2Mins.y ||
            rayStartPosition.y >= aabb2Maxs.y)
        {
            return result;
        }

        // RAY HIT LEFT
        if (minXHitT < maxXHitT)
        {
            result.m_didImpact      = true;
            result.m_impactLength   = maxLength * minXHitT;
            result.m_impactPosition = rayStartPosition + rayForwardNormal * result.m_impactLength;
            result.m_impactNormal   = Vec2(-1.f, 0.f);

            return result;
        }

        // RAY HIT RIGHT
        if (minXHitT >= maxXHitT)
        {
            result.m_didImpact      = true;
            result.m_impactLength   = maxLength * maxXHitT;
            result.m_impactPosition = rayStartPosition + rayForwardNormal * result.m_impactLength;
            result.m_impactNormal   = Vec2(1.f, 0.f);

            return result;
        }

        return result;
    }

    // 10. Calculate enterXHitT, exitXHitT, enterYHitT, and exitYHitT.
    float const enterXHitT = std::min(minXHitT, maxXHitT);
    float const exitXHitT  = std::max(minXHitT, maxXHitT);
    float const enterYHitT = std::min(minYHitT, maxYHitT);
    float const exitYHitT  = std::max(minYHitT, maxYHitT);

    // 11. If the ray enters X before entering Y,
    if (enterXHitT < enterYHitT)
    {
        // 12. If the ray exits X before entering Y, return with no hit.
        if (exitXHitT < enterYHitT)
        {
            return result;
        }

        // 13. If the ray enters Y before the rayStartPosition or the ray enters X after the ray's maxLength,
        // return with no hit.
        if (enterYHitT <= 0.f ||
            enterXHitT >= 1.f)
        {
            return result;
        }

        // 14. If the ray's enterYHitT is AABB2's minYHitT.
        if (fabs(enterYHitT - minYHitT) < FLOAT_MIN)
        {
            // RAY HIT BOTTOM
            result.m_didImpact      = true;
            result.m_impactLength   = maxLength * enterYHitT;
            result.m_impactPosition = rayStartPosition + rayForwardNormal * result.m_impactLength;
            result.m_impactNormal   = Vec2(0.f, -1.f);

            return result;
        }

        // RAY HIT TOP
        result.m_didImpact      = true;
        result.m_impactLength   = maxLength * enterYHitT;
        result.m_impactPosition = rayStartPosition + rayForwardNormal * result.m_impactLength;
        result.m_impactNormal   = Vec2(0.f, 1.f);

        return result;
    }

    // 15. If the ray enters Y before entering X,
    if (enterYHitT < enterXHitT)
    {
        // 16. If the ray exits Y before entering X, return with no hit.
        if (exitYHitT < enterXHitT)
        {
            return result;
        }

        // 17. If the ray enters X before the rayStartPosition or the ray enters Y after the ray's maxLength,
        // return with no hit.
        if (enterXHitT <= 0.f ||
            enterYHitT >= 1.f)
        {
            return result;
        }

        // 18. If the ray's enterXHitT is AABB2's minXHitT.
        if (fabs(enterXHitT - minXHitT) < FLOAT_MIN)
        {
            // RAY HIT LEFT
            result.m_didImpact      = true;
            result.m_impactLength   = maxLength * enterXHitT;
            result.m_impactPosition = rayStartPosition + rayForwardNormal * result.m_impactLength;
            result.m_impactNormal   = Vec2(-1.f, 0.f);

            return result;
        }

        // RAY HIT RIGHT
        result.m_didImpact      = true;
        result.m_impactLength   = maxLength * enterXHitT;
        result.m_impactPosition = rayStartPosition + rayForwardNormal * result.m_impactLength;
        result.m_impactNormal   = Vec2(1.f, 0.f);
    }

    // ERROR_RECOVERABLE("The input is not handled by RaycastVsAABB2D.")
    return result;
}

//----------------------------------------------------------------------------------------------------
RaycastResult3D RaycastVsSphere3D(Vec3 const& rayStartPosition,
                                  Vec3 const& rayForwardNormal,
                                  float const maxLength,
                                  Vec3 const& sphereCenter,
                                  float const sphereRadius)
{
    // 1. Initialize raycastResult3D.
    RaycastResult3D result;
    result.m_rayStartPosition = rayStartPosition;
    result.m_rayForwardNormal = rayForwardNormal;
    result.m_rayMaxLength     = maxLength;

    // 2. Calculate the startToCenterDirection ( SC ), kBasis, jBasis, and SCj.
    Vec3 const  SC     = sphereCenter - rayStartPosition;
    Vec3 const  kBasis = CrossProduct3D(rayForwardNormal, SC).GetNormalized();
    Vec3 const  jBasis = CrossProduct3D(kBasis, rayForwardNormal).GetNormalized();
    float const SCj    = GetProjectedLength3D(SC, jBasis);

    // 3. If SCj is too far from the sphere (outside the sphere along the ray), return with initial result.
    if (SCj >= sphereRadius ||
        SCj <= -sphereRadius)
    {
        return result;
    }

    // 4. Calculate SCi.
    float const SCi = GetProjectedLength3D(SC, rayForwardNormal);

    // 5. If SCi is not intersecting with the disc, return with initial raycastResult2D.
    if (SCi < -sphereRadius ||
        SCi > maxLength + sphereRadius)
    {
        return result;
    }

    // 6. Calculate the adjustedLength and the impactDistance.
    float const adjustedLength = sqrtf(sphereRadius * sphereRadius - SCj * SCj);
    result.m_impactLength      = SCi - adjustedLength;

    // 7. If the ray hits the disc before the rayStartPosition or after the ray's maxLength,
    // return with no hit. (In steps 5, there are some rare cases where they might pass the checkpoints.)
    if (result.m_impactLength < 0 ||
        result.m_impactLength > maxLength)
    {
        return result;
    }

    // RAY HIT
    result.m_didImpact      = true;
    result.m_impactPosition = rayStartPosition + rayForwardNormal * result.m_impactLength;
    result.m_impactNormal   = (result.m_impactPosition - sphereCenter).GetNormalized();

    return result;
}

//----------------------------------------------------------------------------------------------------
RaycastResult3D RaycastVsAABB3D(Vec3 const& rayStartPosition,
                                Vec3 const& rayForwardNormal,
                                float const maxLength,
                                Vec3 const& aabb3Mins,
                                Vec3 const& aabb3Maxs)
{
    // 1. Initialize raycastResult3D.
    RaycastResult3D result;
    result.m_rayStartPosition = rayStartPosition;
    result.m_rayForwardNormal = rayForwardNormal;
    result.m_rayMaxLength     = maxLength;

    // 2. Calculate the rayEndPosition and the rayAABB3.
    Vec3  rayEndPosition = rayStartPosition + rayForwardNormal * maxLength;
    float rayAABB3MinsX  = std::min(rayStartPosition.x, rayEndPosition.x);
    float rayAABB3MinsY  = std::min(rayStartPosition.y, rayEndPosition.y);
    float rayAABB3MinsZ  = std::min(rayStartPosition.z, rayEndPosition.z);
    float rayAABB3MaxsX  = std::max(rayStartPosition.x, rayEndPosition.x);
    float rayAABB3MaxsY  = std::max(rayStartPosition.y, rayEndPosition.y);
    float rayAABB3MaxsZ  = std::max(rayStartPosition.z, rayEndPosition.z);
    AABB3 rayAABB3       = AABB3(Vec3(rayAABB3MinsX, rayAABB3MinsY, rayAABB3MinsZ), Vec3(rayAABB3MaxsX, rayAABB3MaxsY, rayAABB3MaxsZ));

    // 3. If the rayAABB3 and the AABB3 are not overlapping, return with no hit.
    if (DoAABB3sOverlap3D(rayAABB3, AABB3(aabb3Mins, aabb3Maxs)) == false)
    {
        return result;
    }

    // 4. If the rayStartPosition is inside the AABB3, return hit.
    if (IsPointInsideAABB3D(rayStartPosition, aabb3Mins, aabb3Maxs) == true)
    {
        result.m_didImpact      = true;
        result.m_impactPosition = rayStartPosition;
        result.m_impactNormal   = -rayForwardNormal;
        result.m_impactLength   = 0.f;

        return result;
    }

    // 5. If rayForwardNormalX is zero ( rayYZ ),
    // and the rayYZ is outside the AABB3's X hit zone, return with no hit.
    if (rayForwardNormal.x == 0.f)
    {
        if (rayStartPosition.x <= aabb3Mins.x ||
            rayStartPosition.x >= aabb3Maxs.x)
        {
            return result;
        }
    }

    // 6. If rayForwardNormalY is zero ( rayXZ ),
    // and the rayXZ is outside the AABB3's Y hit zone, return with no hit.
    if (rayForwardNormal.y == 0.f)
    {
        if (rayStartPosition.y <= aabb3Mins.y ||
            rayStartPosition.y >= aabb3Maxs.y)
        {
            return result;
        }
    }

    // 7. If rayForwardNormalZ is zero ( rayXY ),
    // and the rayXY is outside the AABB3's Z hit zone, return with no hit.
    if (rayForwardNormal.z == 0.f)
    {
        if (rayStartPosition.z <= aabb3Mins.z ||
            rayStartPosition.z >= aabb3Maxs.z)
        {
            return result;
        }
    }

    // 8. Initialize hitType, hitTypeX, hitTypeY, and hitTypeZ;
    // Calculate minXHitT, maxXHitT, minYHitT, maxYHitT, minZHitT and maxZHitT.
    eAABB3HitType hitType          = eAABB3HitType::NONE;
    eAABB3HitType hitTypeX         = eAABB3HitType::NONE;
    eAABB3HitType hitTypeY         = eAABB3HitType::NONE;
    eAABB3HitType hitTypeZ         = eAABB3HitType::NONE;
    float         oneOverRayRangeX = 1.f / (rayEndPosition.x - rayStartPosition.x);
    float         oneOverRayRangeY = 1.f / (rayEndPosition.y - rayStartPosition.y);
    float         oneOverRayRangeZ = 1.f / (rayEndPosition.z - rayStartPosition.z);
    float         minXHitT         = (aabb3Mins.x - rayStartPosition.x) * oneOverRayRangeX;
    float         maxXHitT         = (aabb3Maxs.x - rayStartPosition.x) * oneOverRayRangeX;
    float         minYHitT         = (aabb3Mins.y - rayStartPosition.y) * oneOverRayRangeY;
    float         maxYHitT         = (aabb3Maxs.y - rayStartPosition.y) * oneOverRayRangeY;
    float         minZHitT         = (aabb3Mins.z - rayStartPosition.z) * oneOverRayRangeZ;
    float         maxZHitT         = (aabb3Maxs.z - rayStartPosition.z) * oneOverRayRangeZ;

    // 9. Initialize enterXHitT, exitXHitT, enterYHitT, exitYHitT, enterZHitT and exitZHitT.
    float enterXHitT = -FLOAT_MAX;
    float exitXHitT  = FLOAT_MAX;
    float enterYHitT = -FLOAT_MAX;
    float exitYHitT  = FLOAT_MAX;
    float enterZHitT = -FLOAT_MAX;
    float exitZHitT  = FLOAT_MAX;

    if (rayForwardNormal.x != 0.f)
    {
        enterXHitT = std::min(minXHitT, maxXHitT);
        exitXHitT  = std::max(minXHitT, maxXHitT);
        hitTypeX   = minXHitT < maxXHitT ? eAABB3HitType::BACK : eAABB3HitType::FRONT;
    }

    if (rayForwardNormal.y != 0.f)
    {
        enterYHitT = std::min(minYHitT, maxYHitT);
        exitYHitT  = std::max(minYHitT, maxYHitT);
        hitTypeY   = minYHitT < maxYHitT ? eAABB3HitType::LEFT : eAABB3HitType::RIGHT;
    }

    float enterXYHitT, exitXYHitT;

    if (enterYHitT < exitXHitT &&
        enterXHitT < exitYHitT)
    {
        enterXYHitT = std::max(enterXHitT, enterYHitT);
        exitXYHitT  = std::min(exitXHitT, exitYHitT);
        hitType     = enterXHitT < enterYHitT ? hitTypeY : hitTypeX;
    }
    else
    {
        return result;
    }

    if (rayForwardNormal.z != 0.f)
    {
        enterZHitT = std::min(minZHitT, maxZHitT);
        exitZHitT  = std::max(minZHitT, maxZHitT);
        hitTypeZ   = minZHitT < maxZHitT ? eAABB3HitType::BOTTOM : eAABB3HitType::TOP;
    }

    float enterXYZHitT;

    if (exitXYHitT > enterZHitT &&
        exitZHitT > enterXYHitT)
    {
        enterXYZHitT = std::max(enterZHitT, enterXYHitT);
        hitType      = enterZHitT < enterXYHitT ? hitType : hitTypeZ;
    }
    else
    {
        return result;
    }

    switch (hitType)
    {
    case eAABB3HitType::BACK: result.m_impactNormal = -Vec3::X_BASIS;
        break;
    case eAABB3HitType::FRONT: result.m_impactNormal = Vec3::X_BASIS;
        break;
    case eAABB3HitType::LEFT: result.m_impactNormal = -Vec3::Y_BASIS;
        break;
    case eAABB3HitType::RIGHT: result.m_impactNormal = Vec3::Y_BASIS;
        break;
    case eAABB3HitType::BOTTOM: result.m_impactNormal = -Vec3::Z_BASIS;
        break;
    case eAABB3HitType::TOP: result.m_impactNormal = Vec3::Z_BASIS;
        break;
    case eAABB3HitType::NONE: ERROR_RECOVERABLE("The input is not handled by RaycastVsAABB3D.")
        break;
    }

    result.m_didImpact      = true;
    result.m_impactLength   = enterXYZHitT * maxLength;
    result.m_impactPosition = rayStartPosition + rayForwardNormal * result.m_impactLength;

    return result;
}

//----------------------------------------------------------------------------------------------------
RaycastResult3D RaycastVsCylinderZ3D(Vec3 const&       rayStartPosition,
                                     Vec3 const&       rayForwardNormal,
                                     float const       maxLength,
                                     Vec2 const&       cylinderCenterXY,
                                     FloatRange const& cylinderMinMaxZ,
                                     float const       cylinderRadius)
{
    // 1. Initialize raycastResult3D.
    RaycastResult3D result;
    result.m_rayStartPosition = rayStartPosition;
    result.m_rayForwardNormal = rayForwardNormal;
    result.m_rayMaxLength     = maxLength;

    // 2. Declare rayMaxLengthZ and impactPosition.
    float rayMaxLengthZ;
    Vec3  impactPosition;

    // 3. If rayStartPositionZ is larger than cylinder's maxZ,
    if (rayStartPosition.z >= cylinderMinMaxZ.m_max)
    {
        // 4. If the ray goes down ( ray goes up does not hit ),
        if (rayForwardNormal.z < 0)
        {
            // 5. Calculate the rayMaxLengthZ and impactPosition
            rayMaxLengthZ  = (cylinderMinMaxZ.m_max - rayStartPosition.z) / rayForwardNormal.z;
            impactPosition = rayStartPosition + rayForwardNormal * rayMaxLengthZ;

            // 6. If impactPosition is inside the cylinderDisc, return with hit.
            if (IsPointInsideDisc2D(Vec2(impactPosition.x, impactPosition.y), cylinderCenterXY, cylinderRadius))
            {
                // RAY HIT TOP
                result.m_didImpact      = true;
                result.m_impactPosition = impactPosition;
                result.m_impactNormal   = Vec3::Z_BASIS;
                result.m_impactLength   = rayMaxLengthZ;

                return result;
            }
        }
    }

    // 7. If rayStartPositionZ is smaller than cylinder's minZ,
    if (rayStartPosition.z <= cylinderMinMaxZ.m_min)
    {
        // 8. If the ray goes up ( ray goes down does not hit ),
        if (rayForwardNormal.z > 0)
        {
            // 9. Calculate the rayMaxLengthZ and impactPosition
            rayMaxLengthZ  = (cylinderMinMaxZ.m_min - rayStartPosition.z) / rayForwardNormal.z;
            impactPosition = rayStartPosition + rayForwardNormal * rayMaxLengthZ;

            // 10. If impactPosition is inside the cylinderDisc, return with hit.
            if (IsPointInsideDisc2D(Vec2(impactPosition.x, impactPosition.y), cylinderCenterXY, cylinderRadius))
            {
                // RAY HIT BOTTOM
                result.m_didImpact      = true;
                result.m_impactPosition = impactPosition;
                result.m_impactNormal   = -Vec3::Z_BASIS;
                result.m_impactLength   = rayMaxLengthZ;

                return result;
            }
        }
    }

    // 11. If rayStartPositionZ is within the cylinder's minZ and maxZ,
    if (cylinderMinMaxZ.IsOnRange(rayStartPosition.z) == true)
    {
        // 12. If the ray starts inside the cylinderDisc, return with hit.
        if (IsPointInsideDisc2D(Vec2(rayStartPosition.x, rayStartPosition.y), cylinderCenterXY, cylinderRadius))
        {
            result.m_didImpact      = true;
            result.m_impactPosition = rayStartPosition;
            result.m_impactNormal   = -rayForwardNormal;
            result.m_impactLength   = 0.f;

            return result;
        }
    }

    // 13. Handle the case where the ray is with the cylinder's minZ and maxZ,
    // but starts outside the cylinderDisc;
    // Calculate the rayForwardNormalXY2D and rayForwardNormalXY2DLength,
    // in order to get the proportion of the ray's on XY surface.
    Vec2 const  rayForwardNormalXY2D       = Vec2(rayForwardNormal.x, rayForwardNormal.y);
    float const rayForwardNormalXY2DLength = rayForwardNormalXY2D.GetLength();

    // 14. If the ray only has Z direction, return with no hit.
    if (rayForwardNormalXY2DLength == 0.f)
    {
        return result;
    }

    // 15. Calculate the raycastResult2D with ray and cylinder both projected on XY surface.
    // (Notice the variables are adjusted correctly.)
    RaycastResult2D const result2D = RaycastVsDisc2D(Vec2(rayStartPosition.x, rayStartPosition.y),
                                                     rayForwardNormalXY2D / rayForwardNormalXY2DLength,
                                                     maxLength * rayForwardNormalXY2DLength,
                                                     cylinderCenterXY,
                                                     cylinderRadius);

    // 16. If the ray2D does not hit, return with no hit.
    if (result2D.m_didImpact == false)
    {
        return result;
    }

    // 17. Calculate the rayImpactLength and impactPosition.
    float const rayImpactLength = result2D.m_impactLength / rayForwardNormalXY2DLength;
    impactPosition              = Vec3(result2D.m_impactPosition.x, result2D.m_impactPosition.y, rayStartPosition.z + rayImpactLength * rayForwardNormal.z);

    // 18. If the ray impactPositionZ is outside cylinderMinZ and cylinderMaxZ, return with no hit.
    if (cylinderMinMaxZ.IsOnRange(impactPosition.z) == false)
    {
        return result;
    }

    // RAY HIT SIDE
    result.m_didImpact      = true;
    result.m_impactPosition = impactPosition;
    result.m_impactNormal   = Vec3(result2D.m_impactNormal.x, result2D.m_impactNormal.y, 0.f);
    result.m_impactLength   = rayImpactLength;

    return result;
}

//----------------------------------------------------------------------------------------------------
RaycastResult3D RaycastVsOBB3D(Vec3 const& rayStartPosition,
                               Vec3 const& rayForwardNormal,
                               float const maxLength,
                               OBB3 const& obb3)
{
    // 1. Initialize raycastResult3D.
    RaycastResult3D result;
    result.m_rayStartPosition = rayStartPosition;
    result.m_rayForwardNormal = rayForwardNormal;
    result.m_rayMaxLength     = maxLength;

    Mat44 obb3Matrix(obb3.m_iBasis, obb3.m_jBasis, obb3.m_kBasis, obb3.m_center);
    Mat44 transformMatrix = obb3Matrix.GetOrthonormalInverse();
    AABB3 localAABB3(-obb3.m_halfDimensions, obb3.m_halfDimensions);
    Vec3  newStartPos      = transformMatrix.TransformPosition3D(rayStartPosition);
    Vec3  newForwardNormal = transformMatrix.TransformVectorQuantity3D(rayForwardNormal);

    result = RaycastVsAABB3D(newStartPos, newForwardNormal, maxLength, localAABB3.m_mins, localAABB3.m_maxs);

    if (!result.m_didImpact)
    {
        return result;
    }

    result.m_impactNormal     = obb3Matrix.TransformVectorQuantity3D(result.m_impactNormal);
    result.m_impactPosition   = obb3Matrix.TransformPosition3D(result.m_impactPosition);
    result.m_rayForwardNormal = rayForwardNormal;
    result.m_rayStartPosition = rayStartPosition;
    return result;
}

//----------------------------------------------------------------------------------------------------
RaycastResult3D RaycastVsPlane3D(Vec3 const&   rayStartPosition,
                                 Vec3 const&   rayForwardNormal,
                                 float const   maxLength,
                                 Plane3 const& plane3)
{
    RaycastResult3D result;
    result.m_rayStartPosition = rayStartPosition;
    result.m_rayForwardNormal = rayForwardNormal;
    result.m_rayMaxLength     = maxLength;

    Vec3 const rayEndPosition = rayStartPosition + rayForwardNormal * maxLength;

    float startPosAltitude = plane3.GetAltitudeOfPoint(rayStartPosition);
    float endPosAltitude   = plane3.GetAltitudeOfPoint(rayEndPosition);

    // all on one side of the plane
    if ((startPosAltitude <= 0.f && endPosAltitude <= 0.f) || (startPosAltitude >= 0.f && endPosAltitude >= 0.f))
    {
        return result;
    }

    // start from the back side of the plane, so hit the back side and the normal is negative
    if (startPosAltitude < 0.f)
    {
        result.m_impactNormal = -plane3.m_normal;
    }
    else
    {
        result.m_impactNormal = plane3.m_normal;
    }

    startPosAltitude = abs(startPosAltitude);
    endPosAltitude   = abs(endPosAltitude);

    result.m_didImpact      = true;
    result.m_impactLength   = maxLength * (startPosAltitude / (startPosAltitude + endPosAltitude));
    result.m_impactPosition = rayStartPosition + rayForwardNormal * result.m_impactLength;

    result.m_rayForwardNormal = rayForwardNormal;
    result.m_rayMaxLength     = maxLength;
    result.m_rayStartPosition = rayStartPosition;

    return result;
}

//----------------------------------------------------------------------------------------------------
RaycastResult2D RaycastVsConvexHull2D(Vec2 const& rayStartPosition, Vec2 const& rayForwardNormal, float maxLength, ConvexHull2 const& convexHull)
{
    RaycastResult2D result;
    result.m_rayStartPosition = rayStartPosition;
    result.m_rayForwardNormal = rayForwardNormal;
    result.m_rayMaxLength     = maxLength;

    // Use slab method: track entry and exit distances
    float tEnter = 0.f;
    float tExit  = maxLength;
    Vec2  enterNormal;

    for (Plane2 const& plane : convexHull.m_boundingPlanes)
    {
        float vd = DotProduct2D(rayForwardNormal, plane.m_normal);
        float v0 = plane.GetAltitudeOfPoint(rayStartPosition);

        // Ray parallel to plane
        if (abs(vd) < 0.0001f)
        {
            // If outside, no intersection
            if (v0 > 0.f)
            {
                return result; // Miss
            }
            continue;
        }

        float t = -v0 / vd;

        // Entering the half-space (moving towards inside)
        if (vd < 0.f)
        {
            if (t > tEnter)
            {
                tEnter      = t;
                enterNormal = plane.m_normal;
            }
        }
        // Exiting the half-space (moving towards outside)
        else
        {
            if (t < tExit)
            {
                tExit = t;
            }
        }

        // Early exit if no intersection possible
        if (tEnter > tExit)
        {
            return result; // Miss
        }
    }

    // Check if intersection is within ray bounds
    if (tEnter < 0.f || tEnter > maxLength)
    {
        return result; // Miss
    }

    // Hit!
    result.m_didImpact      = true;
    result.m_impactLength   = tEnter;
    result.m_impactPosition = rayStartPosition + rayForwardNormal * tEnter;
    result.m_impactNormal   = enterNormal;

    return result;
}
