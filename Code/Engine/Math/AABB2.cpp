//----------------------------------------------------------------------------------------------------
// AABB2.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include <algorithm>

#include "Engine/Math/AABB2.hpp"

#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/MathUtils.hpp"

//----------------------------------------------------------------------------------------------------
STATIC AABB2 AABB2::ZERO_TO_ONE      = AABB2(Vec2(0.f, 0.f), Vec2(1.f, 1.f));
STATIC AABB2 AABB2::NEG_HALF_TO_HALF = AABB2(Vec2(-0.5f, -0.5f), Vec2(0.5f, 0.5f));

//----------------------------------------------------------------------------------------------------
AABB2::AABB2(int const minX, int const minY,
             int const maxX, int const maxY)
    : m_mins(minX, minY),
      m_maxs(maxX, maxY)
{
}

//----------------------------------------------------------------------------------------------------
AABB2::AABB2(float const minX, float const minY,
             float const maxX, float const maxY)
    : m_mins(minX, minY),
      m_maxs(maxX, maxY)
{
}

//----------------------------------------------------------------------------------------------------
AABB2::AABB2(IntVec2 const& mins,
             IntVec2 const& maxs)
    : m_mins(Vec2(mins.x, mins.y)),
      m_maxs(Vec2(maxs.x, maxs.y))
{
}

//----------------------------------------------------------------------------------------------------
AABB2::AABB2(Vec2 const& mins,
             Vec2 const& maxs)
    : m_mins(mins),
      m_maxs(maxs)
{
}

//----------------------------------------------------------------------------------------------------
bool AABB2::IsPointInside(Vec2 const& point) const
{
    return
        point.x >= m_mins.x &&
        point.x <= m_maxs.x &&
        point.y >= m_mins.y &&
        point.y <= m_maxs.y;
}

//----------------------------------------------------------------------------------------------------
Vec2 AABB2::GetNearestPoint(Vec2 const& point) const
{
    if (IsPointInside(point) == true)
    {
        return point;
    }

    float const clampedX = GetClamped(point.x, m_mins.x, m_maxs.x);
    float const clampedY = GetClamped(point.y, m_mins.y, m_maxs.y);

    return
        Vec2(clampedX, clampedY);
}

//----------------------------------------------------------------------------------------------------
Vec2 AABB2::GetCenter() const
{
    float const centerX = (m_maxs.x + m_mins.x) / 2.f;
    float const centerY = (m_maxs.y + m_mins.y) / 2.f;

    return
        Vec2(centerX, centerY);
}

//----------------------------------------------------------------------------------------------------
Vec2 AABB2::GetDimensions() const
{
    float const x = m_maxs.x - m_mins.x;
    float const y = m_maxs.y - m_mins.y;

    return
        Vec2(x, y);
}

//----------------------------------------------------------------------------------------------------
Vec2 AABB2::GetPointAtUV(Vec2 const& uv) const
{
    float const pointX = Interpolate(m_mins.x, m_maxs.x, uv.x);
    float const pointY = Interpolate(m_mins.y, m_maxs.y, uv.y);

    return Vec2(pointX, pointY);
}

//----------------------------------------------------------------------------------------------------
Vec2 AABB2::GetUVForPoint(Vec2 const& pointPos) const
{
    float const u = GetFractionWithinRange(pointPos.x, m_mins.x, m_maxs.x);
    float const v = GetFractionWithinRange(pointPos.y, m_mins.y, m_maxs.y);

    return Vec2(u, v);
}

//----------------------------------------------------------------------------------------------------
float AABB2::GetWidthOverHeightRatios() const
{
    float const width  = m_maxs.x - m_mins.x;
    float const height = m_maxs.y - m_mins.y;

    return width / height;
}

//----------------------------------------------------------------------------------------------------
void AABB2::Translate(Vec2 const& translationToApply)
{
    m_mins += translationToApply;
    m_maxs += translationToApply;
}

//----------------------------------------------------------------------------------------------------
void AABB2::SetCenter(Vec2 const& newCenter)
{
    Vec2 const translation = newCenter - GetCenter();

    Translate(translation);
}

//----------------------------------------------------------------------------------------------------
void AABB2::SetDimensions(Vec2 const& newDimensions)
{
    float const deltaX = newDimensions.x - GetDimensions().x;
    float const deltaY = newDimensions.y - GetDimensions().y;

    m_mins -= Vec2(deltaX * 0.5f, deltaY * 0.5f);
    m_maxs += Vec2(deltaX * 0.5f, deltaY * 0.5f);
}

//----------------------------------------------------------------------------------------------------
void AABB2::StretchToIncludePoint(Vec2 const& targetPointPos)
{
    if (IsPointInside(targetPointPos)) return;

    m_mins.x = std::min(targetPointPos.x, m_mins.x);
    m_mins.y = std::min(targetPointPos.y, m_mins.y);
    m_maxs.x = std::max(targetPointPos.x, m_maxs.x);
    m_maxs.y = std::max(targetPointPos.y, m_maxs.y);
}

//----------------------------------------------------------------------------------------------------
bool AABB2::operator==(const AABB2& aabb2) const
{
    return
        m_mins == aabb2.m_mins &&
        m_maxs == aabb2.m_maxs;
}
