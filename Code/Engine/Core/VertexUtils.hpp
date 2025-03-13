//----------------------------------------------------------------------------------------------------
// VertexUtils.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include <vector>

#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/Mat44.hpp"
#include "Engine/Math/Vec2.hpp"

//----------------------------------------------------------------------------------------------------
struct AABB3;
struct Capsule2;
struct Disc2;
struct LineSegment2;
struct OBB2;
struct Triangle2;
struct Vec3;
struct Vertex_PCU;
struct Vertex_PCUTBN;

//----------------------------------------------------------------------------------------------------
typedef std::vector<Vertex_PCU>    VertexList;
typedef std::vector<Vertex_PCUTBN> VertexList_PCUTBN;

//----------------------------------------------------------------------------------------------------
void TransformVertexArrayXY3D(int numVerts, Vertex_PCU* verts, float uniformScaleXY, float rotationDegreesAboutZ, const Vec2& translationXY);
void AddVertsForDisc2D(VertexList& verts, Vec2 const& discCenter, float discRadius, Rgba8 const& color);
void AddVertsForDisc3D(VertexList& verts, Vec3 const& discCenter, float discRadius, Vec3 const& normalDirection, Rgba8 const& color);
void AddVertsForDisc2D(VertexList& verts, Disc2 const& disc, Rgba8 const& color);
void AddVertsForLineSegment2D(VertexList& verts, Vec2 const& startPosition, Vec2 const& endPosition, float thickness, bool isInfinite, Rgba8 const& color);
void AddVertsForLineSegment2D(VertexList& verts, LineSegment2 const& lineSegment, float thickness, bool isInfinite, Rgba8 const& color);
void AddVertsForTriangle2D(VertexList& verts, Vec2 const& ccw0, Vec2 const& ccw1, Vec2 const& ccw2, Rgba8 const& color);
void AddVertsForTriangle2D(VertexList& verts, Triangle2 const& triangle, Rgba8 const& color);
void AddVertsForAABB2D(VertexList& verts, AABB2 const& aabb2Box, Rgba8 const& color, Vec2 const& uvMins = Vec2::ZERO, Vec2 const& uvMaxs = Vec2::ONE);
void AddVertsForOBB2D(VertexList& verts, OBB2 const& obb2Box, Rgba8 const& color);
void AddVertsForCapsule2D(VertexList& verts, Vec2 const& boneStart, Vec2 const& boneEnd, float radius, Rgba8 const& color);
void AddVertsForCapsule2D(VertexList& verts, Capsule2 const& capsule, Rgba8 const& color);
void AddVertsForHalfDisc2D(VertexList& verts, Vec2 const& discCenter, float discRadius, Rgba8 const& color, bool isTopHalf, float rotationDegrees);
void AddVertsForArrow2D(VertexList& verts, Vec2 const& tailPosition, Vec2 const& tipPosition, float arrowSize, float thickness, Rgba8 const& color);
void AddVertsForQuad3D(VertexList& verts, Vec3 const& bottomLeft, Vec3 const& bottomRight, Vec3 const& topLeft, Vec3 const& topRight, Rgba8 const& color = Rgba8::WHITE, AABB2 const& uv = AABB2::ZERO_TO_ONE);
void AddVertsForAABB3D(VertexList& verts, AABB3 const& bounds, Rgba8 const& color = Rgba8::WHITE, AABB2 const& UVs = AABB2::ZERO_TO_ONE);
void AddVertsForSphere3D(VertexList& verts, Vec3 const& center, float radius, Rgba8 const& color = Rgba8::WHITE, AABB2 const& UVs = AABB2::ZERO_TO_ONE, int numSlices = 32, int numStacks = 16);
void AddVertsForWireframeSphere3D(VertexList& verts, Vec3 const& center, float radius, Rgba8 const& color = Rgba8::WHITE, AABB2 const& UVs = AABB2::ZERO_TO_ONE, int numSlices = 32, int numStacks = 16);

void  TransformVertexArray3D(VertexList& verts, Mat44 const& transform);
AABB2 GetVertexBounds2D(VertexList const& verts);
void  AddVertsForCylinder3D(VertexList& verts, Vec3 const& startPosition, Vec3 const& endPosition, float radius, Rgba8 const& color = Rgba8::WHITE, AABB2 const& UVs = AABB2::ZERO_TO_ONE, int numSlices = 32);
void  AddVertsForCone3D(VertexList& verts, Vec3 const& startPosition, Vec3 const& endPosition, float radius, Rgba8 const& color = Rgba8::WHITE, AABB2 const& UVs = AABB2::ZERO_TO_ONE, int numSlices = 32);
void  AddVertsForArrow3D(VertexList& verts, Vec3 const& startPosition, Vec3 const& endPosition, float coneCylinderHeightRatio, float cylinderRadius, float coneRadius, Rgba8 const& color = Rgba8::WHITE, AABB2 const& UVs = AABB2::ZERO_TO_ONE, int numCylinderSlices = 32, int numConeSlices = 32);
