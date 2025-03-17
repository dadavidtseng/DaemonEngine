//----------------------------------------------------------------------------------------------------
// RaycastUtils.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Math/RaycastUtils.hpp"

#include <algorithm>
#include <cmath>

#include "Disc2.hpp"
#include "FloatRange.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/AABB3.hpp"
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
Ray3::Ray3(Vec3 const& startPosition, Vec3 const& normalDirection, float const maxLength)
    : m_startPosition(startPosition),
      m_normalDirection(normalDirection),
      m_maxLength(maxLength)
{
}

//----------------------------------------------------------------------------------------------------
Ray3::Ray3(Vec3 const& startPosition, Vec3 const& endPosition)
    : m_startPosition(startPosition),
      m_normalDirection((endPosition - startPosition).GetNormalized()),
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

    result.m_impactPosition = rayStartPosition + rayForwardNormal * result.m_impactDistance;
    result.m_impactNormal   = (result.m_impactPosition - discCenter).GetNormalized();
    result.m_didImpact      = true;

    return result;
}

//----------------------------------------------------------------------------------------------------
RaycastResult2D RayCastVsLineSegment2D(Vec2 const& rayStartPosition,
                                       Vec2 const& rayForwardNormal,
                                       float const maxLength,
                                       Vec2 const& lineStartPos,
                                       Vec2 const& lineEndPos)
{
    RaycastResult2D result;
    result.m_rayForwardNormal = rayForwardNormal;
    result.m_rayStartPosition = rayStartPosition;
    result.m_rayMaxLength     = maxLength;

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
    if (impactLength <= 0.f || impactLength >= maxLength)
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
    result.m_impactDistance   = impactLength;
    result.m_impactPosition   = impactPos;
    result.m_rayForwardNormal = rayForwardNormal;
    result.m_rayMaxLength     = maxLength;
    result.m_rayStartPosition = rayStartPosition;
    return result;
}

//----------------------------------------------------------------------------------------------------
RaycastResult2D RayCastVsAABB2D(Vec2 const&  rayStartPosition,
                                Vec2 const&  rayForwardNormal,
                                float        maxLength,
                                AABB2 const& aabb2)
{
    RaycastResult2D result;
    result.m_rayForwardNormal = rayForwardNormal;
    result.m_rayStartPosition = rayStartPosition;
    result.m_rayMaxLength     = maxLength;
    result.m_didImpact        = false;

    Vec2 endPos = rayStartPosition + rayForwardNormal * maxLength;

    if (!DoAABB2sOverlap2D(AABB2(Vec2(std::min(rayStartPosition.x, endPos.x), std::min(rayStartPosition.y, endPos.y)), Vec2(std::max(rayStartPosition.x, endPos.x), std::max(rayStartPosition.y, endPos.y))), aabb2))
    {
        result.m_didImpact = false;
        return result;
    }

    if (aabb2.IsPointInside(rayStartPosition))
    {
        // ray from inside AABB2
        result.m_didImpact        = true;
        result.m_impactDistance   = 0.f;
        result.m_impactNormal     = -rayForwardNormal;
        result.m_impactPosition   = rayStartPosition;
        result.m_rayForwardNormal = rayForwardNormal;
        result.m_rayMaxLength     = maxLength;
        result.m_rayStartPosition = rayStartPosition;
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
            result.m_didImpact        = true;
            result.m_impactDistance   = maxLength * minYHitT;
            result.m_impactNormal     = Vec2(0.f, -1.f);
            result.m_impactPosition   = rayStartPosition + rayForwardNormal * result.m_impactDistance;
            result.m_rayForwardNormal = rayForwardNormal;
            result.m_rayMaxLength     = maxLength;
            result.m_rayStartPosition = rayStartPosition;
            return result;
        }

        if (minYHitT >= maxYHitT && maxYHitT >= 0.f)
        {
            // hit top
            result.m_didImpact        = true;
            result.m_impactDistance   = maxLength * maxYHitT;
            result.m_impactNormal     = Vec2(0.f, 1.f);
            result.m_impactPosition   = rayStartPosition + rayForwardNormal * result.m_impactDistance;
            result.m_rayForwardNormal = rayForwardNormal;
            result.m_rayMaxLength     = maxLength;
            result.m_rayStartPosition = rayStartPosition;
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
            result.m_didImpact        = true;
            result.m_impactDistance   = maxLength * minXHitT;
            result.m_impactNormal     = Vec2(-1.f, 0.f);
            result.m_impactPosition   = rayStartPosition + rayForwardNormal * result.m_impactDistance;
            result.m_rayForwardNormal = rayForwardNormal;
            result.m_rayMaxLength     = maxLength;
            result.m_rayStartPosition = rayStartPosition;
            return result;
        }

        if (maxXHitT <= minXHitT && maxXHitT >= 0.f)
        {
            // hit right
            result.m_didImpact        = true;
            result.m_impactDistance   = maxLength * maxXHitT;
            result.m_impactNormal     = Vec2(1.f, 0.f);
            result.m_impactPosition   = rayStartPosition + rayForwardNormal * result.m_impactDistance;
            result.m_rayForwardNormal = rayForwardNormal;
            result.m_rayMaxLength     = maxLength;
            result.m_rayStartPosition = rayStartPosition;
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
            result.m_didImpact        = true;
            result.m_impactDistance   = maxLength * firstYHitT;
            result.m_impactNormal     = Vec2(0.f, -1.f);
            result.m_impactPosition   = rayStartPosition + rayForwardNormal * result.m_impactDistance;
            result.m_rayForwardNormal = rayForwardNormal;
            result.m_rayMaxLength     = maxLength;
            result.m_rayStartPosition = rayStartPosition;
            return result;
        }

        // hit top
        result.m_didImpact        = true;
        result.m_impactDistance   = maxLength * firstYHitT;
        result.m_impactNormal     = Vec2(0.f, 1.f);
        result.m_impactPosition   = rayStartPosition + rayForwardNormal * result.m_impactDistance;
        result.m_rayForwardNormal = rayForwardNormal;
        result.m_rayMaxLength     = maxLength;
        result.m_rayStartPosition = rayStartPosition;
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
        result.m_didImpact        = true;
        result.m_impactDistance   = maxLength * firstXHitT;
        result.m_impactNormal     = Vec2(-1.f, 0.f);
        result.m_impactPosition   = rayStartPosition + rayForwardNormal * result.m_impactDistance;
        result.m_rayForwardNormal = rayForwardNormal;
        result.m_rayMaxLength     = maxLength;
        result.m_rayStartPosition = rayStartPosition;
        return result;
    }

    // hit right
    result.m_didImpact        = true;
    result.m_impactDistance   = maxLength * firstXHitT;
    result.m_impactNormal     = Vec2(1.f, 0.f);
    result.m_impactPosition   = rayStartPosition + rayForwardNormal * result.m_impactDistance;
    result.m_rayForwardNormal = rayForwardNormal;
    result.m_rayMaxLength     = maxLength;
    result.m_rayStartPosition = rayStartPosition;
    return result;
}

RaycastResult3D RaycastVsAABB3D(Vec3 const& rayStartPosition, Vec3 const& rayForwardNormal, float const maxLength, AABB3 const& box)
{
    RaycastResult3D result;
    result.m_rayForwardNormal = rayForwardNormal;
    result.m_rayStartPosition = rayStartPosition;
    result.m_rayMaxLength     = maxLength;
    result.m_didImpact        = false; // Initialize as no impact

    Vec3 endPos = rayStartPosition + rayForwardNormal * maxLength;
    if (!DoAABB3sOverlap3D(AABB3(Vec3(std::min(rayStartPosition.x, endPos.x), std::min(rayStartPosition.y, endPos.y), std::min(rayStartPosition.z, endPos.z)), Vec3(std::max(rayStartPosition.x, endPos.x), std::max(rayStartPosition.y, endPos.y), std::max(rayStartPosition.z, endPos.z))), box))
    {
        result.m_didImpact = false;
        return result;
    }

    if (box.IsPointInside(rayStartPosition))
    {
        result.m_didImpact        = true;
        result.m_impactDist       = 0.f;
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
    if (rayForwardNormal.x == 0)
    {
        if (rayStartPosition.x > box.m_mins.x && rayStartPosition.x < box.m_maxs.x)
        {
            firstXHitT  = -FLT_MAX;
            secondXHitT = FLT_MAX;
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

    if (rayForwardNormal.y == 0)
    {
        if (rayStartPosition.y > box.m_mins.y && rayStartPosition.y < box.m_maxs.y)
        {
            firstYHitT  = -FLT_MAX;
            secondYHitT = FLT_MAX;
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

    if (rayForwardNormal.z == 0)
    {
        if (rayStartPosition.z > box.m_mins.z && rayStartPosition.z < box.m_maxs.z)
        {
            firstZHitT  = -FLT_MAX;
            secondZHitT = FLT_MAX;
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
    result.m_didImpact  = true;
    result.m_impactDist = firstXYZInt * maxLength;

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
    result.m_impactPosition   = rayStartPosition + rayForwardNormal * result.m_impactDist;
    result.m_rayForwardNormal = rayForwardNormal;
    result.m_rayMaxLength     = maxLength;
    result.m_rayStartPosition = rayStartPosition;

    return result;
}

RaycastResult3D RaycastVsSphere3D(Vec3 const& rayStartPosition, Vec3 const& rayForwardNormal, float const maxLength, Vec3 const& sphereCenter, float const sphereRadius)
{
    RaycastResult3D result;
    result.m_rayForwardNormal = rayForwardNormal;
    result.m_rayStartPosition = rayStartPosition;
    result.m_rayMaxLength     = maxLength;
    result.m_didImpact        = false; // Initialize as no impact

    Vec3 SC = sphereCenter - rayStartPosition;
    //Vec3 upVector = CrossProduct3D( forwardNormal, SC );
    //Vec3 leftNormal = CrossProduct3D( upVector, forwardNormal ).GetNormalized();
    float SCi        = DotProduct3D(SC, rayForwardNormal);
    Vec3  leftVector = (SC - rayForwardNormal * SCi);
    /*
    if (leftVector == Vec3( 0.f, 0.f, 0.f )) {
        // ray go through the center of the sphere
        float impactLength = SC.GetLength() - radius;
        // start point inside disc, hit when start
        if (impactLength > -2 * radius && impactLength < 0) {
            out_rayCastRes.m_didImpact = true;
            out_rayCastRes.m_impactDist = 0.f;
            out_rayCastRes.m_impactNormal = -forwardNormal;
            out_rayCastRes.m_impactPos = startPos;
            out_rayCastRes.m_rayForwardNormal = forwardNormal;
            out_rayCastRes.m_rayMaxLength = maxDist;
            out_rayCastRes.m_rayStartPos = startPos;
            return true;
        }
        // length is short or direction is reversed, cannot get to disc
        if (impactLength <= 0.f || impactLength >= maxDist) {
            return false;
        }
        Vec3 impactPos = impactLength * forwardNormal + startPos;
        out_rayCastRes.m_didImpact = true;
        out_rayCastRes.m_impactDist = impactLength;
        out_rayCastRes.m_impactNormal = (impactPos - center) / radius;
        out_rayCastRes.m_impactPos = impactPos;
        out_rayCastRes.m_rayForwardNormal = forwardNormal;
        out_rayCastRes.m_rayMaxLength = maxDist;
        out_rayCastRes.m_rayStartPos = startPos;
        return true;
    }*/

    float squaredSCj = leftVector.GetLengthSquared();
    float squaredA   = sphereRadius * sphereRadius - squaredSCj;
    if (squaredA <= 0.f)
    {
        return result;
    }

    float a            = sqrtf(squaredA);
    float impactLength = SCi - a;

    // start point inside disc, hit when start
    if (impactLength > -2 * a && impactLength < 0)
    {
        result.m_didImpact        = true;
        result.m_impactDist       = 0.f;
        result.m_impactNormal     = -rayForwardNormal;
        result.m_impactPosition   = rayStartPosition;
        result.m_rayForwardNormal = rayForwardNormal;
        result.m_rayMaxLength     = maxLength;
        result.m_rayStartPosition = rayStartPosition;
        return result;
    }
    // length is short or direction is reversed, cannot get to disc
    if (impactLength <= 0.f || impactLength >= maxLength)
    {
        return result;
    }
    // hit the disc
    Vec3 impactPos            = impactLength * rayForwardNormal + rayStartPosition;
    result.m_didImpact        = true;
    result.m_impactDist       = impactLength;
    result.m_impactNormal     = (impactPos - sphereCenter) / sphereRadius;
    result.m_impactPosition   = impactPos;
    result.m_rayForwardNormal = rayForwardNormal;
    result.m_rayMaxLength     = maxLength;
    result.m_rayStartPosition = rayStartPosition;
    return result;
}
//
// RaycastResult3D RaycastVsCylinderZ3D(Vec3 const& rayStartPosition, Vec3 const& rayForwardNormal, float maxLength, Vec3 const& centerXY, FloatRange  minMaxZ, float radiusXY)
// {
//     return {};
// }

RaycastResult3D RaycastVsCylinderZ3D(Vec3 const& startPos, Vec3 const& forwardNormal, float maxDist, Vec2 const& cylinderCenter, float minZ, float maxZ, float radius)
{
    RaycastResult3D result;
    result.m_rayForwardNormal = forwardNormal;
    result.m_rayStartPosition = startPos;
    result.m_rayMaxLength     = maxDist;
    result.m_didImpact        = false; // Initialize as no impact

    float distanceTravel = 0.f;
    Vec3  hitPos;
    result.m_didImpact = false;
    // check if hit top or bottom
    if (startPos.z >= maxZ)
    {
        // ray goes up do not hit
        if (forwardNormal.z < 0)
        {
            distanceTravel = (maxZ - startPos.z) / forwardNormal.z;
            hitPos         = startPos + forwardNormal * distanceTravel;
            if (distanceTravel < maxDist && IsPointInsideDisc2D(Vec2(hitPos.x, hitPos.y), cylinderCenter, radius))
            {
                // hit
                result.m_didImpact        = true;
                result.m_impactDist       = distanceTravel;
                result.m_impactNormal     = Vec3(0.f, 0.f, 1.f);
                result.m_impactPosition   = hitPos;
                result.m_rayForwardNormal = forwardNormal;
                result.m_rayMaxLength     = maxDist;
                result.m_rayStartPosition = startPos;
                return result;
            }
        }
    }
    else if (startPos.z <= minZ)
    {
        // ray goes up do not hit
        if (forwardNormal.z > 0)
        {
            distanceTravel = (minZ - startPos.z) / forwardNormal.z;
            hitPos         = startPos + forwardNormal * distanceTravel;
            if (distanceTravel < maxDist && IsPointInsideDisc2D(Vec2(hitPos.x, hitPos.y), cylinderCenter, radius))
            {
                // hit
                result.m_didImpact        = true;
                result.m_impactDist       = distanceTravel;
                result.m_impactNormal     = Vec3(0.f, 0.f, -1.f);
                result.m_impactPosition   = hitPos;
                result.m_rayForwardNormal = forwardNormal;
                result.m_rayMaxLength     = maxDist;
                result.m_rayStartPosition = startPos;
                return result;
            }
        }
    }
    else if (IsPointInsideDisc2D(Vec2(startPos.x, startPos.y), cylinderCenter, radius))
    {
        // start inside cylinder
        result.m_didImpact        = true;
        result.m_impactDist       = 0.f;
        result.m_impactNormal     = -forwardNormal;
        result.m_impactPosition   = startPos;
        result.m_rayForwardNormal = forwardNormal;
        result.m_rayMaxLength     = maxDist;
        result.m_rayStartPosition = startPos;
        return result;
    }

    // hit side
    float normalLength2D = Vec2(forwardNormal.x, forwardNormal.y).GetLength();
    if (normalLength2D == 0.f)
    {
        return result;
    }
    Vec3            ray_forward_normal = forwardNormal / normalLength2D;
    // try to ray cast 2D disc
    RaycastResult2D res2D              = RaycastVsDisc2D(Vec2(startPos.x, startPos.y), Vec2(ray_forward_normal.x, ray_forward_normal.y), maxDist * normalLength2D, cylinderCenter, radius);
    bool            isHit              = res2D.m_didImpact;

    if (!isHit)
    {
        return result;
    }

    distanceTravel = res2D.m_impactDistance / normalLength2D;

    // get the hit position
    hitPos = Vec3(res2D.m_impactPosition.x, res2D.m_impactPosition.y, startPos.z + distanceTravel * forwardNormal.z);
    // pass the cylinder without hitting
    if (hitPos.z > maxZ || hitPos.z < minZ)
    {
        return result;
    }
    if (result.m_didImpact && result.m_impactDist < distanceTravel)
    {
        return result;
    }
    // hit side successfully
    result.m_didImpact        = true;
    result.m_impactDist       = distanceTravel;
    result.m_impactNormal     = Vec3(res2D.m_impactNormal.x, res2D.m_impactNormal.y, 0.f);
    result.m_impactPosition   = hitPos;
    result.m_rayForwardNormal = forwardNormal;
    result.m_rayMaxLength     = maxDist;
    result.m_rayStartPosition = startPos;
    return result;
}
