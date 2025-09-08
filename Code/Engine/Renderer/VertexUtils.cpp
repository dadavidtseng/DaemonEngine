//----------------------------------------------------------------------------------------------------
// VertexUtils.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Renderer/VertexUtils.hpp"

#include "Vertex_PCUTBN.hpp"
#include "Engine/Renderer/Vertex_PCU.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Math/Capsule2.hpp"
#include "Engine/Math/Disc2.hpp"
#include "Engine/Math/LineSegment2.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/OBB3.hpp"
#include "Engine/Math/Triangle2.hpp"
#include "Engine/Platform/Window.hpp"

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

//----------------------------------------------------------------------------------------------------
AABB2 GetVertexBounds2D(VertexList_PCU const& verts)
{
    if (verts.empty() == true)
    {
        return {};
    }

    Vec2 min = Vec2(verts[0].m_position.x, verts[0].m_position.y);
    Vec2 max = Vec2(verts[0].m_position.x, verts[0].m_position.y);

    for (Vertex_PCU const& vert : verts)
    {
        min.x = std::min(min.x, vert.m_position.x);
        min.y = std::min(min.y, vert.m_position.y);
        max.x = std::max(max.x, vert.m_position.x);
        max.y = std::max(max.y, vert.m_position.y);
    }

    return AABB2(min, max);
}

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
void TransformVertexArray3D(VertexList_PCU& verts,
                            Mat44 const&    transform)
{
    for (Vertex_PCU& vert : verts)
    {
        vert.m_position = transform.TransformPosition3D(vert.m_position);
    }
}

//----------------------------------------------------------------------------------------------------
void TransformVertexArray3D(VertexList_PCUTBN& verts,
                            Mat44 const&       transform)
{
    for (Vertex_PCUTBN& vert : verts)
    {
        vert.m_position = transform.TransformPosition3D(vert.m_position);
    }
}

//----------------------------------------------------------------------------------------------------
void AddVertsForDisc2D(VertexList_PCU& verts,
                       Vec2 const&     discCenter,
                       float const     discRadius,
                       Rgba8 const&    fillColor)
{
    // 1. Calculate the degree of each triangle in the disc.
    int constexpr   NUM_SIDES        = 32;
    float constexpr DEGREES_PER_SIDE = 360.f / static_cast<float>(NUM_SIDES);

    for (int sideIndex = 0; sideIndex < NUM_SIDES; ++sideIndex)
    {
        // 2. Get the degree of each triangle on the unit circle.
        float const startDegrees = DEGREES_PER_SIDE * static_cast<float>(sideIndex);
        float const endDegrees   = DEGREES_PER_SIDE * static_cast<float>(sideIndex + 1);
        float const cosStart     = CosDegrees(startDegrees);
        float const sinStart     = SinDegrees(startDegrees);
        float const cosEnd       = CosDegrees(endDegrees);
        float const sinEnd       = SinDegrees(endDegrees);

        // 3. Get the positions by ( discCenter ) + ( discRadius ) * ( cos / sin )
        Vec3 centerPosition(discCenter.x, discCenter.y, 0.f);
        Vec3 startOuterPosition(discCenter.x + discRadius * cosStart, discCenter.y + discRadius * sinStart, 0.f);
        Vec3 endOuterPosition(discCenter.x + discRadius * cosEnd, discCenter.y + discRadius * sinEnd, 0.f);

        // 4. Stores the vertices using counter-clockwise order.
        verts.emplace_back(centerPosition, fillColor);
        verts.emplace_back(startOuterPosition, fillColor);
        verts.emplace_back(endOuterPosition, fillColor);
    }
}

void AddVertsForDisc2D(VertexList_PCU& verts,
                       Vec2 const&     discCenter,
                       float const     discRadius,
                       float const     thickness,
                       Rgba8 const&    outlineColor)
{
    float const     halfThickness    = thickness * 0.5f;
    float const     innerRadius      = discRadius - halfThickness;
    float const     outerRadius      = discRadius + halfThickness;
    int constexpr   NUM_SIDES        = 32;
    float constexpr DEGREES_PER_SIDE = 360.f / static_cast<float>(NUM_SIDES);

    verts.reserve(static_cast<int>(verts.size()) + NUM_SIDES * 6);

    for (int sideNum = 0; sideNum < NUM_SIDES; ++sideNum)
    {
        // Compute angle-related terms
        float const startDegrees = DEGREES_PER_SIDE * static_cast<float>(sideNum);
        float const endDegrees   = DEGREES_PER_SIDE * static_cast<float>(sideNum + 1);
        float const cosStart     = CosDegrees(startDegrees);
        float const sinStart     = SinDegrees(startDegrees);
        float const cosEnd       = CosDegrees(endDegrees);
        float const sinEnd       = SinDegrees(endDegrees);

        // Compute inner and outer positions
        Vec3 const innerStartPos(discCenter.x + innerRadius * cosStart, discCenter.y + innerRadius * sinStart, 0.f);
        Vec3 const outerStartPos(discCenter.x + outerRadius * cosStart, discCenter.y + outerRadius * sinStart, 0.f);
        Vec3 const outerEndPos(discCenter.x + outerRadius * cosEnd, discCenter.y + outerRadius * sinEnd, 0.f);
        Vec3 const innerEndPos(discCenter.x + innerRadius * cosEnd, discCenter.y + innerRadius * sinEnd, 0.f);

        verts.emplace_back(innerEndPos, outlineColor);
        verts.emplace_back(innerStartPos, outlineColor);
        verts.emplace_back(outerStartPos, outlineColor);
        verts.emplace_back(innerEndPos, outlineColor);
        verts.emplace_back(outerStartPos, outlineColor);
        verts.emplace_back(outerEndPos, outlineColor);
    }
}

void AddVertsForDisc3D(VertexList_PCU& verts,
                       Vec3 const&     discCenter,
                       float const     discRadius,
                       Vec3 const&     normalDirection,
                       Rgba8 const&    color)
{
    // 1. Calculate the degree of each triangle in the disc.
    int constexpr   NUM_SIDES        = 32;
    float constexpr DEGREES_PER_SIDE = 360.f / static_cast<float>(NUM_SIDES);

    // 2. Get jBasis and kBasis based on normalDirection.
    Vec3 jBasis, kBasis;
    normalDirection.GetOrthonormalBasis(normalDirection, &jBasis, &kBasis);

    for (int sideIndex = 0; sideIndex < NUM_SIDES; ++sideIndex)
    {
        // 3. Get the degree of each triangle on the unit circle.
        float const startDegrees = DEGREES_PER_SIDE * static_cast<float>(sideIndex);
        float const endDegrees   = DEGREES_PER_SIDE * static_cast<float>(sideIndex + 1);
        float const cosStart     = CosDegrees(startDegrees);
        float const sinStart     = SinDegrees(startDegrees);
        float const cosEnd       = CosDegrees(endDegrees);
        float const sinEnd       = SinDegrees(endDegrees);

        // 4. Get the positions by ( discCenter ) + ( discRadius ) * ( cos * jBasis + sin * kBasis )
        Vec3 centerPosition(discCenter);
        Vec3 startOuterPosition(discCenter + discRadius * (cosStart * jBasis + sinStart * kBasis));
        Vec3 endOuterPosition(discCenter + discRadius * (cosEnd * jBasis + sinEnd * kBasis));

        // 5. Stores the vertices using counter-clockwise order.
        verts.emplace_back(centerPosition, color);
        verts.emplace_back(startOuterPosition, color);
        verts.emplace_back(endOuterPosition, color);
    }
}

//----------------------------------------------------------------------------------------------------
void AddVertsForDisc2D(VertexList_PCU& verts,
                       Disc2 const&    disc,
                       Rgba8 const&    color)
{
    AddVertsForDisc2D(verts,
                      disc.m_position,
                      disc.m_radius,
                      color);
}

//----------------------------------------------------------------------------------------------------
void AddVertsForLineSegment2D(VertexList_PCU& verts,
                              Vec2 const&     startPosition,
                              Vec2 const&     endPosition,
                              float const     thickness,
                              bool const      isInfinite,
                              Rgba8 const&    color)
{
    verts.reserve(verts.size() + 6);

    // 1. Calculate lineSegment's forward/normalized direction.
    Vec2 const forwardDirection    = endPosition - startPosition;
    Vec2 const normalizedDirection = forwardDirection.GetNormalized();

    // 2. Calculate lineSegment's halfThicknessOffset for next step.
    Vec2 const perpendicular90DegreesDirection = normalizedDirection.GetRotated90Degrees();
    Vec2 const halfThicknessOffset             = perpendicular90DegreesDirection * (0.5f * thickness);

    Vec3 startLeft  = Vec3(startPosition.x + halfThicknessOffset.x, startPosition.y + halfThicknessOffset.y, 0.f);
    Vec3 startRight = Vec3(startPosition.x - halfThicknessOffset.x, startPosition.y - halfThicknessOffset.y, 0.f);
    Vec3 endLeft    = Vec3(endPosition.x + halfThicknessOffset.x, endPosition.y + halfThicknessOffset.y, 0.f);
    Vec3 endRight   = Vec3(endPosition.x - halfThicknessOffset.x, endPosition.y - halfThicknessOffset.y, 0.f);

    // 3. If this lineSegment is infinite, extend start and end points.
    if (isInfinite)
    {
        // Calculate the infinite extensionFactor by getting the screen size from s_mainWindow.
        float const extensionFactor = static_cast<float>(Window::s_mainWindow->GetClientDimensions().x);

        // Calculate the extendAmount.
        Vec2 const extendAmount = forwardDirection.GetNormalized() * extensionFactor;

        // Apply the extension.
        startLeft -= Vec3(extendAmount.x, extendAmount.y, 0.f);
        startRight -= Vec3(extendAmount.x, extendAmount.y, 0.f);
        endLeft += Vec3(extendAmount.x, extendAmount.y, 0.f);
        endRight += Vec3(extendAmount.x, extendAmount.y, 0.f);
    }

    // Stores the vertices using counter-clockwise order in first triangle.
    verts.emplace_back(startLeft, color);
    verts.emplace_back(startRight, color);
    verts.emplace_back(endRight, color);

    // Stores the vertices using counter-clockwise order in second triangle.
    verts.emplace_back(startLeft, color);
    verts.emplace_back(endRight, color);
    verts.emplace_back(endLeft, color);
}

//----------------------------------------------------------------------------------------------------
void AddVertsForLineSegment2D(VertexList_PCU&     verts,
                              LineSegment2 const& lineSegment,
                              float const         thickness,
                              bool const          isInfinite,
                              Rgba8 const&        color)
{
    AddVertsForLineSegment2D(verts,
                             lineSegment.m_startPosition,
                             lineSegment.m_endPosition,
                             thickness,
                             isInfinite,
                             color);
}

//----------------------------------------------------------------------------------------------------
void AddVertsForTriangle2D(VertexList_PCU& verts,
                           Vec2 const&     ccw0,
                           Vec2 const&     ccw1,
                           Vec2 const&     ccw2,
                           Rgba8 const&    color)
{
    verts.emplace_back(Vec3(ccw0.x, ccw0.y, 0.f), color);
    verts.emplace_back(Vec3(ccw1.x, ccw1.y, 0.f), color);
    verts.emplace_back(Vec3(ccw2.x, ccw2.y, 0.f), color);
}

//----------------------------------------------------------------------------------------------------
void AddVertsForTriangle2D(VertexList_PCU&  verts,
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
void AddVertsForAABB2D(VertexList_PCU& verts,
                       AABB2 const&    aabb2Box,
                       Rgba8 const&    color,
                       Vec2 const&     uvMins,
                       Vec2 const&     uvMaxs)
{
    verts.emplace_back(Vec3(aabb2Box.m_mins.x, aabb2Box.m_mins.y, 0.f), color, uvMins);
    verts.emplace_back(Vec3(aabb2Box.m_maxs.x, aabb2Box.m_mins.y, 0.f), color, Vec2(uvMaxs.x, uvMins.y));
    verts.emplace_back(Vec3(aabb2Box.m_maxs.x, aabb2Box.m_maxs.y, 0.f), color, uvMaxs);

    verts.emplace_back(Vec3(aabb2Box.m_mins.x, aabb2Box.m_mins.y, 0.f), color, uvMins);
    verts.emplace_back(Vec3(aabb2Box.m_maxs.x, aabb2Box.m_maxs.y, 0.f), color, uvMaxs);
    verts.emplace_back(Vec3(aabb2Box.m_mins.x, aabb2Box.m_maxs.y, 0.f), color, Vec2(uvMins.x, uvMaxs.y));
}

//----------------------------------------------------------------------------------------------------
void AddVertsForAABB2D(VertexList_PCU& verts,
                       Vec2 const&     aabbMins,
                       Vec2 const&     aabbMaxs,
                       Rgba8 const&    color,
                       Vec2 const&     uvMins,
                       Vec2 const&     uvMaxs)
{
    verts.emplace_back(Vec3(aabbMins.x, aabbMins.y, 0.f), color, uvMins);
    verts.emplace_back(Vec3(aabbMaxs.x, aabbMins.y, 0.f), color, Vec2(uvMaxs.x, uvMins.y));
    verts.emplace_back(Vec3(aabbMaxs.x, aabbMaxs.y, 0.f), color, uvMaxs);

    verts.emplace_back(Vec3(aabbMins.x, aabbMins.y, 0.f), color, uvMins);
    verts.emplace_back(Vec3(aabbMaxs.x, aabbMaxs.y, 0.f), color, uvMaxs);
    verts.emplace_back(Vec3(aabbMins.x, aabbMaxs.y, 0.f), color, Vec2(uvMins.x, uvMaxs.y));
}

//-----------------------------------------------------------------------------------------------
void AddVertsForOBB2D(VertexList_PCU& verts,
                      Vec2 const&     obb2Center,
                      Vec2 const&     obb2IBasisNormal,
                      Vec2 const&     obb2HalfDimensions,
                      Rgba8 const&    color)
{
    Vec2 cornerPoints[4];

    Vec2 const jBasisNormal = Vec2(-obb2IBasisNormal.y, obb2IBasisNormal.x);

    cornerPoints[0] = obb2Center - obb2IBasisNormal * obb2HalfDimensions.x - jBasisNormal * obb2HalfDimensions.y; // BottomLeft ( Mins )
    cornerPoints[1] = obb2Center + obb2IBasisNormal * obb2HalfDimensions.x - jBasisNormal * obb2HalfDimensions.y; // BottomRight
    cornerPoints[2] = obb2Center + obb2IBasisNormal * obb2HalfDimensions.x + jBasisNormal * obb2HalfDimensions.y; // TopRight ( Maxs )
    cornerPoints[3] = obb2Center - obb2IBasisNormal * obb2HalfDimensions.x + jBasisNormal * obb2HalfDimensions.y; // TopLeft

    AddVertsForTriangle2D(verts, cornerPoints[0], cornerPoints[1], cornerPoints[2], color);
    AddVertsForTriangle2D(verts, cornerPoints[0], cornerPoints[2], cornerPoints[3], color);
}

//----------------------------------------------------------------------------------------------------
void AddVertsForOBB3D(VertexList_PCU& verts,
                      OBB3 const&     obb3,
                      Rgba8 const&    color,
                      AABB2 const&    UVs)
{
    Vec3 minXminYminZ = obb3.m_center - obb3.m_iBasis * obb3.m_halfDimensions.x -
    obb3.m_jBasis * obb3.m_halfDimensions.y - obb3.m_kBasis * obb3.m_halfDimensions.z;
    Vec3 minXminYmaxZ = obb3.m_center - obb3.m_iBasis * obb3.m_halfDimensions.x -
    obb3.m_jBasis * obb3.m_halfDimensions.y + obb3.m_kBasis * obb3.m_halfDimensions.z;
    Vec3 minXmaxYminZ = obb3.m_center - obb3.m_iBasis * obb3.m_halfDimensions.x +
    obb3.m_jBasis * obb3.m_halfDimensions.y - obb3.m_kBasis * obb3.m_halfDimensions.z;
    Vec3 minXmaxYmaxZ = obb3.m_center - obb3.m_iBasis * obb3.m_halfDimensions.x +
    obb3.m_jBasis * obb3.m_halfDimensions.y + obb3.m_kBasis * obb3.m_halfDimensions.z;
    Vec3 maxXminYminZ = obb3.m_center + obb3.m_iBasis * obb3.m_halfDimensions.x -
    obb3.m_jBasis * obb3.m_halfDimensions.y - obb3.m_kBasis * obb3.m_halfDimensions.z;
    Vec3 maxXminYmaxZ = obb3.m_center + obb3.m_iBasis * obb3.m_halfDimensions.x -
    obb3.m_jBasis * obb3.m_halfDimensions.y + obb3.m_kBasis * obb3.m_halfDimensions.z;
    Vec3 maxXmaxYminZ = obb3.m_center + obb3.m_iBasis * obb3.m_halfDimensions.x +
    obb3.m_jBasis * obb3.m_halfDimensions.y - obb3.m_kBasis * obb3.m_halfDimensions.z;
    Vec3 maxXmaxYmaxZ = obb3.m_center + obb3.m_iBasis * obb3.m_halfDimensions.x +
    obb3.m_jBasis * obb3.m_halfDimensions.y + obb3.m_kBasis * obb3.m_halfDimensions.z;

    AddVertsForQuad3D(verts, maxXminYminZ, maxXmaxYminZ, maxXminYmaxZ, maxXmaxYmaxZ, color, UVs);
    AddVertsForQuad3D(verts, minXmaxYminZ, minXminYminZ, minXmaxYmaxZ, minXminYmaxZ, color, UVs);
    AddVertsForQuad3D(verts, minXminYminZ, maxXminYminZ, minXminYmaxZ, maxXminYmaxZ, color, UVs);
    AddVertsForQuad3D(verts, maxXmaxYminZ, minXmaxYminZ, maxXmaxYmaxZ, minXmaxYmaxZ, color, UVs);
    AddVertsForQuad3D(verts, maxXminYmaxZ, maxXmaxYmaxZ, minXminYmaxZ, minXmaxYmaxZ, color, UVs);
    AddVertsForQuad3D(verts, minXminYminZ, minXmaxYminZ, maxXminYminZ, maxXmaxYminZ, color, UVs);
}

void AddVertsForOBB3D(VertexList_PCUTBN& verts,
                      IndexList&         indexes,
                      OBB3 const&        obb3,
                      Rgba8 const&       color,
                      AABB2 const&       UVs)
{
    Vec3 center = obb3.m_center;
    Vec3 i      = obb3.m_iBasis * obb3.m_halfDimensions.x;
    Vec3 j      = obb3.m_jBasis * obb3.m_halfDimensions.y;
    Vec3 k      = obb3.m_kBasis * obb3.m_halfDimensions.z;

    Vec3 corners[8] = {
        center - i - j - k, // 0
        center - i - j + k, // 1
        center - i + j - k, // 2
        center - i + j + k, // 3
        center + i - j - k, // 4
        center + i - j + k, // 5
        center + i + j - k, // 6
        center + i + j + k  // 7
    };

    struct Face
    {
        int  idx[4];
        Vec3 normal;
        Vec3 tangent;
        Vec3 bitangent;
    };

    Face faces[6] = {
        // idx: TL, TR, BL, BR
        {{4, 6, 5, 7}, i.GetNormalized(), j.GetNormalized(), k.GetNormalized()},  // +X
        {{2, 0, 3, 1}, -i.GetNormalized(), j.GetNormalized(), -k.GetNormalized()},  // -X
        {{6, 2, 7, 3}, j.GetNormalized(), i.GetNormalized(), k.GetNormalized()},  // +Y
        {{0, 4, 1, 5}, -j.GetNormalized(), i.GetNormalized(), -k.GetNormalized()},  // -Y
        {{1, 5, 3, 7}, k.GetNormalized(), i.GetNormalized(), j.GetNormalized()},  // +Z
        {{0, 2, 4, 6}, -k.GetNormalized(), i.GetNormalized(), -j.GetNormalized()},  // -Z
    };

    unsigned int baseIndex = static_cast<unsigned int>(verts.size());

    for (int f = 0; f < 6; ++f)
    {
        const Face& face = faces[f];

        Vec2 uvBL(UVs.m_mins.x, UVs.m_mins.y); // Bottom Left
        Vec2 uvBR(UVs.m_maxs.x, UVs.m_mins.y); // Bottom Right
        Vec2 uvTL(UVs.m_mins.x, UVs.m_maxs.y); // Top Left
        Vec2 uvTR(UVs.m_maxs.x, UVs.m_maxs.y); // Top Right

        verts.emplace_back(Vertex_PCUTBN(corners[face.idx[0]], color, uvTL, face.tangent, face.bitangent, face.normal));
        verts.emplace_back(Vertex_PCUTBN(corners[face.idx[1]], color, uvTR, face.tangent, face.bitangent, face.normal));
        verts.emplace_back(Vertex_PCUTBN(corners[face.idx[2]], color, uvBL, face.tangent, face.bitangent, face.normal));
        verts.emplace_back(Vertex_PCUTBN(corners[face.idx[3]], color, uvBR, face.tangent, face.bitangent, face.normal));

        indexes.push_back(baseIndex + 0);
        indexes.push_back(baseIndex + 1);
        indexes.push_back(baseIndex + 2);

        indexes.push_back(baseIndex + 2);
        indexes.push_back(baseIndex + 1);
        indexes.push_back(baseIndex + 3);

        baseIndex += 4;
    }
}

//----------------------------------------------------------------------------------------------------
void AddVertsForWireframeOBB3D(VertexList_PCU& verts,
                               OBB3 const&     obb3,
                               Rgba8 const&    color)
{
    Vec3 minXminYminZ = obb3.m_center - obb3.m_iBasis * obb3.m_halfDimensions.x -
    obb3.m_jBasis * obb3.m_halfDimensions.y - obb3.m_kBasis * obb3.m_halfDimensions.z;
    Vec3 minXminYmaxZ = obb3.m_center - obb3.m_iBasis * obb3.m_halfDimensions.x -
    obb3.m_jBasis * obb3.m_halfDimensions.y + obb3.m_kBasis * obb3.m_halfDimensions.z;
    Vec3 minXmaxYminZ = obb3.m_center - obb3.m_iBasis * obb3.m_halfDimensions.x +
    obb3.m_jBasis * obb3.m_halfDimensions.y - obb3.m_kBasis * obb3.m_halfDimensions.z;
    Vec3 minXmaxYmaxZ = obb3.m_center - obb3.m_iBasis * obb3.m_halfDimensions.x +
    obb3.m_jBasis * obb3.m_halfDimensions.y + obb3.m_kBasis * obb3.m_halfDimensions.z;
    Vec3 maxXminYminZ = obb3.m_center + obb3.m_iBasis * obb3.m_halfDimensions.x -
    obb3.m_jBasis * obb3.m_halfDimensions.y - obb3.m_kBasis * obb3.m_halfDimensions.z;
    Vec3 maxXminYmaxZ = obb3.m_center + obb3.m_iBasis * obb3.m_halfDimensions.x -
    obb3.m_jBasis * obb3.m_halfDimensions.y + obb3.m_kBasis * obb3.m_halfDimensions.z;
    Vec3 maxXmaxYminZ = obb3.m_center + obb3.m_iBasis * obb3.m_halfDimensions.x +
    obb3.m_jBasis * obb3.m_halfDimensions.y - obb3.m_kBasis * obb3.m_halfDimensions.z;
    Vec3 maxXmaxYmaxZ = obb3.m_center + obb3.m_iBasis * obb3.m_halfDimensions.x +
    obb3.m_jBasis * obb3.m_halfDimensions.y + obb3.m_kBasis * obb3.m_halfDimensions.z;
    float diagonalLength = 2.f * obb3.m_halfDimensions.GetLength();

    AddVertsForWireframeQuad3D(verts, maxXminYminZ, maxXmaxYminZ, maxXminYmaxZ, maxXmaxYmaxZ, diagonalLength / 200.f, color);
    AddVertsForWireframeQuad3D(verts, minXmaxYminZ, minXminYminZ, minXmaxYmaxZ, minXminYmaxZ, diagonalLength / 200.f, color);
    AddVertsForWireframeQuad3D(verts, minXminYminZ, maxXminYminZ, minXminYmaxZ, maxXminYmaxZ, diagonalLength / 200.f, color);
    AddVertsForWireframeQuad3D(verts, maxXmaxYminZ, minXmaxYminZ, maxXmaxYmaxZ, minXmaxYmaxZ, diagonalLength / 200.f, color);
    AddVertsForWireframeQuad3D(verts, maxXminYmaxZ, maxXmaxYmaxZ, minXminYmaxZ, minXmaxYmaxZ, diagonalLength / 200.f, color);
    AddVertsForWireframeQuad3D(verts, minXminYminZ, minXmaxYminZ, maxXminYminZ, maxXmaxYminZ, diagonalLength / 200.f, color);
}

//----------------------------------------------------------------------------------------------------
void AddVertsForCapsule2D(VertexList_PCU& verts,
                          Vec2 const&     capsuleStartPosition,
                          Vec2 const&     capsuleEndPosition,
                          float const     capsuleRadius,
                          Rgba8 const&    color)
{
    // 1. Calculate capsule's forward/normalized direction.
    Vec2 const forwardDirection    = capsuleEndPosition - capsuleStartPosition;
    Vec2 const normalizedDirection = forwardDirection.GetNormalized();

    // 2. Calculate capsule's corner positions.
    Vec2 const perpendicular90DegreesDirection = normalizedDirection.GetRotated90Degrees() * capsuleRadius;
    Vec3 const bottomLeft                      = Vec3(capsuleStartPosition.x + perpendicular90DegreesDirection.x, capsuleStartPosition.y + perpendicular90DegreesDirection.y, 0.f);
    Vec3 const bottomRight                     = Vec3(capsuleStartPosition.x - perpendicular90DegreesDirection.x, capsuleStartPosition.y - perpendicular90DegreesDirection.y, 0.f);
    Vec3 const topLeft                         = Vec3(capsuleEndPosition.x + perpendicular90DegreesDirection.x, capsuleEndPosition.y + perpendicular90DegreesDirection.y, 0.f);
    Vec3 const topRight                        = Vec3(capsuleEndPosition.x - perpendicular90DegreesDirection.x, capsuleEndPosition.y - perpendicular90DegreesDirection.y, 0.f);

    AddVertsForQuad3D(verts, bottomLeft, bottomRight, topLeft, topRight, color);

    // 3. Calculate halfDisc's rotation degrees and center start/end.
    float const halfDiscRotationDegrees = Atan2Degrees(-perpendicular90DegreesDirection.y, -perpendicular90DegreesDirection.x);
    Vec2 const& halfDiscCenterStart     = capsuleStartPosition;
    Vec2 const& halfDiscCenterEnd       = capsuleEndPosition;

    AddVertsForHalfDisc2D(verts, halfDiscCenterStart, capsuleRadius, false, halfDiscRotationDegrees, color);
    AddVertsForHalfDisc2D(verts, halfDiscCenterEnd, capsuleRadius, true, halfDiscRotationDegrees, color);
}

//----------------------------------------------------------------------------------------------------
void AddVertsForCapsule2D(VertexList_PCU& verts,
                          Capsule2 const& capsule,
                          Rgba8 const&    color)
{
    AddVertsForCapsule2D(verts, capsule.m_startPosition, capsule.m_endPosition, capsule.m_radius, color);
}

//----------------------------------------------------------------------------------------------------
void AddVertsForHalfDisc2D(VertexList_PCU& verts,
                           Vec2 const&     discCenter,
                           float const     discRadius,
                           bool const      isTopHalf,
                           float const     rotationDegrees,
                           Rgba8 const&    color)
{
    // 1. Calculate the degree of each triangle in the disc.
    int constexpr NUM_SIDES      = 32;
    float         degreesPerSide = 180.f / static_cast<float>(NUM_SIDES);

    // 2. If the disc is not topHalf ( 180-360 ), make it bottomHalf of the disc.
    if (!isTopHalf)
    {
        degreesPerSide = 360.f / static_cast<float>(NUM_SIDES);
    }

    for (int sideNum = 0; sideNum < NUM_SIDES; ++sideNum)
    {
        // 3. Get the degree of each triangle on the unit circle.
        float const startDegrees = degreesPerSide * static_cast<float>(sideNum) + rotationDegrees;
        float const endDegrees   = degreesPerSide * static_cast<float>(sideNum + 1) + rotationDegrees;
        float const cosStart     = CosDegrees(startDegrees);
        float const sinStart     = SinDegrees(startDegrees);
        float const cosEnd       = CosDegrees(endDegrees);
        float const sinEnd       = SinDegrees(endDegrees);

        // 4. Get the positions by ( discCenter ) + ( discRadius ) * ( cos / sin )
        Vec3 centerPosition(discCenter.x, discCenter.y, 0.f);
        Vec3 startOuterPosition(discCenter.x + discRadius * cosStart, discCenter.y + discRadius * sinStart, 0.f);
        Vec3 endOuterPosition(discCenter.x + discRadius * cosEnd, discCenter.y + discRadius * sinEnd, 0.f);

        // 5. Stores the vertices using counter-clockwise order.
        verts.emplace_back(centerPosition, color);
        verts.emplace_back(startOuterPosition, color);
        verts.emplace_back(endOuterPosition, color);
    }
}

//----------------------------------------------------------------------------------------------------
void AddVertsForArrow2D(VertexList_PCU& verts,
                        Vec2 const&     tailPosition,
                        Vec2 const&     tipPosition,
                        float const     arrowSize,
                        float const     thickness,
                        Rgba8 const&    color)
{
    // 1. Calculate arrow's forward/normalized direction.
    Vec2 const forwardDirection    = tipPosition - tailPosition;
    Vec2 const normalizedDirection = forwardDirection.GetNormalized();

    // 2. Calculate arrow's left and right direction.
    Vec2 const arrowLeftDirection  = normalizedDirection.GetRotatedDegrees(-45.f);
    Vec2 const arrowRightDirection = normalizedDirection.GetRotatedDegrees(45.f);

    // 3. Get arrow's left and right position by subtract ( direction ) * ( arrowSize).
    Vec2 const leftArrowPosition  = tipPosition - arrowLeftDirection * arrowSize;
    Vec2 const rightArrowPosition = tipPosition - arrowRightDirection * arrowSize;

    // 4. Adjust the tipPosition to touch the endpoint.
    Vec2 const tipAdjustment = normalizedDirection * thickness * 0.35f;

    AddVertsForLineSegment2D(verts, tailPosition, tipPosition + tipAdjustment, thickness, false, color);
    AddVertsForLineSegment2D(verts, leftArrowPosition, tipPosition, thickness, false, color);
    AddVertsForLineSegment2D(verts, rightArrowPosition, tipPosition, thickness, false, color);
}

//----------------------------------------------------------------------------------------------------
void AddVertsForQuad3D(VertexList_PCU& verts,
                       Vec3 const&     bottomLeft,
                       Vec3 const&     bottomRight,
                       Vec3 const&     topLeft,
                       Vec3 const&     topRight,
                       Rgba8 const&    color,
                       AABB2 const&    UVs)
{
    // Starting at BL, add triangle A with vertexes BL, BR, TR.
    verts.emplace_back(bottomLeft, color, Vec2(UVs.m_mins.x, UVs.m_mins.y));
    verts.emplace_back(bottomRight, color, Vec2(UVs.m_maxs.x, UVs.m_mins.y));
    verts.emplace_back(topRight, color, Vec2(UVs.m_maxs.x, UVs.m_maxs.y));

    // Starting again at BL, add triangle B with vertexes BL, TR, TL.
    verts.emplace_back(bottomLeft, color, Vec2(UVs.m_mins.x, UVs.m_mins.y));
    verts.emplace_back(topRight, color, Vec2(UVs.m_maxs.x, UVs.m_maxs.y));
    verts.emplace_back(topLeft, color, Vec2(UVs.m_mins.x, UVs.m_maxs.y));
}

void AddVertsForQuad3D(VertexList_PCU& verts,
                       IndexList&      indexes,
                       Vec3 const&     bottomLeft,
                       Vec3 const&     bottomRight,
                       Vec3 const&     topLeft,
                       Vec3 const&     topRight,
                       Rgba8 const&    color,
                       AABB2 const&    UVs)
{
    // 1. Store the starting index of this quad's vertices in the vertex list.
    unsigned int const currentIndex = static_cast<unsigned int>(verts.size());

    // 5. Add the four vertices of the quad with position, color, UV, and TBN data.
    verts.emplace_back(bottomLeft, color, UVs.m_mins);                                      // Bottom-left
    verts.emplace_back(bottomRight, color, Vec2(UVs.m_maxs.x, UVs.m_mins.y));    // Bottom-right
    verts.emplace_back(topRight, color, UVs.m_maxs);                                        // Top-right
    verts.emplace_back(topLeft, color, Vec2(UVs.m_mins.x, UVs.m_maxs.y));        // Top-left

    // 6.Define two triangles using the four vertices, ordered counter-clockwise.
    indexes.push_back(currentIndex);            // Triangle 1: bottom-left
    indexes.push_back(currentIndex + 1);    //             bottom-right
    indexes.push_back(currentIndex + 2);    //             top-right

    indexes.push_back(currentIndex);            // Triangle 2: bottom-left
    indexes.push_back(currentIndex + 2);    //             top-right
    indexes.push_back(currentIndex + 3);    //             top-left
}


void AddVertsForQuad3D(VertexList_PCUTBN& verts,
                       Vec3 const&        bottomLeft,
                       Vec3 const&        bottomRight,
                       Vec3 const&        topLeft,
                       Vec3 const&        topRight,
                       Rgba8 const&       color,
                       AABB2 const&       UVs)
{
    // 1. Compute the tangent vector as the direction from bottom-left to bottom-right.
    Vec3 const tangent = (bottomRight - bottomLeft).GetNormalized();

    // 2. Compute the bitangent vector as the direction from bottom-left to top-left.
    Vec3 const bitangent = (topLeft - bottomLeft).GetNormalized();

    // 3. Compute the normal vector as the cross product of tangent and bitangent.
    // This assumes right-handed coordinate system (T x B = N).
    Vec3 const normal = CrossProduct3D(tangent, bitangent).GetNormalized();

    // 4. Add the four vertices of the quad with position, color, UV, and TBN data.
    verts.emplace_back(bottomLeft, color, UVs.m_mins, tangent, bitangent, normal);                                      // Bottom-left
    verts.emplace_back(bottomRight, color, Vec2(UVs.m_maxs.x, UVs.m_mins.y), tangent, bitangent, normal);    // Bottom-right
    verts.emplace_back(topRight, color, UVs.m_maxs, tangent, bitangent, normal);                                        // Top-right
    verts.emplace_back(topLeft, color, Vec2(UVs.m_mins.x, UVs.m_maxs.y), tangent, bitangent, normal);        // Top-left
}

//----------------------------------------------------------------------------------------------------
/// @brief Adds a 3D quad to the provided vertex and index lists, with position, color, UVs, and TBN vectors.
///
/// @param verts       The output vertex list to which new vertices will be added.
/// @param indexes     The output index list to which new triangle indices will be added.
/// @param bottomLeft  The bottom-left corner of the quad in 3D space.
/// @param bottomRight The bottom-right corner of the quad in 3D space.
/// @param topLeft     The top-left corner of the quad in 3D space.
/// @param topRight    The top-right corner of the quad in 3D space.
/// @param color       The color to apply to all vertices of the quad.
/// @param UVs         The UV coordinates (as a 2D bounding box) to map onto the quad.
void AddVertsForQuad3D(VertexList_PCUTBN& verts,
                       IndexList&         indexes,
                       Vec3 const&        bottomLeft,
                       Vec3 const&        bottomRight,
                       Vec3 const&        topLeft,
                       Vec3 const&        topRight,
                       Rgba8 const&       color,
                       AABB2 const&       UVs)
{
    // 1. Store the starting index of this quad's vertices in the vertex list.
    unsigned int const currentIndex = static_cast<unsigned int>(verts.size());

    // 2. Compute the tangent vector as the direction from bottom-left to bottom-right.
    Vec3 const tangent = (bottomRight - bottomLeft).GetNormalized();

    // 3. Compute the bitangent vector as the direction from bottom-left to top-left.
    Vec3 const bitangent = (topLeft - bottomLeft).GetNormalized();

    // 4. Compute the normal vector as the cross product of tangent and bitangent.
    // This assumes right-handed coordinate system (T x B = N).
    Vec3 const normal = CrossProduct3D(tangent, bitangent).GetNormalized();

    // 5. Add the four vertices of the quad with position, color, UV, and TBN data.
    verts.emplace_back(bottomLeft, color, UVs.m_mins, tangent, bitangent, normal);                                      // Bottom-left
    verts.emplace_back(bottomRight, color, Vec2(UVs.m_maxs.x, UVs.m_mins.y), tangent, bitangent, normal);    // Bottom-right
    verts.emplace_back(topRight, color, UVs.m_maxs, tangent, bitangent, normal);                                        // Top-right
    verts.emplace_back(topLeft, color, Vec2(UVs.m_mins.x, UVs.m_maxs.y), tangent, bitangent, normal);        // Top-left

    // 6.Define two triangles using the four vertices, ordered counter-clockwise.
    indexes.push_back(currentIndex);            // Triangle 1: bottom-left
    indexes.push_back(currentIndex + 1);    //             bottom-right
    indexes.push_back(currentIndex + 2);    //             top-right

    indexes.push_back(currentIndex);            // Triangle 2: bottom-left
    indexes.push_back(currentIndex + 2);    //             top-right
    indexes.push_back(currentIndex + 3);    //             top-left
}

void AddVertsForRoundedQuad3D(VertexList_PCUTBN& verts,
                              Vec3 const&        topLeft,
                              Vec3 const&        bottomLeft,
                              Vec3 const&        bottomRight,
                              Vec3 const&        topRight,
                              Rgba8 const&       color,
                              AABB2 const&       UVs)
{
    Vec3 middleTop    = (topRight + topLeft) * 0.5f;
    Vec3 middleBottom = (bottomRight + bottomLeft) * 0.5f;
    Vec3 normal       = CrossProduct3D(bottomRight - bottomLeft, topLeft - bottomLeft).GetNormalized();
    verts.emplace_back(bottomLeft, color, UVs.m_mins, Vec3::ZERO, Vec3::ZERO, (bottomLeft - bottomRight).GetNormalized());
    verts.emplace_back(middleBottom, color, Vec2((UVs.m_mins.x + UVs.m_maxs.x) * 0.5f, UVs.m_mins.y), Vec3::ZERO, Vec3::ZERO, normal);
    verts.emplace_back(middleTop, color, Vec2((UVs.m_mins.x + UVs.m_maxs.x) * 0.5f, UVs.m_maxs.y), Vec3::ZERO, Vec3::ZERO, normal);
    verts.emplace_back(bottomLeft, color, UVs.m_mins, Vec3::ZERO, Vec3::ZERO, (bottomLeft - bottomRight).GetNormalized());
    verts.emplace_back(middleTop, color, Vec2((UVs.m_mins.x + UVs.m_maxs.x) * 0.5f, UVs.m_maxs.y), Vec3::ZERO, Vec3::ZERO, normal);
    verts.emplace_back(topLeft, color, Vec2(UVs.m_mins.x, UVs.m_maxs.y), Vec3::ZERO, Vec3::ZERO, (topLeft - topRight).GetNormalized());


    verts.emplace_back(middleBottom, color, Vec2((UVs.m_mins.x + UVs.m_maxs.x) * 0.5f, UVs.m_mins.y), Vec3::ZERO, Vec3::ZERO, normal);
    verts.emplace_back(bottomRight, color, Vec2(UVs.m_maxs.x, UVs.m_mins.y), Vec3::ZERO, Vec3::ZERO, (bottomRight - bottomLeft).GetNormalized());
    verts.emplace_back(topRight, color, UVs.m_maxs, Vec3::ZERO, Vec3::ZERO, (topRight - topLeft).GetNormalized());
    verts.emplace_back(middleBottom, color, Vec2((UVs.m_mins.x + UVs.m_maxs.x) * 0.5f, UVs.m_mins.y), Vec3::ZERO, Vec3::ZERO, normal);
    verts.emplace_back(topRight, color, UVs.m_maxs, Vec3::ZERO, Vec3::ZERO, (topRight - topLeft).GetNormalized());
    verts.emplace_back(middleTop, color, Vec2((UVs.m_mins.x + UVs.m_maxs.x) * 0.5f, UVs.m_maxs.y), Vec3::ZERO, Vec3::ZERO, normal);
}

//----------------------------------------------------------------------------------------------------
void AddVertsForWireframeQuad3D(VertexList_PCU& verts,
                                Vec3 const&     bottomLeft,
                                Vec3 const&     bottomRight,
                                Vec3 const&     topLeft,
                                Vec3 const&     topRight,
                                float const     thickness,
                                Rgba8 const&    color,
                                AABB2 const&    UVs)
{
    AddVertsForCylinder3D(verts, bottomLeft, bottomRight, thickness, color, UVs, 4);
    AddVertsForCylinder3D(verts, bottomRight, topRight, thickness, color, UVs, 4);
    AddVertsForCylinder3D(verts, topLeft, bottomLeft, thickness, color, UVs, 4);
    AddVertsForCylinder3D(verts, topRight, topLeft, thickness, color, UVs, 4);
}

//----------------------------------------------------------------------------------------------------
void AddVertsForAABB3D(VertexList_PCU& verts,
                       AABB3 const&    bounds,
                       Rgba8 const&    color,
                       AABB2 const&    UVs)
{
    Vec3 const min = bounds.m_mins;
    Vec3 const max = bounds.m_maxs;

    Vec3 const frontBottomLeft  = Vec3(max.x, min.y, min.z);
    Vec3 const frontBottomRight = Vec3(max.x, max.y, min.z);
    Vec3 const frontTopLeft     = Vec3(max.x, min.y, max.z);
    Vec3 const frontTopRight    = Vec3(max.x, max.y, max.z);

    Vec3 const backBottomLeft  = Vec3(min.x, max.y, min.z);
    Vec3 const backBottomRight = Vec3(min.x, min.y, min.z);
    Vec3 const backTopLeft     = Vec3(min.x, max.y, max.z);
    Vec3 const backTopRight    = Vec3(min.x, min.y, max.z);

    float const uvWidth  = UVs.m_maxs.x - UVs.m_mins.x;
    float const uvHeight = UVs.m_maxs.y - UVs.m_mins.y;

    AABB2 const uv(Vec2(UVs.m_mins.x, UVs.m_mins.y), Vec2(UVs.m_mins.x + uvWidth, UVs.m_mins.y + uvHeight));

    AddVertsForQuad3D(verts, frontBottomLeft, frontBottomRight, frontTopLeft, frontTopRight, color, uv);        // Front
    AddVertsForQuad3D(verts, backBottomLeft, backBottomRight, backTopLeft, backTopRight, color, uv);            // Back
    AddVertsForQuad3D(verts, backBottomRight, frontBottomLeft, backTopRight, frontTopLeft, color, uv);          // Left
    AddVertsForQuad3D(verts, frontBottomRight, backBottomLeft, frontTopRight, backTopLeft, color, uv);          // Right
    AddVertsForQuad3D(verts, frontTopLeft, frontTopRight, backTopRight, backTopLeft, color, uv);                // Top
    AddVertsForQuad3D(verts, backBottomRight, backBottomLeft, frontBottomLeft, frontBottomRight, color, uv);    // Bottom
}

//----------------------------------------------------------------------------------------------------
void AddVertsForAABB3D(VertexList_PCUTBN& verts,
                       IndexList&         indexes,
                       AABB3 const&       bounds,
                       Rgba8 const&       color,
                       AABB2 const&       UVs)
{
    Vec3 const min = bounds.m_mins;
    Vec3 const max = bounds.m_maxs;

    Vec3 const frontBottomLeft  = Vec3(max.x, min.y, min.z);
    Vec3 const frontBottomRight = Vec3(max.x, max.y, min.z);
    Vec3 const frontTopLeft     = Vec3(max.x, min.y, max.z);
    Vec3 const frontTopRight    = Vec3(max.x, max.y, max.z);

    Vec3 const backBottomLeft  = Vec3(min.x, max.y, min.z);
    Vec3 const backBottomRight = Vec3(min.x, min.y, min.z);
    Vec3 const backTopLeft     = Vec3(min.x, max.y, max.z);
    Vec3 const backTopRight    = Vec3(min.x, min.y, max.z);

    float const uvWidth  = UVs.m_maxs.x - UVs.m_mins.x;
    float const uvHeight = UVs.m_maxs.y - UVs.m_mins.y;

    AABB2 const uv(Vec2(UVs.m_mins.x, UVs.m_mins.y), Vec2(UVs.m_mins.x + uvWidth, UVs.m_mins.y + uvHeight));

    AddVertsForQuad3D(verts, indexes, frontBottomRight, backBottomLeft, frontTopRight, backTopLeft, color, uv);         // Front
    AddVertsForQuad3D(verts, indexes, backBottomRight, frontBottomLeft, backTopRight, frontTopLeft, color, uv);         // Back
    AddVertsForQuad3D(verts, indexes, backBottomLeft, backBottomRight, backTopLeft, backTopRight, color, uv);           // Left
    AddVertsForQuad3D(verts, indexes, frontBottomLeft, frontBottomRight, frontTopLeft, frontTopRight, color, uv);        // Right
    AddVertsForQuad3D(verts, indexes, backTopRight, frontTopLeft, backTopLeft, frontTopRight, color, uv);               // Top
    AddVertsForQuad3D(verts, indexes, backBottomLeft, frontBottomRight, backBottomRight, frontBottomLeft, color, uv);      // Bottom
}

//----------------------------------------------------------------------------------------------------
void AddVertsForWireframeAABB3D(VertexList_PCU& verts,
                                AABB3 const&    bounds,
                                float const     thickness,
                                Rgba8 const&    color,
                                AABB2 const&    UVs)
{
    Vec3 const min = bounds.m_mins;
    Vec3 const max = bounds.m_maxs;

    Vec3 const frontBottomLeft  = Vec3(max.x, min.y, min.z);
    Vec3 const frontBottomRight = Vec3(max.x, max.y, min.z);
    Vec3 const frontTopLeft     = Vec3(max.x, min.y, max.z);
    Vec3 const frontTopRight    = Vec3(max.x, max.y, max.z);

    Vec3 const backBottomLeft  = Vec3(min.x, max.y, min.z);
    Vec3 const backBottomRight = Vec3(min.x, min.y, min.z);
    Vec3 const backTopLeft     = Vec3(min.x, max.y, max.z);
    Vec3 const backTopRight    = Vec3(min.x, min.y, max.z);

    float const uvWidth  = UVs.m_maxs.x - UVs.m_mins.x;
    float const uvHeight = UVs.m_maxs.y - UVs.m_mins.y;

    AABB2 const uv(Vec2(UVs.m_mins.x, UVs.m_mins.y), Vec2(UVs.m_mins.x + uvWidth, UVs.m_mins.y + uvHeight));

    AddVertsForWireframeQuad3D(verts, frontBottomLeft, frontBottomRight, frontTopLeft, frontTopRight, thickness, color, uv);        // Front
    AddVertsForWireframeQuad3D(verts, backBottomLeft, backBottomRight, backTopLeft, backTopRight, thickness, color, uv);            // Back
    AddVertsForWireframeQuad3D(verts, backBottomRight, frontBottomLeft, backTopRight, frontTopLeft, thickness, color, uv);          // Left
    AddVertsForWireframeQuad3D(verts, frontBottomRight, backBottomLeft, frontTopRight, backTopLeft, thickness, color, uv);          // Right
    AddVertsForWireframeQuad3D(verts, frontTopLeft, frontTopRight, backTopRight, backTopLeft, thickness, color, uv);                // Top
    AddVertsForWireframeQuad3D(verts, backBottomRight, backBottomLeft, frontBottomLeft, frontBottomRight, thickness, color, uv);    // Bottom
}

//----------------------------------------------------------------------------------------------------
void AddVertsForSphere3D(VertexList_PCU& verts,
                         Vec3 const&     center,
                         float const     radius,
                         Rgba8 const&    color,
                         AABB2 const&    UVs,
                         int const       numSlices,
                         int const       numStacks)
{
    float const uvWidth  = UVs.m_maxs.x - UVs.m_mins.x;
    float const uvHeight = UVs.m_maxs.y - UVs.m_mins.y;

    for (int stack = 0; stack < numStacks; ++stack)
    {
        float const phi1 = (1.f - static_cast<float>(stack) / static_cast<float>(numStacks)) * PI;
        float const phi2 = (1.f - (static_cast<float>(stack) + 1.f) / static_cast<float>(numStacks)) * PI;

        float const v1 = static_cast<float>(stack) / static_cast<float>(numStacks);
        float const v2 = (static_cast<float>(stack) + 1.f) / static_cast<float>(numStacks);

        for (int slice = 0; slice < numSlices; ++slice)
        {
            float const theta1 = static_cast<float>(slice) / static_cast<float>(numSlices) * 2.f * PI;
            float const theta2 = (static_cast<float>(slice) + 1.f) / static_cast<float>(numSlices) * 2.f * PI;

            float const u1 = static_cast<float>(slice) / static_cast<float>(numSlices);
            float const u2 = (static_cast<float>(slice) + 1.f) / static_cast<float>(numSlices);

            Vec3 bottomLeft  = center + Vec3(radius * sinf(phi1) * cosf(theta1), radius * sinf(phi1) * sinf(theta1), radius * cosf(phi1));
            Vec3 bottomRight = center + Vec3(radius * sinf(phi1) * cosf(theta2), radius * sinf(phi1) * sinf(theta2), radius * cosf(phi1));
            Vec3 topRight    = center + Vec3(radius * sinf(phi2) * cosf(theta2), radius * sinf(phi2) * sinf(theta2), radius * cosf(phi2));
            Vec3 topLeft     = center + Vec3(radius * sinf(phi2) * cosf(theta1), radius * sinf(phi2) * sinf(theta1), radius * cosf(phi2));

            AABB2 quadUV(Vec2(UVs.m_mins.x + uvWidth * u1, UVs.m_mins.y + uvHeight * v1),
                         Vec2(UVs.m_mins.x + uvWidth * u2, UVs.m_mins.y + uvHeight * v2));

            AddVertsForQuad3D(verts, bottomLeft, bottomRight, topLeft, topRight, color, quadUV);
        }
    }
}

void AddVertsForSphere3D(VertexList_PCUTBN& verts,
                         IndexList&         indexes,
                         Vec3 const&        center,
                         float const        radius,
                         Rgba8 const&       color,
                         AABB2 const&       UVs,
                         int const          numSlices,
                         int const          numStacks)
{
    float const uvWidth  = UVs.m_maxs.x - UVs.m_mins.x;
    float const uvHeight = UVs.m_maxs.y - UVs.m_mins.y;

    std::vector<std::vector<unsigned int>> vertIndexGrid(numStacks + 1, std::vector<unsigned int>(numSlices + 1));

    for (int stack = 0; stack <= numStacks; ++stack)
    {
        float phi = (1.f - static_cast<float>(stack) / static_cast<float>(numStacks)) * PI;
        float v   = static_cast<float>(stack) / static_cast<float>(numStacks);

        for (int slice = 0; slice <= numSlices; ++slice)
        {
            float theta = static_cast<float>(slice) / static_cast<float>(numSlices) * 2.f * PI;
            float u     = static_cast<float>(slice) / static_cast<float>(numSlices);

            // 球面座標轉直角座標
            Vec3 normal = Vec3(sinf(phi) * cosf(theta),
                               sinf(phi) * sinf(theta),
                               cosf(phi));

            Vec3 position = center + radius * normal;
            Vec2 uv       = Vec2(UVs.m_mins.x + uvWidth * u, UVs.m_mins.y + uvHeight * v);

            // 計算 Tangent vector (沿 U 方向，即沿著緯線方向)
            // dP/du = radius * sinf(phi) * (-sinf(theta), cosf(theta), 0)
            Vec3 tangent = Vec3(-sinf(theta), cosf(theta), 0.0f).GetNormalized();

            // 計算 Bitangent vector (沿 V 方向，即沿著經線方向)
            // dP/dv = radius * (cosf(phi) * cosf(theta), cosf(phi) * sinf(theta), -sinf(phi))
            Vec3 bitangent = Vec3(cosf(phi) * cosf(theta),
                                  cosf(phi) * sinf(theta),
                                  -sinf(phi)).GetNormalized();

            // 驗證 TBN 是否形成右手坐標系 (可選的調試代碼)
            // Vec3 crossProduct = CrossProduct3D(tangent, bitangent);
            // float dotWithNormal = DotProduct3D(crossProduct, normal);
            // if (dotWithNormal < 0.0f)
            // {
            //     bitangent = -bitangent; // 如果不是右手坐標系，翻轉 bitangent
            // }

            vertIndexGrid[stack][slice] = static_cast<unsigned int>(verts.size());
            verts.emplace_back(position, color, uv, tangent, bitangent, normal);
        }
    }

    // 建立 index（三角形）
    for (int stack = 0; stack < numStacks; ++stack)
    {
        for (int slice = 0; slice < numSlices; ++slice)
        {
            unsigned int i0 = vertIndexGrid[stack][slice];
            unsigned int i1 = vertIndexGrid[stack][slice + 1];
            unsigned int i2 = vertIndexGrid[stack + 1][slice + 1];
            unsigned int i3 = vertIndexGrid[stack + 1][slice];

            indexes.push_back(i0);
            indexes.push_back(i1);
            indexes.push_back(i2);

            indexes.push_back(i0);
            indexes.push_back(i2);
            indexes.push_back(i3);
        }
    }
}

void AddVertsForWireframeSphere3D(VertexList_PCU& verts,
                                  Vec3 const&     center,
                                  float const     radius,
                                  float const     thickness,
                                  Rgba8 const&    color,
                                  AABB2 const&    UVs,
                                  int const       numSlices,
                                  int const       numStacks)
{
    float const uvWidth  = UVs.m_maxs.x - UVs.m_mins.x;
    float const uvHeight = UVs.m_maxs.y - UVs.m_mins.y;

    for (int stack = 0; stack < numStacks; ++stack)
    {
        float const phi1 = (1.f - static_cast<float>(stack) / static_cast<float>(numStacks)) * PI;
        float const phi2 = (1.f - (static_cast<float>(stack) + 1.f) / static_cast<float>(numStacks)) * PI;

        float const v1 = static_cast<float>(stack) / static_cast<float>(numStacks);
        float const v2 = (static_cast<float>(stack) + 1.f) / static_cast<float>(numStacks);

        for (int slice = 0; slice < numSlices; ++slice)
        {
            float const theta1 = static_cast<float>(slice) / static_cast<float>(numSlices) * 2.f * PI;
            float const theta2 = (static_cast<float>(slice) + 1.f) / static_cast<float>(numSlices) * 2.f * PI;

            float const u1 = static_cast<float>(slice) / static_cast<float>(numSlices);
            float const u2 = (static_cast<float>(slice) + 1.f) / static_cast<float>(numSlices);

            Vec3 bottomLeft  = Vec3(radius * sinf(phi1) * cosf(theta1), radius * sinf(phi1) * sinf(theta1), radius * cosf(phi1));
            Vec3 bottomRight = Vec3(radius * sinf(phi1) * cosf(theta2), radius * sinf(phi1) * sinf(theta2), radius * cosf(phi1));
            Vec3 topRight    = Vec3(radius * sinf(phi2) * cosf(theta2), radius * sinf(phi2) * sinf(theta2), radius * cosf(phi2));
            Vec3 topLeft     = Vec3(radius * sinf(phi2) * cosf(theta1), radius * sinf(phi2) * sinf(theta1), radius * cosf(phi2));

            AABB2 quadUV(Vec2(UVs.m_mins.x + uvWidth * u1, UVs.m_mins.y + uvHeight * v1),
                         Vec2(UVs.m_mins.x + uvWidth * u2, UVs.m_mins.y + uvHeight * v2));

            AddVertsForWireframeQuad3D(verts, center + bottomLeft, center + bottomRight, center + topLeft, center + topRight, thickness, color, quadUV);
        }
    }
}

//----------------------------------------------------------------------------------------------------
void AddVertsForCylinder3D(VertexList_PCU& verts,
                           Vec3 const&     startPosition,
                           Vec3 const&     endPosition,
                           float const     radius,
                           Rgba8 const&    color,
                           AABB2 const&    UVs,
                           int const       numSlices)
{
    // 1. Calculate cylinder's forward/normalized direction.
    Vec3 const forwardDirection = endPosition - startPosition;
    Vec3 const iBasis           = forwardDirection.GetNormalized();

    // 2. Get jBasis and kBasis based on normalDirection.
    Vec3 jBasis;
    Vec3 kBasis;
    iBasis.GetOrthonormalBasis(iBasis, &jBasis, &kBasis);

    // 3. Calculate the degree of each triangle in the top and bottom disc of the cylinder.
    float const DEGREES_PER_SIDE = 360.f / static_cast<float>(numSlices);

    for (int sideIndex = 0; sideIndex < numSlices; ++sideIndex)
    {
        // 4. Get the degree of each triangle on the unit circle.
        float const startDegrees = DEGREES_PER_SIDE * static_cast<float>(sideIndex);
        float const endDegrees   = DEGREES_PER_SIDE * static_cast<float>(sideIndex + 1);
        float const cosStart     = CosDegrees(startDegrees);
        float const sinStart     = SinDegrees(startDegrees);
        float const cosEnd       = CosDegrees(endDegrees);
        float const sinEnd       = SinDegrees(endDegrees);

        // 5. Get the positions by ( discCenter ) + ( radius ) * ( cos * jBasis + sin * kBasis )
        Vec3 topCenterPosition(endPosition);
        Vec3 topLeftPosition(endPosition + radius * (cosStart * jBasis + sinStart * kBasis));
        Vec3 topRightPosition(endPosition + radius * (cosEnd * jBasis + sinEnd * kBasis));
        Vec3 bottomCenterPosition(startPosition);
        Vec3 bottomLeftPosition(startPosition + radius * (-cosStart * -jBasis + -sinStart * -kBasis));
        Vec3 bottomRightPosition(startPosition + radius * (-cosEnd * -jBasis + -sinEnd * -kBasis));

        // 6. Stores the vertices using counter-clockwise order with correct UVs.
        verts.emplace_back(topCenterPosition, color, Vec2::HALF);
        verts.emplace_back(topLeftPosition, color, Vec2::MakeFromPolarDegrees(startDegrees, 0.5f) + Vec2::HALF);
        verts.emplace_back(topRightPosition, color, Vec2::MakeFromPolarDegrees(endDegrees, 0.5f) + Vec2::HALF);
        verts.emplace_back(bottomCenterPosition, color, Vec2::HALF);
        verts.emplace_back(bottomRightPosition, color, Vec2::MakeFromPolarDegrees(-endDegrees, 0.5f) + Vec2::HALF);
        verts.emplace_back(bottomLeftPosition, color, Vec2::MakeFromPolarDegrees(-startDegrees, 0.5f) + Vec2::HALF);

        // 7. Add verts for cylinder's side with correct UVs.
        float const uStart = Interpolate(UVs.m_mins.x, UVs.m_maxs.x, static_cast<float>(sideIndex) / static_cast<float>(numSlices));
        float const uEnd   = Interpolate(UVs.m_mins.x, UVs.m_maxs.x, static_cast<float>(sideIndex + 1) / static_cast<float>(numSlices));

        AddVertsForQuad3D(verts, bottomLeftPosition, bottomRightPosition, topLeftPosition, topRightPosition, color, AABB2(Vec2(uStart, 0.f), Vec2(uEnd, 1.f)));
    }
}

void AddVertsForCylinder3D(VertexList_PCUTBN& verts,
                           IndexList&         indexes,
                           Vec3 const&        startPosition,
                           Vec3 const&        endPosition,
                           float const        radius,
                           Rgba8 const&       color,
                           AABB2 const&       UVs,
                           int const          numSlices)
{
    Vec3 const forwardDirection = endPosition - startPosition;
    Vec3 const iBasis           = forwardDirection.GetNormalized();

    Vec3 jBasis;
    Vec3 kBasis;
    iBasis.GetOrthonormalBasis(iBasis, &jBasis, &kBasis);

    float const DEGREES_PER_SIDE = 360.f / static_cast<float>(numSlices);

    for (int sideIndex = 0; sideIndex < numSlices; ++sideIndex)
    {
        float const startDegrees = DEGREES_PER_SIDE * static_cast<float>(sideIndex);
        float const endDegrees   = DEGREES_PER_SIDE * static_cast<float>(sideIndex + 1);
        float const cosStart     = CosDegrees(startDegrees);
        float const sinStart     = SinDegrees(startDegrees);
        float const cosEnd       = CosDegrees(endDegrees);
        float const sinEnd       = SinDegrees(endDegrees);

        Vec3 topCenterPosition(endPosition);
        Vec3 bottomCenterPosition(startPosition);

        Vec3 topLeftPosition     = topCenterPosition + radius * (cosStart * jBasis + sinStart * kBasis);
        Vec3 topRightPosition    = topCenterPosition + radius * (cosEnd * jBasis + sinEnd * kBasis);
        Vec3 bottomLeftPosition  = bottomCenterPosition + radius * (cosStart * jBasis + sinStart * kBasis);
        Vec3 bottomRightPosition = bottomCenterPosition + radius * (cosEnd * jBasis + sinEnd * kBasis);

        // ===== 上蓋 =====
        Vec3 topNormal    = iBasis;
        Vec3 topTangent   = Vec3::X_BASIS;
        Vec3 topBitangent = Vec3::Y_BASIS;

        Vec2 uvTopLeft  = Vec2::MakeFromPolarDegrees(startDegrees, 0.5f) + Vec2::HALF;
        Vec2 uvTopRight = Vec2::MakeFromPolarDegrees(endDegrees, 0.5f) + Vec2::HALF;

        verts.emplace_back(topCenterPosition, color, Vec2::HALF, topTangent, topBitangent, topNormal);
        verts.emplace_back(topLeftPosition, color, uvTopLeft, topTangent, topBitangent, topNormal);
        verts.emplace_back(topRightPosition, color, uvTopRight, topTangent, topBitangent, topNormal);

        // ===== 下蓋 =====
        Vec3 bottomNormal    = -iBasis;
        Vec3 bottomBitangent = jBasis;
        Vec3 bottomTangent   = -kBasis;

        Vec2 uvBottomRight = Vec2::MakeFromPolarDegrees(-endDegrees, 0.5f) + Vec2::HALF;
        Vec2 uvBottomLeft  = Vec2::MakeFromPolarDegrees(-startDegrees, 0.5f) + Vec2::HALF;

        verts.emplace_back(bottomCenterPosition, color, Vec2::HALF, bottomTangent, bottomBitangent, bottomNormal);
        verts.emplace_back(bottomRightPosition, color, uvBottomRight, bottomTangent, bottomBitangent, bottomNormal);
        verts.emplace_back(bottomLeftPosition, color, uvBottomLeft, bottomTangent, bottomBitangent, bottomNormal);

        int baseIndex = static_cast<int>(verts.size()) - 6;
        indexes.push_back(baseIndex + 0);
        indexes.push_back(baseIndex + 1);
        indexes.push_back(baseIndex + 2);

        indexes.push_back(baseIndex + 3);
        indexes.push_back(baseIndex + 4);
        indexes.push_back(baseIndex + 5);

        // ===== 側面（使用從柱心 outward 的 normal）=====
        Vec3 centerTop    = topCenterPosition;
        Vec3 centerBottom = bottomCenterPosition;

        Vec3 normalBL = (bottomLeftPosition - (centerBottom + centerTop) * 0.5f).GetNormalized();
        Vec3 normalBR = (bottomRightPosition - (centerBottom + centerTop) * 0.5f).GetNormalized();
        Vec3 normalTL = (topLeftPosition - (centerBottom + centerTop) * 0.5f).GetNormalized();
        Vec3 normalTR = (topRightPosition - (centerBottom + centerTop) * 0.5f).GetNormalized();

        float uStart = Interpolate(UVs.m_mins.x, UVs.m_maxs.x, static_cast<float>(sideIndex) / static_cast<float>(numSlices));
        float uEnd   = Interpolate(UVs.m_mins.x, UVs.m_maxs.x, static_cast<float>(sideIndex + 1) / static_cast<float>(numSlices));
        float vMin   = UVs.m_mins.y;
        float vMax   = UVs.m_maxs.y;

        // 自行添加側面的 tangent/bitangent 可選用 jBasis/kBasis 為參考
        Vec3 tangent   = jBasis;
        Vec3 bitangent = kBasis;

        int sideVertStartIndex = static_cast<int>(verts.size());
        verts.emplace_back(bottomLeftPosition, color, Vec2(uStart, vMin), tangent, bitangent, normalBL);
        verts.emplace_back(bottomRightPosition, color, Vec2(uEnd, vMin), tangent, bitangent, normalBR);
        verts.emplace_back(topLeftPosition, color, Vec2(uStart, vMax), tangent, bitangent, normalTL);
        verts.emplace_back(topRightPosition, color, Vec2(uEnd, vMax), tangent, bitangent, normalTR);

        indexes.push_back(sideVertStartIndex + 0);
        indexes.push_back(sideVertStartIndex + 1);
        indexes.push_back(sideVertStartIndex + 2);

        indexes.push_back(sideVertStartIndex + 2);
        indexes.push_back(sideVertStartIndex + 1);
        indexes.push_back(sideVertStartIndex + 3);
    }
}

//----------------------------------------------------------------------------------------------------
void AddVertsForWireframeCylinder3D(VertexList_PCU& verts,
                                    Vec3 const&     startPosition,
                                    Vec3 const&     endPosition,
                                    float const     radius,
                                    float const     thickness,
                                    Rgba8 const&    color,
                                    AABB2 const&    UVs,
                                    int const       numSlices)
{
    // 1. Calculate cylinder's forward/normalized direction.
    Vec3 const forwardDirection = endPosition - startPosition;
    Vec3 const iBasis           = forwardDirection.GetNormalized();

    // 2. Get jBasis and kBasis based on normalDirection.
    Vec3 jBasis;
    Vec3 kBasis;
    iBasis.GetOrthonormalBasis(iBasis, &jBasis, &kBasis);

    // 3. Calculate the degree of each triangle in the top and bottom disc of the cylinder.
    float const DEGREES_PER_SIDE = 360.f / static_cast<float>(numSlices);

    for (int sideIndex = 0; sideIndex < numSlices; ++sideIndex)
    {
        // 4. Get the degree of each triangle on the unit circle.
        float const startDegrees = DEGREES_PER_SIDE * static_cast<float>(sideIndex);
        float const endDegrees   = DEGREES_PER_SIDE * static_cast<float>(sideIndex + 1);
        float const cosStart     = CosDegrees(startDegrees);
        float const sinStart     = SinDegrees(startDegrees);
        float const cosEnd       = CosDegrees(endDegrees);
        float const sinEnd       = SinDegrees(endDegrees);

        // 5. Get the positions by ( discCenter ) + ( radius ) * ( cos * jBasis + sin * kBasis )
        Vec3 topCenterPosition(endPosition);
        Vec3 topLeftPosition(endPosition + radius * (cosStart * jBasis + sinStart * kBasis));
        Vec3 topRightPosition(endPosition + radius * (cosEnd * jBasis + sinEnd * kBasis));
        Vec3 bottomCenterPosition(startPosition);
        Vec3 bottomLeftPosition(startPosition + radius * (-cosStart * -jBasis + -sinStart * -kBasis));
        Vec3 bottomRightPosition(startPosition + radius * (-cosEnd * -jBasis + -sinEnd * -kBasis));

        // 6. Stores the vertices using counter-clockwise order with correct UVs.
        // verts.emplace_back(topCenterPosition, color, Vec2::HALF);
        // verts.emplace_back(topLeftPosition, color, Vec2::MakeFromPolarDegrees(startDegrees, 0.5f) + Vec2::HALF);
        // verts.emplace_back(topRightPosition, color, Vec2::MakeFromPolarDegrees(endDegrees, 0.5f) + Vec2::HALF);
        // verts.emplace_back(bottomCenterPosition, color, Vec2::HALF);
        // verts.emplace_back(bottomRightPosition, color, Vec2::MakeFromPolarDegrees(-endDegrees, 0.5f) + Vec2::HALF);
        // verts.emplace_back(bottomLeftPosition, color, Vec2::MakeFromPolarDegrees(-startDegrees, 0.5f) + Vec2::HALF);

        // 7. Add verts for cylinder's side with correct UVs.
        float const uStart = Interpolate(UVs.m_mins.x, UVs.m_maxs.x, static_cast<float>(sideIndex) / static_cast<float>(numSlices));
        float const uEnd   = Interpolate(UVs.m_mins.x, UVs.m_maxs.x, static_cast<float>(sideIndex + 1) / static_cast<float>(numSlices));

        AddVertsForWireframeQuad3D(verts, bottomLeftPosition, bottomRightPosition, topLeftPosition, topRightPosition, thickness, color, AABB2(Vec2(uStart, 0.f), Vec2(uEnd, 1.f)));
    }
}

//----------------------------------------------------------------------------------------------------
void AddVertsForCone3D(VertexList_PCU& verts,
                       Vec3 const&     startPosition,
                       Vec3 const&     endPosition,
                       float const     radius,
                       Rgba8 const&    color,
                       AABB2 const&    UVs,
                       int const       numSlices)
{
    // 1. Calculate cone's forward/normalized direction.
    Vec3 const forwardDirection = endPosition - startPosition;
    Vec3 const iBasis           = forwardDirection.GetNormalized();

    // 2. Get jBasis and kBasis based on normalDirection.
    Vec3 jBasis, kBasis;
    iBasis.GetOrthonormalBasis(iBasis, &jBasis, &kBasis);

    // 3. Calculate the degree of each triangle in the bottom disc of the cone.
    float const DEGREES_PER_SIDE = 360.f / static_cast<float>(numSlices);

    for (int sideIndex = 0; sideIndex < numSlices; ++sideIndex)
    {
        // 4. Get the degree of each triangle on the unit circle.
        float const startDegrees = DEGREES_PER_SIDE * static_cast<float>(sideIndex);
        float const endDegrees   = DEGREES_PER_SIDE * static_cast<float>(sideIndex + 1);
        float const cosStart     = CosDegrees(startDegrees);
        float const sinStart     = SinDegrees(startDegrees);
        float const cosEnd       = CosDegrees(endDegrees);
        float const sinEnd       = SinDegrees(endDegrees);

        // 5. Get the positions by ( discCenter ) + ( radius ) * ( cos * jBasis + sin * kBasis )
        Vec3 topCenterPosition(endPosition);
        Vec3 topLeftPosition(endPosition);
        Vec3 topRightPosition(endPosition);
        Vec3 bottomCenterPosition(startPosition);
        Vec3 bottomLeftPosition(startPosition + radius * (-cosStart * -jBasis + -sinStart * -kBasis));
        Vec3 bottomRightPosition(startPosition + radius * (-cosEnd * -jBasis + -sinEnd * -kBasis));

        // 6. Stores the vertices using counter-clockwise order.
        verts.emplace_back(topCenterPosition, color);
        verts.emplace_back(topLeftPosition, color);
        verts.emplace_back(topRightPosition, color);
        verts.emplace_back(bottomCenterPosition, color);
        verts.emplace_back(bottomRightPosition, color);
        verts.emplace_back(bottomLeftPosition, color);

        // 7. Add verts for cone's side.
        AddVertsForQuad3D(verts, bottomLeftPosition, bottomRightPosition, topLeftPosition, topRightPosition, color, UVs);
    }
}

void AddVertsForWireframeCone3D(VertexList_PCU& verts,
                                Vec3 const&     startPosition,
                                Vec3 const&     endPosition,
                                float           radius,
                                float           thickness,
                                Rgba8 const&    color,
                                AABB2 const&    UVs,
                                int             numSlices)
{
    // 1. Calculate cone's forward/normalized direction.
    Vec3 const forwardDirection = endPosition - startPosition;
    Vec3 const iBasis           = forwardDirection.GetNormalized();

    // 2. Get jBasis and kBasis based on normalDirection.
    Vec3 jBasis, kBasis;
    iBasis.GetOrthonormalBasis(iBasis, &jBasis, &kBasis);

    // 3. Calculate the degree of each triangle in the bottom disc of the cone.
    float const DEGREES_PER_SIDE = 360.f / static_cast<float>(numSlices);

    for (int sideIndex = 0; sideIndex < numSlices; ++sideIndex)
    {
        // 4. Get the degree of each triangle on the unit circle.
        float const startDegrees = DEGREES_PER_SIDE * static_cast<float>(sideIndex);
        float const endDegrees   = DEGREES_PER_SIDE * static_cast<float>(sideIndex + 1);
        float const cosStart     = CosDegrees(startDegrees);
        float const sinStart     = SinDegrees(startDegrees);
        float const cosEnd       = CosDegrees(endDegrees);
        float const sinEnd       = SinDegrees(endDegrees);

        // 5. Get the positions by ( discCenter ) + ( radius ) * ( cos * jBasis + sin * kBasis )
        Vec3 topCenterPosition(endPosition);
        Vec3 topLeftPosition(endPosition);
        Vec3 topRightPosition(endPosition);
        Vec3 bottomCenterPosition(startPosition);
        Vec3 bottomLeftPosition(startPosition + radius * (-cosStart * -jBasis + -sinStart * -kBasis));
        Vec3 bottomRightPosition(startPosition + radius * (-cosEnd * -jBasis + -sinEnd * -kBasis));

        // 6. Stores the vertices using counter-clockwise order.
        // verts.emplace_back(topCenterPosition, color);
        // verts.emplace_back(topLeftPosition, color);
        // verts.emplace_back(topRightPosition, color);
        // verts.emplace_back(bottomCenterPosition, color);
        // verts.emplace_back(bottomRightPosition, color);
        // verts.emplace_back(bottomLeftPosition, color);

        // 7. Add verts for cone's side.
        AddVertsForWireframeQuad3D(verts, bottomLeftPosition, bottomRightPosition, topLeftPosition, topRightPosition, thickness, color, UVs);
    }
}

//----------------------------------------------------------------------------------------------------
void AddVertsForArrow3D(VertexList_PCU& verts,
                        Vec3 const&     startPosition,
                        Vec3 const&     endPosition,
                        float const     coneCylinderHeightRatio,
                        float const     cylinderRadius,
                        float const     coneRadius,
                        Rgba8 const&    color,
                        AABB2 const&    UVs,
                        int const       numCylinderSlices,
                        int const       numConeSlices)
{
    Vec3 const forwardDirection = endPosition - startPosition;
    Vec3 const midPosition      = startPosition + forwardDirection * coneCylinderHeightRatio;

    AddVertsForCylinder3D(verts, startPosition, midPosition, cylinderRadius, color, UVs, numCylinderSlices);
    AddVertsForCone3D(verts, midPosition, endPosition, coneRadius, color, UVs, numConeSlices);
}
