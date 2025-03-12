//----------------------------------------------------------------------------------------------------
// Sphere3.cpp
//----------------------------------------------------------------------------------------------------

#include "Engine/Math/Sphere3.hpp"

//----------------------------------------------------------------------------------------------------
Sphere3::Sphere3(Vec3 const& centerPosition, float const radius)
    : m_centerPosition(centerPosition),
      m_radius(radius)
{
}

bool Sphere3::IsPointInside(Vec3 const& referencePoint) const
{
    return false;
}

Vec3 Sphere3::GetNearestPoint(Vec3 const& referencePoint) const
{
    // Get the nearest point
    Vec3 const  centerToPointPosition =  referencePoint-m_centerPosition;
    float const distance               = centerToPointPosition.GetLength();

    if (distance <= m_radius)
    {
        return referencePoint; // If the point is inside the circle, return the point
    }

    return m_centerPosition + centerToPointPosition.GetNormalized() * m_radius; // Return the nearest point on the circle
}
