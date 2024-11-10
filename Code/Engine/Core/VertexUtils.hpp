//-----------------------------------------------------------------------------------------------
// VertexUtils.hpp
//

//-----------------------------------------------------------------------------------------------
#pragma once
#include <vector>

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

//-----------------------------------------------------------------------------------------------
void TransformVertexArrayXY3D
(
	int         numVerts, Vertex_PCU* verts,
	float       uniformScaleXY,
	float       rotationDegreesAboutZ,
	Vec2 const& translationXY
);


void AddVertsForDisc2D(VertexList& verts, Vec2 const& discCenter, float discRadius, Rgba8 const& color);
void AddVertsForDisc2D(VertexList& verts, const Disc2& disc, Rgba8 const& color);
void AddVertsForLineSegment2D(VertexList& verts, Vec2 const& start, Vec2 const& end, Rgba8 const& color);
void AddVertsForLineSegment2D(VertexList& verts, const LineSegment2& lineSegment, Rgba8 const& color);
void AddVertsForTriangle2D(VertexList& verts, Vec2 const& ccw0, Vec2 const& ccw1, Vec2 const& ccw2, Rgba8 const& color);
void AddVertsForTriangle2D(VertexList& verts, const Triangle2& triangle, Rgba8 const& color);
void AddVertsForAABB2D(VertexList& verts, const AABB2& alignedBox, Rgba8 const& color);
void AddVertsForOBB2D(VertexList& verts, const OBB2& orientedBox, Rgba8 const& color);
void AddVertsForCapsule2D(VertexList& verts, Vec2 const& boneStart, Vec2 const& boneEnd, float radius, Rgba8 const& color);
void AddVertsForCapsule2D(VertexList& verts, const Capsule2& capsule, Rgba8 const& color);
void AddVertsForHalfDisc2D(VertexList& verts, Vec2 const& discCenter, float discRadius, Rgba8 const& color, bool isTopHalf, float rotationDegrees);
void AddVertsForArrow2D(VertexList& verts, Vec2 const& tailPos, Vec2 const& tipPos, float arrowSize, float thickness, Rgba8 const& color);
