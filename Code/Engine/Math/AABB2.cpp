#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/MathUtils.hpp"

//-----------------------------------------------------------------------------------------------
AABB2::~AABB2() = default;

//-----------------------------------------------------------------------------------------------
AABB2::AABB2() = default;

//-----------------------------------------------------------------------------------------------
AABB2::AABB2(AABB2 const& copyFrom)
	: m_mins(copyFrom.m_mins),
	  m_maxs(copyFrom.m_maxs)
{
}

//-----------------------------------------------------------------------------------------------
AABB2::AABB2(float minX, float minY, float maxX, float maxY)
	: m_mins(minX, minY),
	  m_maxs(maxX, maxY)
{
}

//-----------------------------------------------------------------------------------------------
AABB2::AABB2(Vec2 const& mins, Vec2 const& maxs)
	: m_mins(mins),
	  m_maxs(maxs)
{
}

//-----------------------------------------------------------------------------------------------
bool AABB2::IsPointInside(Vec2 const& point) const
{
	return
		point.x >= m_mins.x &&
		point.x <= m_maxs.x &&
		point.y >= m_mins.y &&
		point.y <= m_maxs.y;
}

//-----------------------------------------------------------------------------------------------
Vec2 const AABB2::GetCenter() const
{
	float x = (m_maxs.x + m_mins.x) / 2;
	float y = (m_maxs.y + m_mins.y) / 2;

	return Vec2(x, y);
}

//-----------------------------------------------------------------------------------------------
Vec2 AABB2::GetDimensions() const
{
	float x = m_maxs.x - m_mins.x;
	float y = m_maxs.y - m_mins.y;

	return Vec2(x, y);
}

//-----------------------------------------------------------------------------------------------
Vec2 AABB2::GetNearestPoint(Vec2 const& referencePosition) const
{
	const float clampX = GetClamped(referencePosition.x, m_mins.x, m_maxs.x);
	const float clampY = GetClamped(referencePosition.y, m_mins.y, m_maxs.y);

    return Vec2(clampX, clampY);
}

//-----------------------------------------------------------------------------------------------
Vec2 AABB2::GetPointAtUV(Vec2 const& uv) const
{
	const float pointX = Interpolate(m_mins.x, m_maxs.x, uv.x);
	const float pointY = Interpolate(m_mins.y, m_maxs.y, uv.y);

	return Vec2(pointX, pointY);
}

//-----------------------------------------------------------------------------------------------
Vec2 AABB2::GetUVForPoint(Vec2 const& pointPos) const
{
	const float u = GetFractionWithinRange(pointPos.x, m_mins.x, m_maxs.x);
	const float v = GetFractionWithinRange(pointPos.y, m_mins.y, m_maxs.y);

	return Vec2(u, v);
}

//-----------------------------------------------------------------------------------------------
void AABB2::Translate(Vec2 const& translationToApply)
{
	m_mins += translationToApply;
	m_maxs += translationToApply;
}

//-----------------------------------------------------------------------------------------------
void AABB2::SetCenter(Vec2 const& newCenter)
{
	Vec2 translation = newCenter - GetCenter();

	Translate(translation);
}

//-----------------------------------------------------------------------------------------------
void AABB2::SetDimensions(Vec2 const& newDimensions)
{
	float deltaX = newDimensions.x - GetDimensions().x;
	float deltaY = newDimensions.y - GetDimensions().y;

	m_mins -= Vec2(deltaX / 2, deltaY / 2);
	m_maxs += Vec2(deltaX / 2, deltaY / 2);
}

//-----------------------------------------------------------------------------------------------
void AABB2::StretchToIncludePoint(Vec2 const& targetPointPos)
{
	if (IsPointInside(targetPointPos))
		return;

	if (targetPointPos.x < m_mins.x)
		m_mins.x = targetPointPos.x;

	if (targetPointPos.x > m_maxs.x)
		m_maxs.x = targetPointPos.x;

	if (targetPointPos.y < m_mins.y)
		m_mins.y = targetPointPos.y;

	if (targetPointPos.y > m_maxs.y)
		m_maxs.y = targetPointPos.y;
}
