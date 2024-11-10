//----------------------------------------------------------------------------------------------------
// LineSegment2.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include "Engine/Math/Vec2.hpp"

//----------------------------------------------------------------------------------------------------
struct LineSegment2
{
	Vec2  m_start{0.f, 0.f}; // Start point of the line segment
	Vec2  m_end{0.f, 0.f};   // End point of the line segment
	float m_thickness     = 0.f;
	bool  m_isInfinite = false;

	// Constructors
	LineSegment2() = default; // Default constructor
	explicit LineSegment2(const Vec2& start, const Vec2& end, float thickness, bool isInfinite);
	// Parameterized constructor

	// Accessors (const methods)
	float GetLength() const;
	Vec2  GetCenter() const; // Get the center of the line segment
	Vec2  GetNearestPoint(Vec2 const& referencePosition) const;
// Get the nearest point on the line segment to a reference position

// Mutators (non-const methods)
	void Translate(const Vec2& translation);            // Translate the line segment
	void SetCenter(const Vec2& newCenter);              // Set the center of the line segment
	void RotateAboutCenter(float rotationDeltaDegrees); // Rotate the line segment about its center
};
