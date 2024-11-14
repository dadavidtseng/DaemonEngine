//----------------------------------------------------------------------------------------------------
// OBB2.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Math/OBB2.hpp"

#include <cmath>

#include "Engine/Math/MathUtils.hpp"

//----------------------------------------------------------------------------------------------------
OBB2::OBB2(const Vec2& center, const Vec2& iBasisNormal, const Vec2& halfDimensions)
    : m_center(center),
      m_iBasisNormal(iBasisNormal.GetNormalized()), // Ensure the basis vector is a unit vector
      m_halfDimensions(halfDimensions)
{
}

//----------------------------------------------------------------------------------------------------
// Get the positions of the four corner points
void OBB2::GetCornerPoints(Vec2* out_fourCornerWorldPositions) const
{
    // Calculate the Y-axis basis vector
    Vec2 jBasisNormal = Vec2(-m_iBasisNormal.y, m_iBasisNormal.x); // 90 degrees rotation

    // Calculate the positions of the four corner points
    out_fourCornerWorldPositions[0] = m_center + m_iBasisNormal * m_halfDimensions.x + jBasisNormal * m_halfDimensions.y; // Top-right corner
    out_fourCornerWorldPositions[1] = m_center + m_iBasisNormal * m_halfDimensions.x - jBasisNormal * m_halfDimensions.y; // Bottom-right corner
    out_fourCornerWorldPositions[2] = m_center - m_iBasisNormal * m_halfDimensions.x - jBasisNormal * m_halfDimensions.y; // Bottom-left corner
    out_fourCornerWorldPositions[3] = m_center - m_iBasisNormal * m_halfDimensions.x + jBasisNormal * m_halfDimensions.y; // Top-left corner
}

//----------------------------------------------------------------------------------------------------
// Convert world position to local position
Vec2 const OBB2::GetLocalPosForWorldPos(const Vec2& worldPosition) const
{
    Vec2  relativePos = worldPosition - m_center; // Calculate relative position
    float localX      = DotProduct2D(relativePos, m_iBasisNormal); // Project relative position onto the X-axis
    float localY      = DotProduct2D(relativePos, Vec2(-m_iBasisNormal.y, m_iBasisNormal.x)); // Project relative position onto the Y-axis

    return Vec2(localX, localY); // Return local coordinates
}

//----------------------------------------------------------------------------------------------------
// Convert local position to world position
Vec2 const OBB2::GetWorldPosForLocalPos(const Vec2& localPosition) const
{
    Vec2 worldPos = m_center; // Start from the center
    worldPos += m_iBasisNormal * localPosition.x; // Add X-axis offset
    worldPos += Vec2(-m_iBasisNormal.y, m_iBasisNormal.x) * localPosition.y; // Add Y-axis offset

    return worldPos; // Return world coordinates
}

//----------------------------------------------------------------------------------------------------
// Rotate around the center
void OBB2::RotateAboutCenter(float rotationDeltaDegrees)
{
    // Calculate the new basis vector
    float angleRadians = ConvertDegreesToRadians(rotationDeltaDegrees);
    float cosTheta     = cosf(angleRadians);
    float sinTheta     = sinf(angleRadians);

    // Update iBasisNormal
    float newX     = m_iBasisNormal.x * cosTheta - m_iBasisNormal.y * sinTheta;
    float newY     = m_iBasisNormal.x * sinTheta + m_iBasisNormal.y * cosTheta;
    m_iBasisNormal = Vec2(newX, newY).GetNormalized(); // Ensure it is still a unit vector
}

//----------------------------------------------------------------------------------------------------
// Check if a point is inside the OBB
bool OBB2::IsPointInside(Vec2 const& point) const
{
    Vec2  relativePos = point - m_center; // Calculate relative position
    float localX      = DotProduct2D(relativePos, m_iBasisNormal); // Project relative position onto the X-axis
    float localY      = DotProduct2D(relativePos, Vec2(-m_iBasisNormal.y, m_iBasisNormal.x)); // Project relative position onto the Y-axis

    // Check if local coordinates are within half dimensions
    return fabs(localX) <= m_halfDimensions.x && fabs(localY) <= m_halfDimensions.y;
}

//----------------------------------------------------------------------------------------------------
// Get the center point of the OBB
Vec2 const OBB2::GetCenter() const
{
    return m_center; // Return the center point
}

//----------------------------------------------------------------------------------------------------
// Get the full dimensions (width and height) of the OBB
Vec2 const OBB2::GetDimensions() const
{
    return m_halfDimensions * 2.0f; // Return the full dimensions
}

//----------------------------------------------------------------------------------------------------
// Set the center point of the OBB
void OBB2::SetCenter(Vec2 const& newCenter)
{
    m_center = newCenter; // Update the center point
}

//----------------------------------------------------------------------------------------------------
// Set the dimensions of the OBB
void OBB2::SetDimensions(Vec2 const& newDimensions)
{
    m_halfDimensions = newDimensions * 0.5f; // Update half dimensions
}

//----------------------------------------------------------------------------------------------------
// Get the nearest point on the OBB to a given point
Vec2 OBB2::GetNearestPoint(const Vec2& point) const
{
    // Convert the given point to local coordinates relative to the OBB
    Vec2 localPoint = GetLocalPosForWorldPos(point);

    // Clamp the local point to the OBB's half dimensions to keep it within the OBB boundaries
    localPoint.x = GetClamped(localPoint.x, -m_halfDimensions.x, m_halfDimensions.x);
    localPoint.y = GetClamped(localPoint.y, -m_halfDimensions.y, m_halfDimensions.y);

    // Convert the clamped local point back to world coordinates
    return GetWorldPosForLocalPos(localPoint);
}
