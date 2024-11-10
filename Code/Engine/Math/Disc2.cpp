//----------------------------------------------------------------------------------------------------
// Disc2.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Math/Disc2.hpp"

//-----------------------------------------------------------------------------------------------
Disc2::Disc2(const Vec2& position, const float radius)
	: m_position(position), m_radius(radius) // Parameterized constructor
{
}

//-----------------------------------------------------------------------------------------------
bool Disc2::IsPointInside(Vec2 const& point) const
{
	const Vec2  displacement    = point - m_position; // Vector from center to the point
	const float distanceSquared = displacement.GetLengthSquared(); // Squared distance from center to point
	const float radiusSquared   = m_radius * m_radius; // Squared radius of the disc

	return distanceSquared <= radiusSquared; // Return true if point is inside or on the boundary
}

//-----------------------------------------------------------------------------------------------
float Disc2::GetRadius() const
{
	return m_radius; // Get the radius
}

//-----------------------------------------------------------------------------------------------
Vec2 Disc2::GetCenter() const
{
	return m_position; // Get the center
}

//-----------------------------------------------------------------------------------------------
Vec2 Disc2::GetNearestPoint(Vec2 const& referencePosition) const
{
	// Get the nearest point
	const Vec2  toCenter = referencePosition - m_position;
	const float distance = toCenter.GetLength();

	if (distance <= m_radius)
	{
		return referencePosition; // If the point is inside the circle, return the point
	}

	return m_position + toCenter.GetNormalized() * m_radius; // Return the nearest point on the circle
}

//-----------------------------------------------------------------------------------------------
void Disc2::Translate(Vec2 const& translationToApply)
{
	m_position += translationToApply; // Move the center
}

//-----------------------------------------------------------------------------------------------
void Disc2::SetCenter(Vec2 const& newCenter)
{
	m_position = newCenter; // Set the center
}

//-----------------------------------------------------------------------------------------------
void Disc2::SetRadius(const float newRadius)
{
	m_radius = newRadius; // Set the radius
}

//-----------------------------------------------------------------------------------------------
void Disc2::StretchToIncludePoint(Vec2 const& targetPointPos)
{
	// If the target point is outside the circle, stretch the circle to include that point
	const Vec2  toTarget = targetPointPos - m_position;
	const float distance = toTarget.GetLength();

	if (distance > m_radius)
	{
		m_radius = distance; // Update the radius
	}
}
