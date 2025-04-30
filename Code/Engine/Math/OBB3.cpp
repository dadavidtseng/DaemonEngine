#include "Engine/Math/OBB3.hpp"
#include "Engine/Math/MathUtils.hpp"

OBB3::OBB3()
{
}

OBB3::OBB3(Vec3 const& center, Vec3 const& halfDimensions, Vec3 const& iBasis, Vec3 const& jBasis, Vec3 const& kBasis)
    : m_center(center)
      , m_halfDimensions(halfDimensions)
      , m_iBasis(iBasis)
      , m_jBasis(jBasis)
      , m_kBasis(kBasis)
{
}

bool OBB3::IsPointInside(Vec3 const& point) const
{
    Vec3 const localPos = GetLocalPosition(point);
    return localPos.x < m_halfDimensions.x && localPos.x > -m_halfDimensions.x
        && localPos.y < m_halfDimensions.y && localPos.y > -m_halfDimensions.y
        && localPos.z < m_halfDimensions.z && localPos.z > -m_halfDimensions.z;
}

Vec3 OBB3::GetNearestPoint(Vec3 const& referencePoint) const
{
    Vec3 localPos = GetLocalPosition(referencePoint);
    localPos.x    = GetClamped(localPos.x, -m_halfDimensions.x, m_halfDimensions.x);
    localPos.y    = GetClamped(localPos.y, -m_halfDimensions.y, m_halfDimensions.y);
    localPos.z    = GetClamped(localPos.z, -m_halfDimensions.z, m_halfDimensions.z);
    return GetWorldPosition(localPos);
}

void OBB3::Translate(Vec3 const& translation)
{
    m_center += translation;
}

Vec3 OBB3::GetLocalPosition(Vec3 const& worldPosition) const
{
    Vec3  CP  = worldPosition - m_center;
    float CPi = DotProduct3D(CP, m_iBasis);
    float CPj = DotProduct3D(CP, m_jBasis);
    float CPk = DotProduct3D(CP, m_kBasis);
    return Vec3(CPi, CPj, CPk);
}

Vec3 OBB3::GetWorldPosition(Vec3 const& LocalPosition) const
{
    return LocalPosition.x * m_iBasis + LocalPosition.y * m_jBasis + LocalPosition.z * m_kBasis + m_center;
}
