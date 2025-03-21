//----------------------------------------------------------------------------------------------------
// VertexUtils.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Core/VertexUtils.hpp"

#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Math/Capsule2.hpp"
#include "Engine/Math/Disc2.hpp"
#include "Engine/Math/LineSegment2.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/OBB2.hpp"
#include "Engine/Math/Triangle2.hpp"
#include "Engine/Renderer/Window.hpp"

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
        verts.emplace_back(centerPosition, color);
        verts.emplace_back(startOuterPosition, color);
        verts.emplace_back(endOuterPosition, color);
    }
}

void AddVertsForDisc3D(VertexList& verts, Vec3 const& discCenter, float const discRadius, Vec3 const& normalDirection, Rgba8 const& color)
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
void AddVertsForDisc2D(VertexList&  verts,
                       Disc2 const& disc,
                       Rgba8 const& color)
{
    AddVertsForDisc2D(verts,
                      disc.m_position,
                      disc.m_radius,
                      color);
}

//----------------------------------------------------------------------------------------------------
void AddVertsForLineSegment2D(VertexList&  verts,
                              Vec2 const&  startPosition,
                              Vec2 const&  endPosition,
                              float const  thickness,
                              bool const   isInfinite,
                              Rgba8 const& color)
{
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
void AddVertsForLineSegment2D(VertexList&         verts,
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
void AddVertsForTriangle2D(VertexList&  verts,
                           Vec2 const&  ccw0,
                           Vec2 const&  ccw1,
                           Vec2 const&  ccw2,
                           Rgba8 const& color)
{
    verts.emplace_back(Vec3(ccw0.x, ccw0.y, 0.f), color);
    verts.emplace_back(Vec3(ccw1.x, ccw1.y, 0.f), color);
    verts.emplace_back(Vec3(ccw2.x, ccw2.y, 0.f), color);
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

//----------------------------------------------------------------------------------------------------
void AddVertsForAABB2D(VertexList&  verts,
                       Vec2 const&  aabbMins,
                       Vec2 const&  aabbMaxs,
                       Rgba8 const& color,
                       Vec2 const&  uvMins,
                       Vec2 const&  uvMaxs)
{
    verts.emplace_back(Vec3(aabbMins.x, aabbMins.y, 0.f), color, uvMins);
    verts.emplace_back(Vec3(aabbMaxs.x, aabbMins.y, 0.f), color, Vec2(uvMaxs.x, uvMins.y));
    verts.emplace_back(Vec3(aabbMaxs.x, aabbMaxs.y, 0.f), color, uvMaxs);

    verts.emplace_back(Vec3(aabbMins.x, aabbMins.y, 0.f), color, uvMins);
    verts.emplace_back(Vec3(aabbMaxs.x, aabbMaxs.y, 0.f), color, uvMaxs);
    verts.emplace_back(Vec3(aabbMins.x, aabbMaxs.y, 0.f), color, Vec2(uvMins.x, uvMaxs.y));
}

//-----------------------------------------------------------------------------------------------
void AddVertsForOBB2D(VertexList&  verts,
                      Vec2 const&  obb2Center,
                      Vec2 const&  obb2IBasisNormal,
                      Vec2 const&  obb2HalfDimensions,
                      Rgba8 const& color)
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
void AddVertsForCapsule2D(VertexList&  verts,
                          Vec2 const&  capsuleStartPosition,
                          Vec2 const&  capsuleEndPosition,
                          float const  capsuleRadius,
                          Rgba8 const& color)
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

    AddVertsForHalfDisc2D(verts, halfDiscCenterStart, capsuleRadius, color, false, halfDiscRotationDegrees);
    AddVertsForHalfDisc2D(verts, halfDiscCenterEnd, capsuleRadius, color, true, halfDiscRotationDegrees);
}

//----------------------------------------------------------------------------------------------------
void AddVertsForCapsule2D(VertexList&     verts,
                          Capsule2 const& capsule,
                          Rgba8 const&    color)
{
    AddVertsForCapsule2D(verts, capsule.m_startPosition, capsule.m_endPosition, capsule.m_radius, color);
}

//----------------------------------------------------------------------------------------------------
void AddVertsForHalfDisc2D(VertexList&  verts,
                           Vec2 const&  discCenter,
                           float const  discRadius,
                           Rgba8 const& color,
                           bool const   isTopHalf,
                           float const  rotationDegrees)
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
void AddVertsForArrow2D(VertexList&  verts,
                        Vec2 const&  tailPosition,
                        Vec2 const&  tipPosition,
                        float const  arrowSize,
                        float const  thickness,
                        Rgba8 const& color)
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
void AddVertsForQuad3D(VertexList&  verts,
                       Vec3 const&  bottomLeft,
                       Vec3 const&  bottomRight,
                       Vec3 const&  topLeft,
                       Vec3 const&  topRight,
                       Rgba8 const& color,
                       AABB2 const& uv)
{
    // Starting at BL, add triangle A with vertexes BL, BR, TR.
    verts.emplace_back(bottomLeft, color, Vec2(uv.m_mins.x, uv.m_mins.y));
    verts.emplace_back(bottomRight, color, Vec2(uv.m_maxs.x, uv.m_mins.y));
    verts.emplace_back(topRight, color, Vec2(uv.m_maxs.x, uv.m_maxs.y));

    // Starting again at BL, add triangle B with vertexes BL, TR, TL.
    verts.emplace_back(bottomLeft, color, Vec2(uv.m_mins.x, uv.m_mins.y));
    verts.emplace_back(topRight, color, Vec2(uv.m_maxs.x, uv.m_maxs.y));
    verts.emplace_back(topLeft, color, Vec2(uv.m_mins.x, uv.m_maxs.y));
}

//----------------------------------------------------------------------------------------------------
void AddVertsForAABB3D(VertexList&  verts,
                       AABB3 const& bounds,
                       Rgba8 const& color,
                       AABB2 const& UVs)
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
void AddVertsForSphere3D(VertexList&  verts,
                         Vec3 const&  center,
                         float const  radius,
                         Rgba8 const& color,
                         AABB2 const& UVs,
                         int const    numSlices,
                         int const    numStacks)
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

            AddVertsForQuad3D(verts, center + bottomLeft, center + bottomRight, center + topLeft, center + topRight, color, quadUV);
        }
    }
}

void AddVertsForWireframeSphere3D(VertexList&  verts,
                                  Vec3 const&  center,
                                  float const  radius,
                                  Rgba8 const& color,
                                  AABB2 const& UVs,
                                  int const    numSlices,
                                  int const    numStacks)
{
}

//----------------------------------------------------------------------------------------------------
void TransformVertexArray3D(VertexList&  verts,
                            Mat44 const& transform)
{
    for (Vertex_PCU& vert : verts)
    {
        vert.m_position = transform.TransformPosition3D(vert.m_position);
    }
}

//----------------------------------------------------------------------------------------------------
AABB2 GetVertexBounds2D(VertexList const& verts)
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
void AddVertsForCylinder3D(VertexList&  verts,
                           Vec3 const&  startPosition,
                           Vec3 const&  endPosition,
                           float const  radius,
                           Rgba8 const& color,
                           AABB2 const& UVs,
                           int const    numSlices)
{
    // float degreesPerSlice = 360.f / (float)numSlices;
    // Vec3 center = (startPosition + endPosition) * 0.5f;
    // float minZ = startPosition.z;
    // float maxZ = endPosition.z;
    // Vec3 start = Vec3( center.x, center.y, minZ );
    // Vec3 end = Vec3( center.x, center.y, maxZ );
    // float curDegrees = 0.f;
    // Vec3 sideVector = Vec3( CosDegrees( curDegrees ), SinDegrees( curDegrees ), 0.f );
    // Vec3 nextSideVector;
    // for (int i = 0; i < numSlices; i++) {
    //     curDegrees += degreesPerSlice;
    //     nextSideVector = Vec3( CosDegrees( curDegrees ), SinDegrees( curDegrees ), 0.f );
    //     Vec3 startSidePoint = start + sideVector * radius;
    //     Vec3 startNextSidePoint = start + nextSideVector * radius;
    //     Vec3 endSidePoint = end + sideVector * radius;
    //     Vec3 endNextSidePoint = end + nextSideVector * radius;
    //     // start triangle
    //     verts.emplace_back( start, color, Vec2( 0.5f, 0.5f ) );
    //     verts.emplace_back( startNextSidePoint, color, Vec2( nextSideVector.x * 0.5f + 0.5f, nextSideVector.y * 0.5f + 0.5f ) );
    //     verts.emplace_back( startSidePoint, color, Vec2( sideVector.x * 0.5f + 0.5f, sideVector.y * 0.5f + 0.5f ) );
    //     // cylinder side quad
    //     AddVertsForQuad3D( verts, startSidePoint, startNextSidePoint, endSidePoint, endNextSidePoint, color, AABB2( Vec2( Interpolate( UVs.m_mins.x, UVs.m_maxs.x, (float)i / (float)numSlices ), 0.f ), Vec2( Interpolate( UVs.m_mins.x, UVs.m_maxs.x, (float)(i + 1) / (float)numSlices ), 1.f ) ) );
    //     // end triangle
    //     verts.emplace_back( end, color, Vec2( 0.5f, 0.5f ) );
    //     verts.emplace_back( endSidePoint, color, Vec2( sideVector.x * 0.5f + 0.5f, sideVector.y * 0.5f + 0.5f ) );
    //     verts.emplace_back( endNextSidePoint, color, Vec2( nextSideVector.x * 0.5f + 0.5f, nextSideVector.y * 0.5f + 0.5f ) );
    //     sideVector = nextSideVector;
    // }

    // 1. Calculate cylinder's forward/normalized direction.
    Vec3 const forwardDirection = endPosition - startPosition;
    Vec3 const iBasis           = forwardDirection.GetNormalized();

    // 2. Get jBasis and kBasis based on normalDirection.
    Vec3 const jBasis = Vec3::X_BASIS;
    Vec3 const kBasis = Vec3::Y_BASIS;
    // iBasis.GetOrthonormalBasis(iBasis, &jBasis, &kBasis);

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

        topLeftPosition.z     = endPosition.z;
        topRightPosition.z    = endPosition.z;
        bottomLeftPosition.z  = startPosition.z;
        bottomRightPosition.z = startPosition.z;

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

//----------------------------------------------------------------------------------------------------
void AddVertsForCone3D(VertexList&  verts,
                       Vec3 const&  startPosition,
                       Vec3 const&  endPosition,
                       float const  radius,
                       Rgba8 const& color,
                       AABB2 const& UVs,
                       int const    numSlices)
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

//----------------------------------------------------------------------------------------------------
void AddVertsForArrow3D(VertexList& verts, Vec3 const& startPosition, Vec3 const& endPosition, float const coneCylinderHeightRatio, float const cylinderRadius, float const coneRadius, Rgba8 const& color, AABB2 const& UVs, int numCylinderSlices, int numConeSlices)
{
    Vec3 const forwardDirection = endPosition - startPosition;
    Vec3 const midPosition      = startPosition + forwardDirection * coneCylinderHeightRatio;

    AddVertsForCylinder3D(verts, startPosition, midPosition, cylinderRadius, color, UVs, numCylinderSlices);
    AddVertsForCone3D(verts, midPosition, endPosition, coneRadius, color, UVs, numConeSlices);
}
