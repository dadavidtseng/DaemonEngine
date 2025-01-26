//----------------------------------------------------------------------------------------------------
// EulerAngles.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Math/EulerAngles.hpp"

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
}

//----------------------------------------------------------------------------------------------------
Mat44 EulerAngles::GetAsMatrix_IFwd_JLeft_KUp() const
{
    return Mat44();
}
