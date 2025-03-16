//----------------------------------------------------------------------------------------------------
// OBB2.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Math/OBB2.hpp"

#include "Engine/Math/MathUtils.hpp"

//----------------------------------------------------------------------------------------------------
OBB2::OBB2(Vec2 const& center,
           Vec2 const& iBasisNormal,
           Vec2 const& halfDimensions)
    : m_center(center),
      m_iBasisNormal(iBasisNormal.GetNormalized()),
      m_halfDimensions(halfDimensions)
{
}

//----------------------------------------------------------------------------------------------------
bool OBB2::IsPointInside(Vec2 const& point) const
{
    Vec2 const localPoint = GetLocalPosFromWorldPos(point);
    Vec2 const obbMins    = -m_halfDimensions;
    Vec2 const obbMaxs    = m_halfDimensions;

    return
        localPoint.x >= obbMins.x &&
        localPoint.x <= obbMaxs.x &&
        localPoint.y >= obbMins.y &&
        localPoint.y <= obbMaxs.y;
}

//----------------------------------------------------------------------------------------------------
Vec2 OBB2::GetNearestPoint(Vec2 const& point) const
{
    if (IsPointInside(point) == true)
    {
        return point;
    }

    Vec2 localPoint = GetLocalPosFromWorldPos(point);

    localPoint.x = GetClamped(localPoint.x, -m_halfDimensions.x, m_halfDimensions.x);
    localPoint.y = GetClamped(localPoint.y, -m_halfDimensions.y, m_halfDimensions.y);

    return GetWorldPosFromLocalPos(localPoint);
}

//----------------------------------------------------------------------------------------------------
Vec2 const OBB2::GetCenter() const
{
    return m_center;
}

//----------------------------------------------------------------------------------------------------
Vec2 const OBB2::GetDimensions() const
{
    return m_halfDimensions * 2.f;
}

//----------------------------------------------------------------------------------------------------
void OBB2::GetCornerPoints(Vec2* out_fourCornerWorldPositions) const
{
    Vec2 const jBasisNormal = Vec2(-m_iBasisNormal.y, m_iBasisNormal.x);

    out_fourCornerWorldPositions[0] = m_center - m_iBasisNormal * m_halfDimensions.x - jBasisNormal * m_halfDimensions.y; // BottomLeft ( Mins )
    out_fourCornerWorldPositions[1] = m_center + m_iBasisNormal * m_halfDimensions.x - jBasisNormal * m_halfDimensions.y; // BottomRight
    out_fourCornerWorldPositions[2] = m_center + m_iBasisNormal * m_halfDimensions.x + jBasisNormal * m_halfDimensions.y; // TopRight ( Maxs )
    out_fourCornerWorldPositions[3] = m_center - m_iBasisNormal * m_halfDimensions.x + jBasisNormal * m_halfDimensions.y; // TopLeft
}

//----------------------------------------------------------------------------------------------------
Vec2 const OBB2::GetLocalPosFromWorldPos(Vec2 const& worldPosition) const
{
    Vec2 const  centerToWorldPosition = worldPosition - m_center;
    float const localX                = DotProduct2D(centerToWorldPosition, m_iBasisNormal);
    float const localY                = DotProduct2D(centerToWorldPosition, Vec2(-m_iBasisNormal.y, m_iBasisNormal.x));

    return Vec2(localX, localY);
}

//----------------------------------------------------------------------------------------------------
Vec2 const OBB2::GetWorldPosFromLocalPos(Vec2 const& localPosition) const
{
    Vec2 worldPosition = m_center;
    worldPosition += m_iBasisNormal * localPosition.x;
    worldPosition += Vec2(-m_iBasisNormal.y, m_iBasisNormal.x) * localPosition.y;

    return worldPosition;
}

//----------------------------------------------------------------------------------------------------
void OBB2::SetCenter(Vec2 const& newCenter)
{
    m_center = newCenter;
}

//----------------------------------------------------------------------------------------------------
void OBB2::SetDimensions(Vec2 const& newDimensions)
{
    m_halfDimensions = newDimensions * 0.5f;
}

//----------------------------------------------------------------------------------------------------
void OBB2::RotateAboutCenter(float const rotationDeltaDegrees)
{
    m_iBasisNormal.RotateDegrees(rotationDeltaDegrees);
}
