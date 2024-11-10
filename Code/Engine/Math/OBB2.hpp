//-----------------------------------------------------------------------------------------------
// OBB2.hpp
//-----------------------------------------------------------------------------------------------

#pragma once
#include "Engine/Math/Vec2.hpp"

//-----------------------------------------------------------------------------------------------
struct OBB2
{
	Vec2 m_center;            // Center point
	Vec2 m_iBasisNormal;      // Basis vector in the X-axis direction (unit vector)
	Vec2 m_halfDimensions;    // Half the width and height (half dimensions)

	OBB2(); // Default constructor
	explicit OBB2(const Vec2& center, const Vec2& iBasisNormal, const Vec2& halfDimensions);

	void GetCornerPoints(Vec2* out_fourCornerWorldPositions) const; // Get the positions of the four corner points (for rendering)
	Vec2 const GetLocalPosForWorldPos(const Vec2& worldPosition) const; // Convert world position to local position
	Vec2 const GetWorldPosForLocalPos(const Vec2& localPosition) const; // Convert local position to world position
	void RotateAboutCenter(float rotationDeltaDegrees); // Rotate around the center

	bool       IsPointInside(Vec2 const& point) const;   // Check if a point is inside the OBB
	Vec2 const GetCenter() const;                        // Get the center point of the OBB
	Vec2 const GetDimensions() const;                    // Get the full dimensions (width and height) of the OBB
	void       SetCenter(Vec2 const& newCenter);         // Set the center point of the OBB
	void       SetDimensions(Vec2 const& newDimensions); // Set the dimensions of the OBB
	Vec2       GetNearestPoint(const Vec2& point) const;
};
