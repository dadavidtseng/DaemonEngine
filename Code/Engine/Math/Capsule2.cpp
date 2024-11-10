// Capsule2.cpp
#include "Engine/Math/Capsule2.hpp"
#include <cmath>

#include "MathUtils.hpp"

//-----------------------------------------------------------------------------------------------
Capsule2::Capsule2() = default;

//-----------------------------------------------------------------------------------------------
Capsule2::Capsule2(const Vec2& start, const Vec2& end, const float radius)
	: m_start(start), m_end(end), m_radius(radius)
{
}

//-----------------------------------------------------------------------------------------------
void Capsule2::Translate(const Vec2& translation)
{
	m_start += translation;
	m_end += translation;
}

//-----------------------------------------------------------------------------------------------
void Capsule2::SetCenter(const Vec2& newCenter)
{
	Vec2 currentCenter = GetCenter();
	Vec2 translation = newCenter - currentCenter;
	Translate(translation);
}

//-----------------------------------------------------------------------------------------------
void Capsule2::RotateAboutCenter(const Vec2& rotationDeltaDegrees)
{
	Vec2 center = GetCenter();
	float angleInRadians = rotationDeltaDegrees.x * (PI / 180.f);

	float cosTheta = cos(angleInRadians);
	float sinTheta = sin(angleInRadians);

	Vec2 startRelToCenter = m_start - center;
	Vec2 endRelToCenter = m_end - center;

	m_start.x = center.x + (startRelToCenter.x * cosTheta - startRelToCenter.y * sinTheta);
	m_start.y = center.y + (startRelToCenter.x * sinTheta + startRelToCenter.y * cosTheta);

	m_end.x = center.x + (endRelToCenter.x * cosTheta - endRelToCenter.y * sinTheta);
	m_end.y = center.y + (endRelToCenter.x * sinTheta + endRelToCenter.y * cosTheta);
}

//-----------------------------------------------------------------------------------------------
Vec2 const Capsule2::GetCenter() const
{
	return (m_start + m_end) * 0.5f; // ????????
}

//-----------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------
// Check if a point is inside the capsule, including its ends and body
bool Capsule2::IsPointInside(Vec2 const& point) const
{
	// Calculate the capsule's direction vector
	Vec2 direction = m_end - m_start;
	float capsuleLength = direction.GetLength();

	// Normalize the direction vector if capsule has length
	if (capsuleLength > 0.f)
	{
		direction.Normalize();
	}

	// Calculate the projection of the point onto the capsule's direction
	float projectionLength = DotProduct2D(point - m_start, direction);
	float clampedProjectionLength = GetClamped(projectionLength, 0.f, capsuleLength);

	// Find the nearest point on the capsule segment
	Vec2 nearestPointOnSegment = m_start + direction * clampedProjectionLength;

	// Calculate the distance from the point to the nearest point on the segment
	float distanceToSegment = (point - nearestPointOnSegment).GetLength();

	// Check if the point is within the radius of the capsule
	return distanceToSegment <= m_radius ||
		(point - m_start).GetLengthSquared() <= (m_radius * m_radius) ||
		(point - m_end).GetLengthSquared() <= (m_radius * m_radius);
}

//-----------------------------------------------------------------------------------------------
// Get the nearest point on the capsule to the given point
Vec2 Capsule2::GetNearestPoint(const Vec2& point) const
{
	// Check if the point is inside the capsule
	if (IsPointInside(point))
	{
		return point; // Return the point itself if it's inside the capsule
	}

	// Calculate the capsule's direction vector and its length
	Vec2 capsuleDir = m_end - m_start;
	float capsuleLength = capsuleDir.GetLength();

	// If capsule has length, normalize the direction; otherwise, return start or end as nearest point
	if (capsuleLength > 0.f)
	{
		capsuleDir.Normalize();
	}
	else
	{
		// Capsule is degenerate (start == end), treat as a point
		return m_start;
	}

	// Project the point onto the capsule's line segment and clamp it to stay within the segment
	Vec2 startToPoint = point - m_start;
	float projectionLength = DotProduct2D(startToPoint, capsuleDir);
	projectionLength = GetClamped(projectionLength, 0.f, capsuleLength);

	// Calculate the nearest point on the capsule's segment
	Vec2 nearestPointOnSegment = m_start + capsuleDir * projectionLength;

	// Calculate the direction from the nearest point to the given point
	Vec2 nearestDirToPoint = point - nearestPointOnSegment;

	// Normalize the direction to get the direction towards the nearest point on the boundary
	if (nearestDirToPoint.GetLength() > 0.f)
	{
		nearestDirToPoint.Normalize();
		return nearestPointOnSegment + nearestDirToPoint * m_radius; // Return the nearest point on the capsule boundary
	}

	return nearestPointOnSegment; // If the point is very close to the segment, return the nearest segment point
}