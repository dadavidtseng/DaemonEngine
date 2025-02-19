//-----------------------------------------------------------------------------------------------
// Camera.cpp
//

//-----------------------------------------------------------------------------------------------
#include "Engine/Renderer/Camera.hpp"

// //-----------------------------------------------------------------------------------------------
// void Camera::SetOrthoView(Vec2 const& bottomLeft, Vec2 const& topRight)
// {
//     m_bottomLeft = bottomLeft;
//     m_topRight   = topRight;
// }
//
// //-----------------------------------------------------------------------------------------------
// Vec2 Camera::GetOrthoBottomLeft() const
// {
//     return m_bottomLeft;
// }
//
// //-----------------------------------------------------------------------------------------------
// Vec2 Camera::GetOrthoTopRight() const
// {
//     return m_topRight;
// }
//
// //-----------------------------------------------------------------------------------------------
// void Camera::Translate2D(Vec2 const& translation)
// {
//     m_bottomLeft += translation;
//     m_topRight += translation;
// }

void Camera::SetOrthoGraphicView(Vec2 const& bottomLeft, Vec2 const& topRight, float const near, float const far)
{
    m_orthographicBottomLeft = bottomLeft;
    m_orthographicTopRight   = topRight;
    m_orthographicNear       = near;
    m_orthographicFar        = far;
}

void Camera::SetPerspectiveGraphicView(float const aspect, float const fov, float const near, float const far)
{
    m_perspectiveAspect = aspect;
    m_perspectiveFOV    = fov;
    m_perspectiveNear   = near;
    m_perspectiveFar    = far;
}

void Camera::SetPositionAndOrientation(Vec3 const& position, EulerAngles const& orientation)
{
    m_position    = position;
    m_orientation = orientation;
}

//----------------------------------------------------------------------------------------------------
void Camera::SetPosition(Vec3 const& position)
{
    m_position = position;
}

//----------------------------------------------------------------------------------------------------
Vec3 Camera::GetPosition() const
{
    return m_position;
}

void Camera::SetOrientation(EulerAngles const& orientation)
{
    m_orientation = orientation;
}

EulerAngles Camera::GetOrientation() const
{
    return m_orientation;
}

Mat44 Camera::GetCameraToWorldTransform() const
{
    Mat44 c2w;

    c2w.AppendZRotation(m_orientation.m_yawDegrees);
    c2w.AppendYRotation(m_orientation.m_pitchDegrees);
    c2w.AppendXRotation(m_orientation.m_rollDegrees);

    c2w.AppendTranslation3D(m_position);

    return c2w;
}

Mat44 Camera::GetWorldToCameraTransform() const
{
    return GetCameraToWorldTransform().GetOrthonormalInverse();
}

void Camera::SetCameraToRenderTransform(Mat44 const& m)
{
    m_cameraToRenderTransform = m;
}

Mat44 Camera::GetCameraToRenderTransform() const
{
    return m_cameraToRenderTransform;
}

//----------------------------------------------------------------------------------------------------
Mat44 Camera::GetRenderToClipTransform() const
{
    // Vec2  orthographicTopLeft  = m_orthographicBottomLeft;
    // Vec2  orthographicTopRight = m_orthographicTopRight;
    // float left                 = orthographicTopLeft.x;
    // float right                = orthographicTopRight.x;
    // float bottom               = orthographicTopRight.y;
    // float top                  = orthographicTopLeft.y;
    //
    // return Mat44::MakeOrthoProjection(left, right, bottom, top, m_orthographicNear, m_orthographicFar);

    return Mat44::MakePerspectiveProjection(m_perspectiveFOV, m_perspectiveAspect, m_perspectiveNear, m_perspectiveFar);
}

//----------------------------------------------------------------------------------------------------
Vec2 Camera::GetOrthographicBottomLeft() const
{
    return m_orthographicBottomLeft;
}

//----------------------------------------------------------------------------------------------------
Vec2 Camera::GetOrthographicTopRight() const
{
    return m_orthographicTopRight;
}

void Camera::Translate2D(Vec2 const& translation)
{
    m_orthographicBottomLeft += translation;
    m_orthographicTopRight += translation;
}

Mat44 Camera::GetOrthographicMatrix() const
{
    return {};
}

Mat44 Camera::GetPerspectiveMatrix() const
{
    return {};
}

Mat44 Camera::GetProjectionMatrix() const
{
    return {};
}
