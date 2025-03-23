//----------------------------------------------------------------------------------------------------
// AABB3.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Math/AABB3.hpp"

#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Math/MathUtils.hpp"

//----------------------------------------------------------------------------------------------------
STATIC AABB3 AABB3::ZERO_TO_ONE      = AABB3(Vec3(0.f, 0.f, 0.f), Vec3(1.f, 1.f, 1.f));
STATIC AABB3 AABB3::NEG_HALF_TO_HALF = AABB3(Vec3(-0.5f, -0.5f, -0.5f), Vec3(0.5f, 0.5f, 0.5f));
STATIC AABB3 AABB3::NEG_ONE = AABB3(Vec3(-1.f, -1.f, -1.f), Vec3(-1.f, -1.f, -1.f));

//----------------------------------------------------------------------------------------------------
AABB3::AABB3(float const minX, float const minY, float const minZ,
             float const maxX, float const maxY, float const maxZ)
    : m_mins(Vec3(minX, minY, minZ)),
      m_maxs(Vec3(maxX, maxY, maxZ))
{
}

//----------------------------------------------------------------------------------------------------
AABB3::AABB3(Vec3 const& mins,
             Vec3 const& maxs)
    : m_mins(mins),
      m_maxs(maxs)
{
}

//----------------------------------------------------------------------------------------------------
bool AABB3::IsPointInside(Vec3 const& point) const
{
    return
        point.x >= m_mins.x &&
        point.x <= m_maxs.x &&
        point.y >= m_mins.y &&
        point.y <= m_maxs.y &&
        point.z >= m_mins.z &&
        point.z <= m_maxs.z;
}

//----------------------------------------------------------------------------------------------------
Vec3 AABB3::GetNearestPoint(Vec3 const& point) const
{
    float const clampedX = GetClamped(point.x, m_mins.x, m_maxs.x);
    float const clampedY = GetClamped(point.y, m_mins.y, m_maxs.y);
    float const clampedZ = GetClamped(point.z, m_mins.z, m_maxs.z);

    return Vec3(clampedX, clampedY, clampedZ);
}
