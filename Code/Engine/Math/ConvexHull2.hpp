#pragma once
#include <vector>
#include "Engine/Math/Plane2.hpp"

struct ConvexPoly2;

struct ConvexHull2 {
public:
	ConvexHull2();
	ConvexHull2( std::vector<Plane2> const& boundingPlanes );
	ConvexHull2( ConvexPoly2 const& convexPoly );

	void Translate( Vec2 const& offset );
	void Rotate( float degrees, Vec2 const& refPoint = Vec2( 0.f, 0.f ) );
	void Scale( float scaleFactor, Vec2 const& refPoint = Vec2( 0.f, 0.f ) );

public:
	std::vector<Plane2> m_boundingPlanes;
};


struct ConvexPoly2 {
public:
	ConvexPoly2() = delete;
	ConvexPoly2( std::vector<Vec2> const& vertexPosCCW );
	ConvexPoly2( ConvexHull2 const& convexHull );

	/// Get the count of vertices
	int GetVertexCount() const;
	/// Get a const vertex array
	std::vector<Vec2> const& GetVertexArray() const;
	/// Add a vertex to the end of the vertex list, return false if the result is not a convex polygon
	bool AddVertexToEnd( Vec2 const& pos );
	/// Clear all the vertices
	void ClearVertices();
	/// Check if this convex polygon is actually a convex polygon
	bool IsValid() const;

	void Translate( Vec2 const& offset );
	void Rotate( float degrees, Vec2 const& refPoint = Vec2( 0.f, 0.f ) );
	void Scale( float scaleFactor, Vec2 const& refPoint = Vec2( 0.f, 0.f ) );

protected:
	std::vector<Vec2> m_vertexPos;

};
