//----------------------------------------------------------------------------------------------------
// Cylinder.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Math/Cylinder3.hpp"

//----------------------------------------------------------------------------------------------------
Cylinder3::Cylinder3(Vec3 const& startPosition, Vec3 const& endPosition, float const radius)
    : m_startPosition(startPosition),
      m_endPosition(endPosition),
      m_radius(radius)
{
}

bool Cylinder3::IsPointInside(Vec3 const& referencePoint) const
{
    return false;
}

Vec3 Cylinder3::GetNearestPoint(Vec3 const& referencePoint) const
{
    return referencePoint;
}
