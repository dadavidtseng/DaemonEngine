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
    return referencePoint;
}
