//-----------------------------------------------------------------------------------------------
// VertexUtils.hpp
//

//-----------------------------------------------------------------------------------------------
#pragma once
#include <vector>

#include "Engine/Math/Vec2.hpp"

//-----------------------------------------------------------------------------------------------
struct Rgba8;
struct Triangle2;
struct Capsule2;
struct LineSegment2;
struct Disc2;
struct Vec2;
struct Vertex_PCU;
struct AABB2;
struct OBB2;

//----------------------------------------------------------------------------------------------------
typedef std::vector<Vertex_PCU> VertexList;

//----------------------------------------------------------------------------------------------------
void TransformVertexArrayXY3D(int numVerts, Vertex_PCU* verts, float uniformScaleXY, float rotationDegreesAboutZ, Vec2 const& translationXY);
void AddVertsForDisc2D(VertexList& verts, Vec2 const& discCenter, float discRadius, Rgba8 const& color);
void AddVertsForDisc2D(VertexList& verts, Disc2 const& disc, Rgba8 const& color);
void AddVertsForLineSegment2D(VertexList& verts, Vec2 const& startPos, Vec2 const& endPos, Rgba8 const& color);
void AddVertsForLineSegment2D(VertexList& verts, LineSegment2 const& lineSegment, Rgba8 const& color);
void AddVertsForTriangle2D(VertexList& verts, Vec2 const& ccw0, Vec2 const& ccw1, Vec2 const& ccw2, Rgba8 const& color);
void AddVertsForTriangle2D(VertexList& verts, Triangle2 const& triangle, Rgba8 const& color);
void AddVertsForAABB2D(VertexList& verts, AABB2 const& aabb2Box, Rgba8 const& color, Vec2 const& uvMins = Vec2::ZERO, Vec2 const& uvMaxs = Vec2::ONE);
void AddVertsForOBB2D(VertexList& verts, OBB2 const& obb2Box, Rgba8 const& color);
void AddVertsForCapsule2D(VertexList& verts, Vec2 const& boneStart, Vec2 const& boneEnd, float radius, Rgba8 const& color);
void AddVertsForCapsule2D(VertexList& verts, Capsule2 const& capsule, Rgba8 const& color);
void AddVertsForHalfDisc2D(VertexList& verts, Vec2 const& discCenter, float discRadius, Rgba8 const& color, bool isTopHalf, float rotationDegrees);
void AddVertsForArrow2D(VertexList& verts, Vec2 const& tailPos, Vec2 const& tipPos, float arrowSize, float thickness, Rgba8 const& color);
