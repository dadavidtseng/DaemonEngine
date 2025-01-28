//----------------------------------------------------------------------------------------------------
// EulerAngles.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Math/EulerAngles.hpp"

#include "Engine/Math/MathUtils.hpp"

//----------------------------------------------------------------------------------------------------
EulerAngles::EulerAngles(float const yawDegrees,
                         float const pitchDegrees,
                         float const rollDegrees)
    : m_yawDegrees(yawDegrees),
      m_pitchDegrees(pitchDegrees),
      m_rollDegrees(rollDegrees)
{
}

//----------------------------------------------------------------------------------------------------
void EulerAngles::GetAsVectors_IFwd_JLeft_KUp(Vec3& out_forwardIBasis,
                                              Vec3& out_leftJBasis,
                                              Vec3& out_upKBasis) const
{
    float const cy = CosDegrees(m_yawDegrees);
    float const sy = SinDegrees(m_yawDegrees);
    float const cp = CosDegrees(m_pitchDegrees);
    float const sp = SinDegrees(m_pitchDegrees);
    float const cr = CosDegrees(m_rollDegrees);
    float const sr = SinDegrees(m_rollDegrees);

    out_forwardIBasis = Vec3(cy * cp, sy * cp, -sp);
    out_leftJBasis    = Vec3(sr * sp * cy - sy * cr, cr * cy + sr * sp * sy, cp * sr);
    out_upKBasis      = Vec3(sr * sy + cr * sp * cy, cr * sp * sy - sr * cy, cr * cp);
}

//----------------------------------------------------------------------------------------------------
Mat44 EulerAngles::GetAsMatrix_IFwd_JLeft_KUp() const
{
    Vec3 out_forwardIBasis;
    Vec3 out_leftJBasis;
    Vec3 out_upKBasis;

    GetAsVectors_IFwd_JLeft_KUp(out_forwardIBasis, out_leftJBasis, out_upKBasis);

    return Mat44(out_forwardIBasis, out_leftJBasis, out_upKBasis, Vec3::ZERO);
}
