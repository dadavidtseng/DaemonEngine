//-----------------------------------------------------------------------------------------------
// Triangle2.hpp
//

//-----------------------------------------------------------------------------------------------
#pragma once
#include "Engine/Math/Vec2.hpp"

//-----------------------------------------------------------------------------------------------
struct Triangle2
{
	Vec2 m_positionCounterClockwise[3];

	// Construction/Destruction
	~Triangle2();                                               // Destructor (do nothing)
	Triangle2();                                                // Default constructor (do nothing)
	Triangle2(Triangle2 const& copyFrom);                      // Copy constructor (from another Triangle2)
	explicit Triangle2(const Vec2& p1, const Vec2& p2, const Vec2& p3); // Constructor (from three points)
	explicit Triangle2(const Vec2 points[3]);                 // Constructor (from an array of points)

	// Accessors (const methods)
	bool IsPointInside(Vec2 const& point) const;              // Check if a point is inside the triangle
	Vec2 GetCenter() const;                                   // Get the center of the triangle
	Vec2 GetNearestPoint(Vec2 const& referencePosition) const; // Get the nearest point on the triangle to a reference position

	// Mutators (non-const methods)
	void Translate(Vec2 const& translation);                 // Translate the triangle by a given vector
	void SetCenter(Vec2 const& newCenter);
	void StretchToIncludePoint(Vec2 const& targetPointPos);  // Stretch the triangle to include a given point
	void RotateAboutCenter(float degrees);
};