#include "Engine/Math/ConvexHull2.hpp"
#include "Engine/Math/MathUtils.hpp"
#include <algorithm>

ConvexPoly2::ConvexPoly2( std::vector<Vec2> const& vertexPosCCW )
	:m_vertexPos(vertexPosCCW)
{

}

ConvexPoly2::ConvexPoly2( ConvexHull2 const& convexHull )
{
	int numOfPlanes = (int)convexHull.m_boundingPlanes.size();
	if (numOfPlanes == 1 || numOfPlanes == 0) { // no intersections
		return;
	}
	std::vector<Vec2> validPoints;
	for (int i = 0; i < numOfPlanes; ++i) {
		// get intersection of all previous planes
		for (int j = i - 1; j >= 0; --j) {
			Vec2 point = GetPlaneIntersection2D( convexHull.m_boundingPlanes[i], convexHull.m_boundingPlanes[j] );
			// check if it fits the requirement of all planes
			bool fit = true;
			for (int k = 0; k < numOfPlanes; ++k) {
				if (convexHull.m_boundingPlanes[k].GetAltitudeOfPoint( point ) > 0.001f) {
					fit = false;
					break;
				}
			}
			if (fit) {
				validPoints.push_back( point );
			}
		}
	}

	if ((int)validPoints.size() == 0) {
		return;
	}

	// construct a convex polygon from these points
	// choose a point with lowest y
	int lowestIndex = 0;
	for (int i = 0; i < (int)validPoints.size(); i++) {
		if (validPoints[i].y < validPoints[lowestIndex].y) {
			lowestIndex = i;
		}
	}
	Vec2 lowestPoint = validPoints[lowestIndex];

	// sort the vector in increasing order of the angle points and the lowest y point make with the x-axis.
	std::sort( validPoints.begin(), validPoints.end(), [&](Vec2 const& a, Vec2 const& b) {
		if (a == lowestPoint) {
			return true;
		}
		if (b == lowestPoint) {
			return false;
		}
		Vec2 aNormal = (a - lowestPoint).GetNormalized();
		Vec2 bNormal = (b - lowestPoint).GetNormalized();
		return aNormal.x > bNormal.x;
		} );

	m_vertexPos = validPoints;
}

int ConvexPoly2::GetVertexCount() const
{
	return (int)m_vertexPos.size();
}

std::vector<Vec2> const& ConvexPoly2::GetVertexArray() const
{
	return m_vertexPos;
}

bool ConvexPoly2::AddVertexToEnd( Vec2 const& pos )
{
	m_vertexPos.push_back( pos );
	if (!IsValid()) {
		m_vertexPos.pop_back();
		return false;
	}
	return true;
}

void ConvexPoly2::ClearVertices()
{
	m_vertexPos.clear();
}

bool ConvexPoly2::IsValid() const
{
	int vertSize = (int)m_vertexPos.size();
	if (vertSize == 0) {
		return false;
	}
	else if (vertSize == 1 || vertSize == 2) {
		return true;
	}
	for (int i = 0; i < vertSize - 1; ++i) {
		if (i == vertSize - 2) {
			if (CrossProduct2D( m_vertexPos[i + 1] - m_vertexPos[i], m_vertexPos[0] - m_vertexPos[i + 1] ) < 0.f) {
				return false;
			}
		}
		else {
			if (CrossProduct2D( m_vertexPos[i + 1] - m_vertexPos[i], m_vertexPos[i + 2] - m_vertexPos[i + 1] ) < 0.f) {
				return false;
			}
		}
	}
	return true;
}

void ConvexPoly2::Translate( Vec2 const& offset )
{
	for (int i = 0; i < (int)m_vertexPos.size(); ++i) {
		m_vertexPos[i] += offset;
	}
}

void ConvexPoly2::Rotate( float degrees, Vec2 const& refPoint /*= Vec2( 0.f, 0.f ) */ )
{
	Translate( -refPoint );
	for (int i = 0; i < (int)m_vertexPos.size(); ++i) {
		m_vertexPos[i].RotateDegrees( degrees );
	}
	Translate( refPoint );
}

void ConvexPoly2::Scale( float scaleFactor, Vec2 const& refPoint /*= Vec2( 0.f, 0.f ) */ )
{
	Translate( -refPoint );
	for (int i = 0; i < (int)m_vertexPos.size(); ++i) {
		m_vertexPos[i] *= scaleFactor;
	}
	Translate( refPoint );
}

ConvexHull2::ConvexHull2()
{

}

ConvexHull2::ConvexHull2( std::vector<Plane2> const& boundingPlanes )
	:m_boundingPlanes(boundingPlanes)
{

}

ConvexHull2::ConvexHull2( ConvexPoly2 const& convexPoly )
{
	if (!convexPoly.IsValid()) {
		return;
	}
	std::vector<Vec2> const& verts = convexPoly.GetVertexArray();
	if ((int)verts.size() == 1) {
		return;
	}
	for (int i = 0; i < (int)verts.size(); ++i) {
		if (i == (int)verts.size() - 1) {
			Vec2 normal = (verts[0] - verts[i]).GetNormalized().GetRotatedMinus90Degrees();
			m_boundingPlanes.emplace_back( normal, verts[i] );
		}
		else {
			Vec2 normal = (verts[i + 1] - verts[i]).GetNormalized().GetRotatedMinus90Degrees();
			m_boundingPlanes.emplace_back( normal, verts[i] );
		}
	}
}

void ConvexHull2::Translate( Vec2 const& offset )
{
	for (int i = 0; i < (int)m_boundingPlanes.size(); ++i) {
		Vec2 origin = m_boundingPlanes[i].GetOriginPoint();
		origin += offset;
		float offsetDist = m_boundingPlanes[i].GetAltitudeOfPoint( origin );
		m_boundingPlanes[i].m_distanceFromOrigin += offsetDist;
	}
}

void ConvexHull2::Rotate( float degrees, Vec2 const& refPoint )
{
	Translate( -refPoint );
	for (int i = 0; i < (int)m_boundingPlanes.size(); ++i) {
		m_boundingPlanes[i].m_normal.RotateDegrees( degrees );
	}
	Translate( refPoint );
}

void ConvexHull2::Scale( float scaleFactor, Vec2 const& refPoint /*= Vec2( 0.f, 0.f ) */ )
{
	Translate( -refPoint );
	for (int i = 0; i < (int)m_boundingPlanes.size(); ++i) {
		m_boundingPlanes[i].m_distanceFromOrigin *= scaleFactor;
	}
	Translate( refPoint );
}
