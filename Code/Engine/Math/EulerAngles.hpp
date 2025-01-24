//----------------------------------------------------------------------------------------------------
// EulerAngles.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include "Engine/Math/Mat44.hpp"

struct Vec3;

//----------------------------------------------------------------------------------------------------
struct EulerAngles
{
    EulerAngles() = default;
    EulerAngles(float yawDegrees, float pitchDegrees, float rollDegrees);
    void  GetAsVectors_IFwd_JLeft_KUp(Vec3& out_forwardIBasis, Vec3& out_leftJBasis, Vec3& out_upKBasis) const;
    Mat44 GetAsMatrix_IFwd_JLeft_KUp() const;

    float m_yawDegrees   = 0.f;
    float m_pitchDegrees = 0.f;
    float m_rollDegrees  = 0.f;
};
