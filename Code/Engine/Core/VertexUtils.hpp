//----------------------------------------------------------------------------------------------------
// VertexUtils.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include <vector>

#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Core/Vertex_PCUTBN.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/Mat44.hpp"
#include "Engine/Math/Vec2.hpp"

//----------------------------------------------------------------------------------------------------
struct AABB3;
struct Capsule2;
struct Disc2;
struct LineSegment2;
struct OBB2;
struct OBB3;
struct Triangle2;
struct Vec3;

//----------------------------------------------------------------------------------------------------
typedef std::vector<Vertex_PCU>    VertexList_PCU;
typedef std::vector<Vertex_PCUTBN> VertexList_PCUTBN;
typedef std::vector<unsigned int>  IndexList;

//----------------------------------------------------------------------------------------------------
AABB2 GetVertexBounds2D(VertexList_PCU const& verts);

void TransformVertexArrayXY3D(int numVerts, Vertex_PCU* verts, float uniformScaleXY, float rotationDegreesAboutZ, const Vec2& translationXY);
void TransformVertexArray3D(VertexList_PCU& verts, Mat44 const& transform);

void AddVertsForDisc2D(VertexList_PCU& verts, Vec2 const& discCenter, float discRadius, Rgba8 const& fillColor = Rgba8::WHITE);
void AddVertsForDisc2D(VertexList_PCU& verts, Vec2 const& discCenter, float discRadius, float thickness, Rgba8 const& outlineColor = Rgba8::WHITE);
void AddVertsForDisc3D(VertexList_PCU& verts, Vec3 const& discCenter, float discRadius, Vec3 const& normalDirection, Rgba8 const& color = Rgba8::WHITE);
void AddVertsForDisc2D(VertexList_PCU& verts, Disc2 const& disc, Rgba8 const& color = Rgba8::WHITE);
void AddVertsForLineSegment2D(VertexList_PCU& verts, Vec2 const& startPosition, Vec2 const& endPosition, float thickness, bool isInfinite, Rgba8 const& color = Rgba8::WHITE);
void AddVertsForLineSegment2D(VertexList_PCU& verts, LineSegment2 const& lineSegment, float thickness, bool isInfinite, Rgba8 const& color = Rgba8::WHITE);
void AddVertsForTriangle2D(VertexList_PCU& verts, Vec2 const& ccw0, Vec2 const& ccw1, Vec2 const& ccw2, Rgba8 const& color = Rgba8::WHITE);
void AddVertsForTriangle2D(VertexList_PCU& verts, Triangle2 const& triangle, Rgba8 const& color = Rgba8::WHITE);
void AddVertsForAABB2D(VertexList_PCU& verts, AABB2 const& aabb2Box, Rgba8 const& color = Rgba8::WHITE, Vec2 const& uvMins = Vec2::ZERO, Vec2 const& uvMaxs = Vec2::ONE);
void AddVertsForAABB2D(VertexList_PCU& verts, Vec2 const& aabbMins, Vec2 const& aabbMaxs, Rgba8 const& color = Rgba8::WHITE, Vec2 const& uvMins = Vec2::ZERO, Vec2 const& uvMaxs = Vec2::ONE);
void AddVertsForOBB2D(VertexList_PCU& verts, Vec2 const& obb2Center, Vec2 const& obb2IBasisNormal, Vec2 const& obb2HalfDimensions, Rgba8 const& color = Rgba8::WHITE);
void AddVertsForOBB3D(VertexList_PCU& verts, OBB3 const& obb3, Rgba8 const& color = Rgba8::WHITE, AABB2 const& UVs = AABB2::ZERO_TO_ONE);
void AddVertsForWireframeOBB3D(VertexList_PCU& verts, OBB3 const& obb3, Rgba8 const& color);
void AddVertsForCapsule2D(VertexList_PCU& verts, Vec2 const& capsuleStartPosition, Vec2 const& capsuleEndPosition, float capsuleRadius, Rgba8 const& color = Rgba8::WHITE);
void AddVertsForCapsule2D(VertexList_PCU& verts, Capsule2 const& capsule, Rgba8 const& color = Rgba8::WHITE);
void AddVertsForHalfDisc2D(VertexList_PCU& verts, Vec2 const& discCenter, float discRadius, bool isTopHalf, float rotationDegrees, Rgba8 const& color = Rgba8::WHITE);
void AddVertsForArrow2D(VertexList_PCU& verts, Vec2 const& tailPosition, Vec2 const& tipPosition, float arrowSize, float thickness, Rgba8 const& color = Rgba8::WHITE);
void AddVertsForQuad3D(VertexList_PCU& verts, Vec3 const& bottomLeft, Vec3 const& bottomRight, Vec3 const& topLeft, Vec3 const& topRight, Rgba8 const& color = Rgba8::WHITE, AABB2 const& UVs = AABB2::ZERO_TO_ONE);
void AddVertsForQuad3D(VertexList_PCUTBN& verts, Vec3 const& bottomLeft, Vec3 const& bottomRight, Vec3 const& topLeft, Vec3 const& topRight, Rgba8 const& color = Rgba8::WHITE, AABB2 const& UVs = AABB2::ZERO_TO_ONE);
void AddVertsForQuad3D(VertexList_PCUTBN& verts, IndexList& indexes, Vec3 const& bottomLeft, Vec3 const& bottomRight, Vec3 const& topLeft, Vec3 const& topRight, Rgba8 const& color = Rgba8::WHITE, AABB2 const& UVs = AABB2::ZERO_TO_ONE);
void AddVertsForRoundedQuad3D(VertexList_PCUTBN& verts, Vec3 const& topLeft, Vec3 const& bottomLeft, Vec3 const& bottomRight, Vec3 const& topRight, Rgba8 const& color = Rgba8::WHITE, AABB2 const& UVs = AABB2::ZERO_TO_ONE);
void AddVertsForWireframeQuad3D(VertexList_PCU& verts, Vec3 const& bottomLeft, Vec3 const& bottomRight, Vec3 const& topLeft, Vec3 const& topRight, float thickness, Rgba8 const& color = Rgba8::WHITE, AABB2 const& UVs = AABB2::ZERO_TO_ONE);
void AddVertsForAABB3D(VertexList_PCU& verts, AABB3 const& bounds, Rgba8 const& color = Rgba8::WHITE, AABB2 const& UVs = AABB2::ZERO_TO_ONE);
void AddVertsForAABB3D(VertexList_PCUTBN& verts, IndexList& indexes, AABB3 const& bounds, Rgba8 const& color = Rgba8::WHITE, AABB2 const& UVs = AABB2::ZERO_TO_ONE);
void AddVertsForWireframeAABB3D(VertexList_PCU& verts, AABB3 const& bounds, float thickness, Rgba8 const& color = Rgba8::WHITE, AABB2 const& UVs = AABB2::ZERO_TO_ONE);
void AddVertsForSphere3D(VertexList_PCU& verts, Vec3 const& center, float radius, Rgba8 const& color = Rgba8::WHITE, AABB2 const& UVs = AABB2::ZERO_TO_ONE, int numSlices = 32, int numStacks = 16);
void AddVertsForSphere3D(VertexList_PCUTBN& verts, IndexList& indexes, Vec3 const& center, float radius, Rgba8 const& color = Rgba8::WHITE, AABB2 const& UVs = AABB2::ZERO_TO_ONE, int numSlices = 32, int numStacks = 16);
void AddVertsForWireframeSphere3D(VertexList_PCU& verts, Vec3 const& center, float radius, float thickness, Rgba8 const& color = Rgba8::WHITE, AABB2 const& UVs = AABB2::ZERO_TO_ONE, int numSlices = 32, int numStacks = 16);

void AddVertsForCylinder3D(VertexList_PCU& verts, Vec3 const& startPosition, Vec3 const& endPosition, float radius, Rgba8 const& color = Rgba8::WHITE, AABB2 const& UVs = AABB2::ZERO_TO_ONE, int numSlices = 32);
void AddVertsForCylinder3D(VertexList_PCUTBN& verts, IndexList& indexes, Vec3 const& startPosition, Vec3 const& endPosition, float radius, Rgba8 const& color = Rgba8::WHITE, AABB2 const& UVs = AABB2::ZERO_TO_ONE, int numSlices = 32);
void AddVertsForWireframeCylinder3D(VertexList_PCU& verts, Vec3 const& startPosition, Vec3 const& endPosition, float radius, float thickness, Rgba8 const& color = Rgba8::WHITE, AABB2 const& UVs = AABB2::ZERO_TO_ONE, int numSlices = 32);
void AddVertsForCone3D(VertexList_PCU& verts, Vec3 const& startPosition, Vec3 const& endPosition, float radius, Rgba8 const& color = Rgba8::WHITE, AABB2 const& UVs = AABB2::ZERO_TO_ONE, int numSlices = 32);
void AddVertsForWireframeCone3D(VertexList_PCU& verts, Vec3 const& startPosition, Vec3 const& endPosition, float radius, float thickness, Rgba8 const& color = Rgba8::WHITE, AABB2 const& UVs = AABB2::ZERO_TO_ONE, int numSlices = 32);
void AddVertsForArrow3D(VertexList_PCU& verts, Vec3 const& startPosition, Vec3 const& endPosition, float coneCylinderHeightRatio, float cylinderRadius, float coneRadius, Rgba8 const& color = Rgba8::WHITE, AABB2 const& UVs = AABB2::ZERO_TO_ONE, int numCylinderSlices = 32, int numConeSlices = 32);
