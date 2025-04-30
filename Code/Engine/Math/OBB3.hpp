//----------------------------------------------------------------------------------------------------
// OBB3.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include "Engine/Math/Vec3.hpp"

//----------------------------------------------------------------------------------------------------
struct OBB3
{
    OBB3();
    OBB3(Vec3 const& center, Vec3 const& halfDimensions, Vec3 const& iBasis, Vec3 const& jBasis, Vec3 const& kBasis);
    ~OBB3() = default;

    bool IsPointInside(Vec3 const& point) const;
    Vec3 GetNearestPoint(Vec3 const& referencePoint) const;
    void Translate(Vec3 const& translation);

private:
    Vec3 GetLocalPosition(Vec3 const& worldPosition) const;
    Vec3 GetWorldPosition(Vec3 const& LocalPosition) const;

public:
    Vec3 m_center;
    Vec3 m_halfDimensions;
    Vec3 m_iBasis;
    Vec3 m_jBasis;
    Vec3 m_kBasis;
};
