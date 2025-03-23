//----------------------------------------------------------------------------------------------------
// EulerAngles.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Math/EulerAngles.hpp"

#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Math/MathUtils.hpp"

//----------------------------------------------------------------------------------------------------
STATIC EulerAngles EulerAngles::ZERO = EulerAngles(0.f, 0.f, 0.f);

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

//----------------------------------------------------------------------------------------------------
void EulerAngles::SetFromText(char const* text)
{
    // Use SplitStringOnDelimiter to divide the input text into parts based on the delimiter ','
    StringList const parts = SplitStringOnDelimiter(text, ',');

    // Input must contain exactly two parts; otherwise, reset to default values
    if (parts.size() != 3)
    {
        m_yawDegrees   = 0.f;
        m_pitchDegrees = 0.f;
        m_rollDegrees  = 0.f;

        return;
    }

    // Convert the two parts to floats using atof
    m_yawDegrees   = static_cast<float>(atof(parts[0].c_str()));
    m_pitchDegrees = static_cast<float>(atof(parts[1].c_str()));
    m_rollDegrees  = static_cast<float>(atof(parts[2].c_str()));
}
