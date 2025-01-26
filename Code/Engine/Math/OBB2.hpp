//-----------------------------------------------------------------------------------------------
// OBB2.hpp
//-----------------------------------------------------------------------------------------------

#pragma once
#include "Engine/Math/Vec2.hpp"

//-----------------------------------------------------------------------------------------------
struct OBB2
{
    Vec2 m_center         = Vec2::ZERO;
    Vec2 m_iBasisNormal   = Vec2::ZERO;
    Vec2 m_halfDimensions = Vec2::ZERO;

    OBB2()                     = default;
    OBB2(OBB2 const& copyFrom) = default;
    explicit OBB2(Vec2 const& center, Vec2 const& iBasisNormal, Vec2 const& halfDimensions);

    void       GetCornerPoints(Vec2* out_fourCornerWorldPositions) const;
    Vec2 const GetLocalPosForWorldPos(Vec2 const& worldPosition) const;
    Vec2 const GetWorldPosForLocalPos(Vec2 const& localPosition) const;
    void       RotateAboutCenter(float rotationDeltaDegrees);

    bool       IsPointInside(Vec2 const& point) const;
    Vec2 const GetCenter() const;
    Vec2 const GetDimensions() const;
    void       SetCenter(Vec2 const& newCenter);
    void       SetDimensions(Vec2 const& newDimensions);
    Vec2       GetNearestPoint(Vec2 const& point) const;
};
