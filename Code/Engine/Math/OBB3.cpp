//----------------------------------------------------------------------------------------------------
// OBB3.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Math/OBB3.hpp"
#include "Engine/Math/MathUtils.hpp"

//----------------------------------------------------------------------------------------------------
OBB3::OBB3(Vec3 const& center,
           Vec3 const& halfDimensions,
           Vec3 const& iBasis,
           Vec3 const& jBasis,
           Vec3 const& kBasis)
    : m_center(center),
      m_halfDimensions(halfDimensions),
      m_iBasis(iBasis),
      m_jBasis(jBasis),
      m_kBasis(kBasis)
{
}

//----------------------------------------------------------------------------------------------------
bool OBB3::IsPointInside(Vec3 const& point) const
{
    Vec3 const localPosition = GetLocalPosition(point);

    return
        localPosition.x < m_halfDimensions.x && localPosition.x > -m_halfDimensions.x &&
        localPosition.y < m_halfDimensions.y && localPosition.y > -m_halfDimensions.y &&
        localPosition.z < m_halfDimensions.z && localPosition.z > -m_halfDimensions.z;
}

//----------------------------------------------------------------------------------------------------
Vec3 OBB3::GetNearestPoint(Vec3 const& referencePoint) const
{
    Vec3 localPosition = GetLocalPosition(referencePoint);

    localPosition.x = GetClamped(localPosition.x, -m_halfDimensions.x, m_halfDimensions.x);
    localPosition.y = GetClamped(localPosition.y, -m_halfDimensions.y, m_halfDimensions.y);
    localPosition.z = GetClamped(localPosition.z, -m_halfDimensions.z, m_halfDimensions.z);

    return GetWorldPosition(localPosition);
}

//----------------------------------------------------------------------------------------------------
void OBB3::Translate(Vec3 const& translation)
{
    m_center += translation;
}

//----------------------------------------------------------------------------------------------------
Vec3 OBB3::GetLocalPosition(Vec3 const& worldPosition) const
{
    Vec3 const  CP  = worldPosition - m_center;
    float const CPi = DotProduct3D(CP, m_iBasis);
    float const CPj = DotProduct3D(CP, m_jBasis);
    float const CPk = DotProduct3D(CP, m_kBasis);

    return Vec3(CPi, CPj, CPk);
}

//----------------------------------------------------------------------------------------------------
Vec3 OBB3::GetWorldPosition(Vec3 const& LocalPosition) const
{
    return LocalPosition.x * m_iBasis + LocalPosition.y * m_jBasis + LocalPosition.z * m_kBasis + m_center;
}
