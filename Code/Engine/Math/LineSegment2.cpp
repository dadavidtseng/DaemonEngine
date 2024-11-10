//-----------------------------------------------------------------------------------------------
// LineSegment2.cpp
//-----------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------
#include "Engine/Math/LineSegment2.hpp"
#include "Engine/Math/MathUtils.hpp"

//-----------------------------------------------------------------------------------------------
LineSegment2::LineSegment2(const Vec2& start, const Vec2& end, const float thickness, const bool isInfinite)
	: m_start(start),
	  m_end(end),
	  m_thickness(thickness),
	  m_isInfinite(isInfinite)
// Parameterized constructor initializes start and end points
{
}

float LineSegment2::GetLength() const
{
	return GetDistance2D(m_start, m_end);
}

// Get the center of the line segment
Vec2 LineSegment2::GetCenter() const
{
	return (m_start + m_end) * 0.5f; // Return the midpoint
}

// Get the nearest point on the line segment to a reference position
Vec2 LineSegment2::GetNearestPoint(Vec2 const& referencePosition) const
{
	const Vec2  lineDir           = m_end - m_start; // Direction of the line segment
	const float lineLengthSquared = lineDir.GetLengthSquared();

	// If the line segment is essentially a point
	if (lineLengthSquared == 0.0f)
	{
		return m_start; // Return the start point as the nearest point
	}

	// Project the reference position onto the infinite line defined by m_start and m_end
	const float t = DotProduct2D((referencePosition - m_start), lineDir) / lineLengthSquared;

	if (m_isInfinite)
	{
		// Return the nearest point on the infinite line
		return m_start + t * lineDir;
	}
	else
	{
		// Clamp t to the range [0, 1] for the finite line segment
		const float clampedT = GetClampedZeroToOne(t);
		return m_start + clampedT * lineDir; // Return the nearest point on the line segment
	}
}

//-----------------------------------------------------------------------------------------------
void LineSegment2::Translate(const Vec2& translation)
{
	m_start += translation; // Translate the start point
	m_end += translation;   // Translate the end point
}

//-----------------------------------------------------------------------------------------------
void LineSegment2::SetCenter(const Vec2& newCenter)
{
	// Set the center of the line segment
	Vec2 currentCenter = (m_start + m_end) * 0.5f;  // Calculate the current center
	Vec2 offset        = newCenter - currentCenter; // Calculate the offset to new center
	Translate(offset);                              // Translate the line segment to the new center
}

//-----------------------------------------------------------------------------------------------
void LineSegment2::RotateAboutCenter(float rotationDeltaDegrees)
{
	// Calculate the center of the line segment
	Vec2 center = (m_start + m_end) * 0.5f;

	// Rotate the start point around the center
	Vec2  toStart  = m_start - center;
	float angleRad = rotationDeltaDegrees * (PI / 180.0f); // Convert degrees to radians
	float cosTheta = CosDegrees(angleRad);
	float sinTheta = SinDegrees(angleRad);

	m_start.x = center.x + (toStart.x * cosTheta - toStart.y * sinTheta);
	m_start.y = center.y + (toStart.x * sinTheta + toStart.y * cosTheta);

	// Rotate the end point around the center
	Vec2 toEnd = m_end - center;
	m_end.x    = center.x + (toEnd.x * cosTheta - toEnd.y * sinTheta);
	m_end.y    = center.y + (toEnd.x * sinTheta + toEnd.y * cosTheta);
}
