//----------------------------------------------------------------------------------------------------
// RaycastUtils.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Math/RaycastUtils.hpp"

#include <algorithm>
#include <cmath>

#include "Disc2.hpp"
#include "FloatRange.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Math/MathUtils.hpp"

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

    // 6. Calculate the adjustedLength and the impactDistance
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
    Vec2  rayEndPosition = rayStartPosition + rayForwardNormal * maxLength;
    float rayAABB2MinsX  = std::min(rayStartPosition.x, rayEndPosition.x);
    float rayAABB2MinsY  = std::min(rayStartPosition.y, rayEndPosition.y);
    float rayAABB2MaxsX  = std::max(rayStartPosition.x, rayEndPosition.x);
    float rayAABB2MaxsY  = std::max(rayStartPosition.y, rayEndPosition.y);
    AABB2 rayAABB2       = AABB2(Vec2(rayAABB2MinsX, rayAABB2MinsY), Vec2(rayAABB2MaxsX, rayAABB2MaxsY));

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
    float oneOverRayRangeX = 1.f / (rayEndPosition.x - rayStartPosition.x);
    float oneOverRayRangeY = 1.f / (rayEndPosition.y - rayStartPosition.y);
    float minXHitT         = (aabb2Mins.x - rayStartPosition.x) * oneOverRayRangeX;
    float maxXHitT         = (aabb2Maxs.x - rayStartPosition.x) * oneOverRayRangeX;
    float minYHitT         = (aabb2Mins.y - rayStartPosition.y) * oneOverRayRangeY;
    float maxYHitT         = (aabb2Maxs.y - rayStartPosition.y) * oneOverRayRangeY;

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
        if (minYHitT > maxYHitT)
        {
            result.m_didImpact      = true;
            result.m_impactLength   = maxLength * maxYHitT;
            result.m_impactPosition = rayStartPosition + rayForwardNormal * result.m_impactLength;
            result.m_impactNormal   = Vec2(0.f, 1.f);

            return result;
        }

        // not hit
        result.m_didImpact = false;

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
        if (maxXHitT <= minXHitT)
        {
            result.m_didImpact      = true;
            result.m_impactLength   = maxLength * maxXHitT;
            result.m_impactPosition = rayStartPosition + rayForwardNormal * result.m_impactLength;
            result.m_impactNormal   = Vec2(1.f, 0.f);

            return result;
        }

        // not hit
        result.m_didImpact = false;
        return result;
    }

    // 10. Calculate enterXHitT, exitXHitT, enterYHitT, and exitYHitT.
    float enterXHitT = std::min(minXHitT, maxXHitT);
    float exitXHitT  = std::max(minXHitT, maxXHitT);
    float enterYHitT = std::min(minYHitT, maxYHitT);
    float exitYHitT  = std::max(minYHitT, maxYHitT);

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

    ERROR_RECOVERABLE("The input is not handled by RaycastVsAABB2D.")
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

    // 2. Calculate the startToCenterDirection ( SC ) and SCi (projection of SC onto rayForwardNormal).
    Vec3 const SC = sphereCenter - rayStartPosition;
    Vec3 kBasis = CrossProduct3D( rayForwardNormal, SC );
    Vec3 jBasis = CrossProduct3D( kBasis, rayForwardNormal ).GetNormalized();

    float const SCj = GetProjectedLength3D(SC, jBasis);

    // 3. If SCi is too far from the sphere (outside the sphere along the ray), return with initial result.
    if (SCj >= sphereRadius ||
        SCj <= -sphereRadius)
    {
        return result;
    }

    float const SCi = GetProjectedLength3D(SC, rayForwardNormal);

    if (SCi < -sphereRadius ||
            SCi > maxLength + sphereRadius)
    {
        return result;
    }

    float const adjustedLength = sqrtf(sphereRadius * sphereRadius - SCj * SCj);
    result.m_impactLength      = SCi - adjustedLength;

    // 7. If the ray hits the disc before the rayStartPosition or after the ray's maxLength,
    // return with no hit. (In steps 5, there are some rare cases where they might pass the checkpoints.)
    if (result.m_impactLength < 0 ||
        result.m_impactLength > maxLength)
    {
        return result;
    }

    result.m_didImpact      = true;
    result.m_impactPosition = rayStartPosition + rayForwardNormal * result.m_impactLength;
    result.m_impactNormal   = (result.m_impactPosition - sphereCenter).GetNormalized();

    return result;

    // // 1. Initialize raycastResult3D.
    // RaycastResult3D result;
    // result.m_rayForwardNormal = rayForwardNormal;
    // result.m_rayStartPosition = rayStartPosition;
    // result.m_rayMaxLength     = maxLength;
    //
    // Vec3 SC = sphereCenter - rayStartPosition;
    // //Vec3 upVector = CrossProduct3D( forwardNormal, SC );
    // //Vec3 leftNormal = CrossProduct3D( upVector, forwardNormal ).GetNormalized();
    // float SCi        = DotProduct3D(SC, rayForwardNormal);
    // Vec3  leftVector = (SC - rayForwardNormal * SCi);
    //
    // // if (leftVector == Vec3( 0.f, 0.f, 0.f )) {
    // //     // ray go through the center of the sphere
    // //     float impactLength = SC.GetLength() - sphereRadius;
    // //     // start point inside disc, hit when start
    // //     if (impactLength > -2 * sphereRadius && impactLength < 0) {
    // //         result.m_didImpact = true;
    // //         result.m_impactLength = 0.f;
    // //         result.m_impactNormal = -rayForwardNormal;
    // //         result.m_impactPosition = rayStartPosition;
    // //
    // //
    // //
    // //         return result;
    // //     }
    // //     // length is short or direction is reversed, cannot get to disc
    // //     if (impactLength <= 0.f || impactLength >= maxLength) {
    // //         return result;
    // //     }
    // //     Vec3 impactPos = impactLength * rayForwardNormal + rayStartPosition;
    // //     result.m_didImpact = true;
    // //     result.m_impactLength      = impactLength;
    // //     result.m_impactNormal = (impactPos - sphereCenter) / sphereRadius;
    // //     result.m_impactPosition = impactPos;
    // //
    // //     return result;
    // // }
    //
    // float squaredSCj = leftVector.GetLengthSquared();
    // float squaredA   = sphereRadius * sphereRadius - squaredSCj;
    // if (squaredA <= 0.f)
    // {
    //     return result;
    // }
    //
    // float a            = sqrtf(squaredA);
    // float impactLength = SCi - a;
    //
    // // start point inside disc, hit when start
    // if (impactLength > -2 * a && impactLength < 0)
    // {
    //     result.m_didImpact        = true;
    //     result.m_impactLength     = 0.f;
    //     result.m_impactNormal     = -rayForwardNormal;
    //     result.m_impactPosition   = rayStartPosition;
    //     result.m_rayForwardNormal = rayForwardNormal;
    //     result.m_rayMaxLength     = maxLength;
    //     result.m_rayStartPosition = rayStartPosition;
    //     return result;
    // }
    // // length is short or direction is reversed, cannot get to disc
    // if (impactLength <= 0.f || impactLength >= maxLength)
    // {
    //     return result;
    // }
    // // hit the disc
    // Vec3 impactPos            = impactLength * rayForwardNormal + rayStartPosition;
    // result.m_didImpact        = true;
    // result.m_impactLength     = impactLength;
    // result.m_impactNormal     = (impactPos - sphereCenter) / sphereRadius;
    // result.m_impactPosition   = impactPos;
    // result.m_rayForwardNormal = rayForwardNormal;
    // result.m_rayMaxLength     = maxLength;
    // result.m_rayStartPosition = rayStartPosition;
    // return result;
}

//----------------------------------------------------------------------------------------------------
RaycastResult3D RaycastVsAABB3D(Vec3 const&  rayStartPosition,
                                Vec3 const&  rayForwardNormal,
                                float const  maxLength,
                                AABB3 const& box)
{
    // 1. Initialize raycastResult3D.
    RaycastResult3D result;
    result.m_rayForwardNormal = rayForwardNormal;
    result.m_rayStartPosition = rayStartPosition;
    result.m_rayMaxLength     = maxLength;

    Vec3 endPos = rayStartPosition + rayForwardNormal * maxLength;
    if (!DoAABB3sOverlap3D(AABB3(Vec3(std::min(rayStartPosition.x, endPos.x), std::min(rayStartPosition.y, endPos.y), std::min(rayStartPosition.z, endPos.z)), Vec3(std::max(rayStartPosition.x, endPos.x), std::max(rayStartPosition.y, endPos.y), std::max(rayStartPosition.z, endPos.z))), box))
    {
        result.m_didImpact = false;
        return result;
    }

    if (box.IsPointInside(rayStartPosition))
    {
        result.m_didImpact        = true;
        result.m_impactLength     = 0.f;
        result.m_impactNormal     = -rayForwardNormal;
        result.m_impactPosition   = rayStartPosition;
        result.m_rayForwardNormal = rayForwardNormal;
        result.m_rayMaxLength     = maxLength;
        result.m_rayStartPosition = rayStartPosition;
        return result;
    }

    // 0 -x 1 +x 2 -y 3 +y 4 -z 5 +z
    int   hitFace       = -1;
    int   hitFaceX      = -1, hitFaceY = -1, hitFaceZ = -1;
    float oneOverRangeX = 1.f / (endPos.x - rayStartPosition.x);
    float minXHitT      = (box.m_mins.x - rayStartPosition.x) * oneOverRangeX;
    float maxXHitT      = (box.m_maxs.x - rayStartPosition.x) * oneOverRangeX;
    float firstXHitT, secondXHitT;
    if (rayForwardNormal.x == 0.f)
    {
        if (rayStartPosition.x > box.m_mins.x && rayStartPosition.x < box.m_maxs.x)
        {
            firstXHitT  = -FLOAT_MAX;
            secondXHitT = FLOAT_MAX;
        }
        else
        {
            return result;
        }
    }
    else if (minXHitT < maxXHitT)
    {
        firstXHitT  = minXHitT;
        secondXHitT = maxXHitT;
        hitFaceX    = 0;
    }
    else
    {
        firstXHitT  = maxXHitT;
        secondXHitT = minXHitT;
        hitFaceX    = 1;
    }

    float oneOverRangeY = 1.f / (endPos.y - rayStartPosition.y);
    float minYHitT      = (box.m_mins.y - rayStartPosition.y) * oneOverRangeY;
    float maxYHitT      = (box.m_maxs.y - rayStartPosition.y) * oneOverRangeY;
    float firstYHitT, secondYHitT;

    if (rayForwardNormal.y == 0.f)
    {
        if (rayStartPosition.y > box.m_mins.y && rayStartPosition.y < box.m_maxs.y)
        {
            firstYHitT  = -FLOAT_MAX;
            secondYHitT = FLOAT_MAX;
        }
        else
        {
            return result;
        }
    }
    else if (minYHitT < maxYHitT)
    {
        firstYHitT  = minYHitT;
        secondYHitT = maxYHitT;
        hitFaceY    = 2;
    }
    else
    {
        firstYHitT  = maxYHitT;
        secondYHitT = minYHitT;
        hitFaceY    = 3;
    }

    float firstXYInt, secondXYInt;
    if (secondXHitT > firstYHitT && secondYHitT > firstXHitT)
    {
        if (firstXHitT < firstYHitT)
        {
            firstXYInt = firstYHitT;
            hitFace    = hitFaceY;
        }
        else
        {
            firstXYInt = firstXHitT;
            hitFace    = hitFaceX;
        }

        if (secondXHitT < secondYHitT)
        {
            secondXYInt = secondXHitT;
        }
        else
        {
            secondXYInt = secondYHitT;
        }
    }
    else
    {
        result.m_didImpact = false;
        return result;
    }

    float oneOverRangeZ = 1.f / (endPos.z - rayStartPosition.z);
    float minZHitT      = (box.m_mins.z - rayStartPosition.z) * oneOverRangeZ;
    float maxZHitT      = (box.m_maxs.z - rayStartPosition.z) * oneOverRangeZ;
    float firstZHitT, secondZHitT;

    if (rayForwardNormal.z == 0.f)
    {
        if (rayStartPosition.z > box.m_mins.z && rayStartPosition.z < box.m_maxs.z)
        {
            firstZHitT  = -FLOAT_MAX;
            secondZHitT = FLOAT_MAX;
        }
        else
        {
            return result;
        }
    }
    else if (minZHitT < maxZHitT)
    {
        firstZHitT  = minZHitT;
        secondZHitT = maxZHitT;
        hitFaceZ    = 4;
    }
    else
    {
        firstZHitT  = maxZHitT;
        secondZHitT = minZHitT;
        hitFaceZ    = 5;
    }

    float firstXYZInt;
    if (secondXYInt > firstZHitT && secondZHitT > firstXYInt)
    {
        if (firstXYInt > firstZHitT)
        {
            firstXYZInt = firstXYInt;
        }
        else
        {
            firstXYZInt = firstZHitT;
            hitFace     = hitFaceZ;
        }
    }
    else
    {
        result.m_didImpact = false;
        return result;
    }
    result.m_didImpact    = true;
    result.m_impactLength = firstXYZInt * maxLength;

    switch (hitFace)
    {
    case 0: result.m_impactNormal = Vec3(-1.f, 0.f, 0.f);
        break;
    case 1: result.m_impactNormal = Vec3(1.f, 0.f, 0.f);
        break;
    case 2: result.m_impactNormal = Vec3(0.f, -1.f, 0.f);
        break;
    case 3: result.m_impactNormal = Vec3(0.f, 1.f, 0.f);
        break;
    case 4: result.m_impactNormal = Vec3(0.f, 0.f, -1.f);
        break;
    case 5: result.m_impactNormal = Vec3(0.f, 0.f, 1.f);
        break;
    }
    result.m_impactPosition   = rayStartPosition + rayForwardNormal * result.m_impactLength;
    result.m_rayForwardNormal = rayForwardNormal;
    result.m_rayMaxLength     = maxLength;
    result.m_rayStartPosition = rayStartPosition;

    return result;
}

//----------------------------------------------------------------------------------------------------
RaycastResult3D RaycastVsCylinderZ3D(Vec3 const& rayStartPosition,
                                     Vec3 const& rayForwardNormal,
                                     float const maxLength,
                                     Vec2 const& cylinderCenterXY,
                                     FloatRange  cylinderMinMaxZ,
                                     float const cylinderRadius)
{
    // 1. Initialize raycastResult3D.
    RaycastResult3D result;
    result.m_rayForwardNormal = rayForwardNormal;
    result.m_rayStartPosition = rayStartPosition;
    result.m_rayMaxLength     = maxLength;

    float distanceTravel = 0.f;
    Vec3  hitPos;
    result.m_didImpact = false;

    // check if hit top or bottom
    if (rayStartPosition.z >= cylinderMinMaxZ.m_max)
    {
        // ray goes up do not hit
        if (rayForwardNormal.z < 0)
        {
            distanceTravel = (cylinderMinMaxZ.m_max - rayStartPosition.z) / rayForwardNormal.z;
            hitPos         = rayStartPosition + rayForwardNormal * distanceTravel;
            if (distanceTravel < maxLength && IsPointInsideDisc2D(Vec2(hitPos.x, hitPos.y), cylinderCenterXY, cylinderRadius))
            {
                // hit
                result.m_didImpact        = true;
                result.m_impactLength     = distanceTravel;
                result.m_impactNormal     = Vec3(0.f, 0.f, 1.f);
                result.m_impactPosition   = hitPos;
                result.m_rayForwardNormal = rayForwardNormal;
                result.m_rayMaxLength     = maxLength;
                result.m_rayStartPosition = rayStartPosition;
                return result;
            }
        }
    }
    else if (rayStartPosition.z <= cylinderMinMaxZ.m_min)
    {
        // ray goes up do not hit
        if (rayForwardNormal.z > 0)
        {
            distanceTravel = (cylinderMinMaxZ.m_min - rayStartPosition.z) / rayForwardNormal.z;
            hitPos         = rayStartPosition + rayForwardNormal * distanceTravel;
            if (distanceTravel < maxLength && IsPointInsideDisc2D(Vec2(hitPos.x, hitPos.y), cylinderCenterXY, cylinderRadius))
            {
                // hit
                result.m_didImpact        = true;
                result.m_impactLength     = distanceTravel;
                result.m_impactNormal     = Vec3(0.f, 0.f, -1.f);
                result.m_impactPosition   = hitPos;
                result.m_rayForwardNormal = rayForwardNormal;
                result.m_rayMaxLength     = maxLength;
                result.m_rayStartPosition = rayStartPosition;
                return result;
            }
        }
    }
    else if (IsPointInsideDisc2D(Vec2(rayStartPosition.x, rayStartPosition.y), cylinderCenterXY, cylinderRadius))
    {
        // start inside cylinder
        result.m_didImpact        = true;
        result.m_impactLength     = 0.f;
        result.m_impactNormal     = -rayForwardNormal;
        result.m_impactPosition   = rayStartPosition;
        result.m_rayForwardNormal = rayForwardNormal;
        result.m_rayMaxLength     = maxLength;
        result.m_rayStartPosition = rayStartPosition;
        return result;
    }

    // hit side
    float normalLength2D = Vec2(rayForwardNormal.x, rayForwardNormal.y).GetLength();
    if (normalLength2D == 0.f)
    {
        return result;
    }
    Vec3 ray_forward_normal = rayForwardNormal / normalLength2D;
    // try to ray cast 2D disc
    RaycastResult2D res2D = RaycastVsDisc2D(Vec2(rayStartPosition.x, rayStartPosition.y), Vec2(ray_forward_normal.x, ray_forward_normal.y), maxLength * normalLength2D, cylinderCenterXY, cylinderRadius);
    bool            isHit = res2D.m_didImpact;

    if (!isHit)
    {
        return result;
    }

    distanceTravel = res2D.m_impactLength / normalLength2D;

    // get the hit position
    hitPos = Vec3(res2D.m_impactPosition.x, res2D.m_impactPosition.y, rayStartPosition.z + distanceTravel * rayForwardNormal.z);
    // pass the cylinder without hitting
    if (hitPos.z > cylinderMinMaxZ.m_max || hitPos.z < cylinderMinMaxZ.m_min)
    {
        return result;
    }
    if (result.m_didImpact && result.m_impactLength < distanceTravel)
    {
        return result;
    }
    // hit side successfully
    result.m_didImpact        = true;
    result.m_impactLength     = distanceTravel;
    result.m_impactNormal     = Vec3(res2D.m_impactNormal.x, res2D.m_impactNormal.y, 0.f);
    result.m_impactPosition   = hitPos;
    result.m_rayForwardNormal = rayForwardNormal;
    result.m_rayMaxLength     = maxLength;
    result.m_rayStartPosition = rayStartPosition;
    return result;
}
