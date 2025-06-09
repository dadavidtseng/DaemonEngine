//----------------------------------------------------------------------------------------------------
// Camera.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Math/Mat44.hpp"
#include "Engine/Math/RaycastUtils.hpp"
#include "Engine/Math/Vec2.hpp"

struct EulerAngles;
struct Vec3;

//-----------------------------------------------------------------------------------------------
class Camera
{
public:
    enum Mode
    {
        eMode_Orthographic,
        eMode_Perspective,
        eMode_Count
    };

    void SetOrthoGraphicView(Vec2 const& bottomLeft, Vec2 const& topRight, float near = 0.f, float far = 1.f);
    void SetPerspectiveGraphicView(float aspect, float fov, float near, float far);

    void        SetPositionAndOrientation(Vec3 const& position, EulerAngles const& orientation);
    void        SetPosition(Vec3 const& position);
    Vec3        GetPosition() const;
    void        SetOrientation(EulerAngles const& orientation);
    EulerAngles GetOrientation() const;

    Mat44 GetCameraToWorldTransform() const;
    Mat44 GetWorldToCameraTransform() const;

    void  SetCameraToRenderTransform(Mat44 const& c2rTransform);
    Mat44 GetCameraToRenderTransform() const;

    Mat44 GetRenderToClipTransform() const;

    Vec2 GetOrthographicBottomLeft() const;
    Vec2 GetOrthographicTopRight() const;
    void Translate2D(Vec2 const& translation);

    Mat44 GetOrthographicMatrix() const;
    Mat44 GetPerspectiveMatrix() const;
    Mat44 GetProjectionMatrix() const;
    AABB2 GetViewPortUnnormalized(Vec2 const& space) const;
    void  SetNormalizedViewport(AABB2 const& newViewPort);
    void SetViewport(AABB2 const& newViewPort);
    Vec2  PerspectiveWorldPosToScreen(Vec3 const& worldPos) const;
    Vec3  PerspectiveScreenPosToWorld(Vec2 const& screenPos) const;
    Ray3  ScreenPosToWorldRay(Vec2 const& screenPos) const;
    Mode  m_mode = eMode_Orthographic;

private:
    Vec3        m_position    = Vec3::ZERO;
    EulerAngles m_orientation = EulerAngles::ZERO;
    AABB2       m_viewPort;

    Vec2  m_orthographicBottomLeft = Vec2::ZERO;
    Vec2  m_orthographicTopRight   = Vec2::ZERO;
    float m_orthographicNear       = 0.f;
    float m_orthographicFar        = 0.f;

    float m_perspectiveAspect = 0.f;
    float m_perspectiveFOV    = 0.f;
    float m_perspectiveNear   = 0.f;
    float m_perspectiveFar    = 0.f;

    Mat44 m_cameraToRenderTransform;
};
