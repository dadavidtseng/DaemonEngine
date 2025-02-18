//----------------------------------------------------------------------------------------------------
// VertexUtils.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Core/VertexUtils.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/Capsule2.hpp"
#include "Engine/Math/Disc2.hpp"
#include "Engine/Math/LineSegment2.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/OBB2.hpp"
#include "Engine/Math/Triangle2.hpp"

//----------------------------------------------------------------------------------------------------
void TransformVertexArrayXY3D(int const   numVerts,
                              Vertex_PCU* verts,
                              float const uniformScaleXY,
                              float const rotationDegreesAboutZ,
                              Vec2 const& translationXY)
{
    for (int vertIndex = 0; vertIndex < numVerts; ++vertIndex)
    {
        Vec3& position = verts[vertIndex].m_position;

        TransformPositionXY3D(position, uniformScaleXY, rotationDegreesAboutZ, translationXY);
    }
}

//----------------------------------------------------------------------------------------------------
void AddVertsForDisc2D(VertexList&  verts,
                       Vec2 const&  discCenter,
                       float const  discRadius,
                       Rgba8 const& color)
{
    constexpr int   NUM_SIDES        = 32;
    constexpr float DEGREES_PER_SIDE = 360.f / static_cast<float>(NUM_SIDES);

    for (int sideNum = 0; sideNum < NUM_SIDES; ++sideNum)
    {
        float const startDegrees = DEGREES_PER_SIDE * static_cast<float>(sideNum);
        float const endDegrees   = DEGREES_PER_SIDE * static_cast<float>(sideNum + 1);
        float const cosStart     = CosDegrees(startDegrees);
        float const sinStart     = SinDegrees(startDegrees);
        float const cosEnd       = CosDegrees(endDegrees);
        float const sinEnd       = SinDegrees(endDegrees);

        Vec3 centerPos(discCenter.x, discCenter.y, 0.f);
        Vec3 startOuterPos(discCenter.x + discRadius * cosStart, discCenter.y + discRadius * sinStart, 0.f);
        Vec3 endOuterPos(discCenter.x + discRadius * cosEnd, discCenter.y + discRadius * sinEnd, 0.f);

        verts.emplace_back(centerPos, color);
        verts.emplace_back(startOuterPos, color);
        verts.emplace_back(endOuterPos, color);
    }
}

//----------------------------------------------------------------------------------------------------
void AddVertsForDisc2D(VertexList&  verts,
                       Disc2 const& disc,
                       Rgba8 const& color)
{
    AddVertsForDisc2D(verts, disc.m_position, disc.m_radius, color);
}


//----------------------------------------------------------------------------------------------------
void AddVertsForLineSegment2D(VertexList&  verts,
                              Vec2 const&  startPos,
                              Vec2 const&  endPos,
                              Rgba8 const& color)
{
    verts.emplace_back(Vec3(startPos.x, startPos.y, 0.f), color, Vec2::ZERO);
    verts.emplace_back(Vec3(endPos.x, endPos.y, 0.f), color, Vec2::ZERO);
}

//----------------------------------------------------------------------------------------------------
void AddVertsForLineSegment2D(VertexList&         verts,
                              LineSegment2 const& lineSegment,
                              Rgba8 const&        color)
{
    AddVertsForLineSegment2D(verts, lineSegment.m_start, lineSegment.m_end, color);
}

//----------------------------------------------------------------------------------------------------
void AddVertsForTriangle2D(VertexList&  verts,
                           Vec2 const&  ccw0,
                           Vec2 const&  ccw1,
                           Vec2 const&  ccw2,
                           Rgba8 const& color)
{
    verts.emplace_back(Vec3(ccw0.x, ccw0.y, 0.f), color, Vec2::ZERO);
    verts.emplace_back(Vec3(ccw1.x, ccw1.y, 0.f), color, Vec2::ZERO);
    verts.emplace_back(Vec3(ccw2.x, ccw2.y, 0.f), color, Vec2::ZERO);
}

//----------------------------------------------------------------------------------------------------
void AddVertsForTriangle2D(VertexList&      verts,
                           Triangle2 const& triangle,
                           Rgba8 const&     color)
{
    AddVertsForTriangle2D(verts,
                          triangle.m_positionCounterClockwise[0],
                          triangle.m_positionCounterClockwise[1],
                          triangle.m_positionCounterClockwise[2],
                          color);
}

//----------------------------------------------------------------------------------------------------
void AddVertsForAABB2D(VertexList&  verts,
                       AABB2 const& aabb2Box,
                       Rgba8 const& color,
                       Vec2 const&  uvMins,
                       Vec2 const&  uvMaxs)
{
    verts.emplace_back(Vec3(aabb2Box.m_mins.x, aabb2Box.m_mins.y, 0.f), color, uvMins);
    verts.emplace_back(Vec3(aabb2Box.m_maxs.x, aabb2Box.m_mins.y, 0.f), color, Vec2(uvMaxs.x, uvMins.y));
    verts.emplace_back(Vec3(aabb2Box.m_maxs.x, aabb2Box.m_maxs.y, 0.f), color, uvMaxs);

    verts.emplace_back(Vec3(aabb2Box.m_mins.x, aabb2Box.m_mins.y, 0.f), color, uvMins);
    verts.emplace_back(Vec3(aabb2Box.m_maxs.x, aabb2Box.m_maxs.y, 0.f), color, uvMaxs);
    verts.emplace_back(Vec3(aabb2Box.m_mins.x, aabb2Box.m_maxs.y, 0.f), color, Vec2(uvMins.x, uvMaxs.y));
}

//-----------------------------------------------------------------------------------------------
void AddVertsForOBB2D(VertexList&  verts,
                      OBB2 const&  obb2Box,
                      Rgba8 const& color)
{
    Vec2 cornerPoints[4];
    obb2Box.GetCornerPoints(cornerPoints);

    AddVertsForTriangle2D(verts, cornerPoints[0], cornerPoints[1], cornerPoints[2], color);
    AddVertsForTriangle2D(verts, cornerPoints[0], cornerPoints[2], cornerPoints[3], color);
}

void AddVertsForCapsule2D(VertexList&  verts,
                          Vec2 const&  boneStart,
                          Vec2 const&  boneEnd,
                          float const  radius,
                          Rgba8 const& color)
{
    Vec2 direction = boneEnd - boneStart;
    direction.Normalize();

    Vec2 const perpendicular = Vec2(-direction.y, direction.x) * radius;
    Vec2 const bottomLeft    = boneStart - perpendicular;
    Vec2 const bottomRight   = boneStart + perpendicular;
    Vec2 const topLeft       = boneEnd - perpendicular;
    Vec2 const topRight      = boneEnd + perpendicular;

    verts.emplace_back(Vec3(bottomLeft.x, bottomLeft.y, 0.f), color, Vec2::ZERO);
    verts.emplace_back(Vec3(bottomRight.x, bottomRight.y, 0.f), color, Vec2(1.f, 0.f));
    verts.emplace_back(Vec3(topRight.x, topRight.y, 0.f), color, Vec2::ONE);

    verts.emplace_back(Vec3(topRight.x, topRight.y, 0.f), color, Vec2::ONE);
    verts.emplace_back(Vec3(topLeft.x, topLeft.y, 0.f), color, Vec2(0.f, 1.f));
    verts.emplace_back(Vec3(bottomLeft.x, bottomLeft.y, 0.f), color, Vec2::ZERO);

    Vec2 const  rotatedDirection    = direction.GetRotatedMinus90Degrees();
    float const rotationAngle       = Atan2Degrees(rotatedDirection.y, rotatedDirection.x);
    Vec2 const& halfDiscCenterStart = boneStart;
    Vec2 const& halfDiscCenterEnd   = boneEnd;

    AddVertsForHalfDisc2D(verts, halfDiscCenterStart, radius, color, false, rotationAngle);
    AddVertsForHalfDisc2D(verts, halfDiscCenterEnd, radius, color, true, rotationAngle);
}

//----------------------------------------------------------------------------------------------------
void AddVertsForCapsule2D(VertexList&     verts,
                          Capsule2 const& capsule,
                          Rgba8 const&    color)
{
    AddVertsForCapsule2D(verts, capsule.m_start, capsule.m_end, capsule.m_radius, color);
}

//----------------------------------------------------------------------------------------------------
void AddVertsForHalfDisc2D(VertexList&  verts,
                           Vec2 const&  discCenter,
                           float const  discRadius,
                           Rgba8 const& color,
                           bool const   isTopHalf,
                           float const  rotationDegrees)
{
    constexpr int   NUM_SIDES        = 32;
    constexpr float DEGREES_PER_SIDE = 180.f / static_cast<float>(NUM_SIDES); // ????180?

    for (int sideNum = 0; sideNum < NUM_SIDES; ++sideNum)
    {
        float startDegrees = DEGREES_PER_SIDE * static_cast<float>(sideNum);
        float endDegrees   = DEGREES_PER_SIDE * static_cast<float>(sideNum + 1);

        if (!isTopHalf)
        {
            startDegrees += 180.f;
            endDegrees += 180.f;
        }

        startDegrees += rotationDegrees;
        endDegrees += rotationDegrees;

        float cosStart = CosDegrees(startDegrees);
        float sinStart = SinDegrees(startDegrees);
        float cosEnd   = CosDegrees(endDegrees);
        float sinEnd   = SinDegrees(endDegrees);

        Vec3 centerPos(discCenter.x, discCenter.y, 0.f);
        Vec3 startOuterPos(discCenter.x + discRadius * cosStart, discCenter.y + discRadius * sinStart, 0.f);
        Vec3 endOuterPos(discCenter.x + discRadius * cosEnd, discCenter.y + discRadius * sinEnd, 0.f);

        verts.emplace_back(centerPos, color);
        verts.emplace_back(startOuterPos, color);
        verts.emplace_back(endOuterPos, color);
    }
}

void AddVertsForArrow2D(VertexList& verts, Vec2 const& tailPos, Vec2 const& tipPos, float arrowSize, float thickness, Rgba8 const& color)
{
    Vec2 const direction           = tipPos - tailPos;
    Vec2 const normalizedDirection = -direction.GetNormalized();

    Vec2 const arrowDirectionLeft  = normalizedDirection.GetRotatedDegrees(45.f);
    Vec2 const arrowDirectionRight = normalizedDirection.GetRotatedDegrees(-45.f);

    Vec2 const leftArrow  = tipPos + arrowDirectionLeft * arrowSize;
    Vec2 const rightArrow = tipPos + arrowDirectionRight * arrowSize;

    Vec2 const mainPerpendicular = normalizedDirection.GetRotated90Degrees() * (thickness * 0.5f);

    Vec3 tailLeft  = Vec3((tailPos + mainPerpendicular).x, (tailPos + mainPerpendicular).y, 0.0f);
    Vec3 tailRight = Vec3((tailPos - mainPerpendicular).x, (tailPos - mainPerpendicular).y, 0.0f);
    Vec3 tipLeft   = Vec3((tipPos + mainPerpendicular).x, (tipPos + mainPerpendicular).y, 0.0f);
    Vec3 tipRight  = Vec3((tipPos - mainPerpendicular).x, (tipPos - mainPerpendicular).y, 0.0f);

    verts.emplace_back(tailLeft, color);
    verts.emplace_back(tipLeft, color);
    verts.emplace_back(tipRight, color);

    verts.emplace_back(tailLeft, color);
    verts.emplace_back(tipRight, color);
    verts.emplace_back(tailRight, color);

    // 計算箭頭左側的垂直向量
    Vec2 const leftPerpendicular = arrowDirectionLeft.GetRotated90Degrees() * (thickness * 0.5f);

    // 箭頭左側的四個頂點
    Vec3 leftTipLeft  = Vec3((tipPos + leftPerpendicular).x, (tipPos + leftPerpendicular).y, 0.0f);
    Vec3 leftTipRight = Vec3((tipPos - leftPerpendicular).x, (tipPos - leftPerpendicular).y, 0.0f);
    Vec3 leftEndLeft  = Vec3((leftArrow + leftPerpendicular).x, (leftArrow + leftPerpendicular).y, 0.0f);
    Vec3 leftEndRight = Vec3((leftArrow - leftPerpendicular).x, (leftArrow - leftPerpendicular).y, 0.0f);

    // 增加箭頭左側的頂點
    verts.emplace_back(leftTipLeft, color);
    verts.emplace_back(leftEndLeft, color);
    verts.emplace_back(leftEndRight, color);

    verts.emplace_back(leftTipLeft, color);
    verts.emplace_back(leftEndRight, color);
    verts.emplace_back(leftTipRight, color);

    // 計算箭頭右側的垂直向量
    Vec2 const rightPerpendicular = arrowDirectionRight.GetRotated90Degrees() * (thickness * 0.5f);

    // 箭頭右側的四個頂點
    Vec3 rightTipLeft  = Vec3((tipPos + rightPerpendicular).x, (tipPos + rightPerpendicular).y, 0.0f);
    Vec3 rightTipRight = Vec3((tipPos - rightPerpendicular).x, (tipPos - rightPerpendicular).y, 0.0f);
    Vec3 rightEndLeft  = Vec3((rightArrow + rightPerpendicular).x, (rightArrow + rightPerpendicular).y, 0.0f);
    Vec3 rightEndRight = Vec3((rightArrow - rightPerpendicular).x, (rightArrow - rightPerpendicular).y, 0.0f);

    // 增加箭頭右側的頂點
    verts.emplace_back(rightTipLeft, color);
    verts.emplace_back(rightEndLeft, color);
    verts.emplace_back(rightEndRight, color);

    verts.emplace_back(rightTipLeft, color);
    verts.emplace_back(rightEndRight, color);
    verts.emplace_back(rightTipRight, color);
}

//----------------------------------------------------------------------------------------------------
void AddVertsForQuad3D(std::vector<Vertex_PCU>& verts, Vec3 const& bottomLeft, Vec3 const& bottomRight, Vec3 const& topRight, Vec3 const& topLeft, Rgba8 const& color, AABB2 const& uv)
{
    // Starting at BL, add triangle A with vertexes BL, BR, TR.
    verts.push_back(Vertex_PCU(bottomLeft, color, Vec2(uv.m_mins.x, uv.m_mins.y)));
    verts.push_back(Vertex_PCU(bottomRight, color, Vec2(uv.m_maxs.x, uv.m_mins.y)));
    verts.push_back(Vertex_PCU(topRight, color, Vec2(uv.m_maxs.x, uv.m_maxs.y)));

    // Starting again at BL, add triangle B with vertexes BL, TR, TL.
    verts.push_back(Vertex_PCU(bottomLeft, color, Vec2(uv.m_mins.x, uv.m_mins.y)));
    verts.push_back(Vertex_PCU(topRight, color, Vec2(uv.m_maxs.x, uv.m_maxs.y)));
    verts.push_back(Vertex_PCU(topLeft, color, Vec2(uv.m_mins.x, uv.m_maxs.y)));
}
