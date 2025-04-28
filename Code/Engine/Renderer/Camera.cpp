//----------------------------------------------------------------------------------------------------
// Camera.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Renderer/Camera.hpp"

#include "Window.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"

//----------------------------------------------------------------------------------------------------
void Camera::SetOrthoGraphicView(Vec2 const& bottomLeft,
                                 Vec2 const& topRight,
                                 float const near,
                                 float const far)
{
    m_orthographicBottomLeft = bottomLeft;
    m_orthographicTopRight   = topRight;
    m_orthographicNear       = near;
    m_orthographicFar        = far;

    m_mode = eMode_Orthographic;
}

//----------------------------------------------------------------------------------------------------
void Camera::SetPerspectiveGraphicView(float const aspect,
                                       float const fov,
                                       float const near,
                                       float const far)
{
    m_perspectiveAspect = aspect;
    m_perspectiveFOV    = fov;
    m_perspectiveNear   = near;
    m_perspectiveFar    = far;

    m_mode = eMode_Perspective;
}

//----------------------------------------------------------------------------------------------------
void Camera::SetPositionAndOrientation(Vec3 const&        position,
                                       EulerAngles const& orientation)
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

//----------------------------------------------------------------------------------------------------
void Camera::SetOrientation(EulerAngles const& orientation)
{
    m_orientation = orientation;
}

//----------------------------------------------------------------------------------------------------
EulerAngles Camera::GetOrientation() const
{
    return m_orientation;
}

//----------------------------------------------------------------------------------------------------
Mat44 Camera::GetCameraToWorldTransform() const
{
    Mat44 c2w;

    c2w.AppendTranslation3D(m_position);
    c2w.Append(m_orientation.GetAsMatrix_IFwd_JLeft_KUp());

    return c2w;
}

//----------------------------------------------------------------------------------------------------
Mat44 Camera::GetWorldToCameraTransform() const
{
    return GetCameraToWorldTransform().GetOrthonormalInverse();
}

//----------------------------------------------------------------------------------------------------
void Camera::SetCameraToRenderTransform(Mat44 const& c2rTransform)
{
    m_cameraToRenderTransform = c2rTransform;
}

//----------------------------------------------------------------------------------------------------
Mat44 Camera::GetCameraToRenderTransform() const
{
    return m_cameraToRenderTransform;
}

//----------------------------------------------------------------------------------------------------
Mat44 Camera::GetRenderToClipTransform() const
{
    Mat44 r2c;

    switch (m_mode)
    {
    case eMode_Orthographic:
        {
            float const left   = m_orthographicBottomLeft.x;
            float const right  = m_orthographicTopRight.x;
            float const bottom = m_orthographicBottomLeft.y;
            float const top    = m_orthographicTopRight.y;

            r2c = Mat44::MakeOrthoProjection(left, right, bottom, top, m_orthographicNear, m_orthographicFar);

            break;
        }

    case eMode_Perspective:
        {
            r2c = Mat44::MakePerspectiveProjection(m_perspectiveFOV, m_perspectiveAspect, m_perspectiveNear, m_perspectiveFar);

            break;
        }
    case eMode_Count:
        {
            ERROR_RECOVERABLE("You haven't set the m_mode yet!")

            break;
        }
    }

    return r2c;
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

//----------------------------------------------------------------------------------------------------
void Camera::Translate2D(Vec2 const& translation)
{
    m_orthographicBottomLeft += translation;
    m_orthographicTopRight += translation;
}

//----------------------------------------------------------------------------------------------------
Mat44 Camera::GetOrthographicMatrix() const
{
    Vec2 const  orthographicTopLeft  = m_orthographicBottomLeft;
    Vec2 const  orthographicTopRight = m_orthographicTopRight;
    float const left                 = orthographicTopLeft.x;
    float const right                = orthographicTopRight.x;
    float const bottom               = orthographicTopRight.y;
    float const top                  = orthographicTopLeft.y;

    Mat44 const r2c = Mat44::MakeOrthoProjection(left, right, bottom, top, m_orthographicNear, m_orthographicFar);

    return r2c;
}

//----------------------------------------------------------------------------------------------------
Mat44 Camera::GetPerspectiveMatrix() const
{
    Mat44 const r2c = Mat44::MakePerspectiveProjection(m_perspectiveFOV, m_perspectiveAspect, m_perspectiveNear, m_perspectiveFar);

    return r2c;
}

//----------------------------------------------------------------------------------------------------
Mat44 Camera::GetProjectionMatrix() const
{
    return GetRenderToClipTransform();
}

AABB2 Camera::GetViewPortUnnormalized(Vec2 const& vec2)
{
    return m_viewPort;
}

// viewport = AABB2(Vec2(0, 0.5f), Vec2::ONE)
void Camera::SetNormalizedViewport(AABB2 const& viewPort)
{
    float x = (float)Window::s_mainWindow->GetClientDimensions().x * viewPort.m_maxs.x;
    float y = (float)Window::s_mainWindow->GetClientDimensions().y * viewPort.m_maxs.y;
    float x1 = (float)Window::s_mainWindow->GetClientDimensions().x * viewPort.m_mins.x;
    float y1 = (float)Window::s_mainWindow->GetClientDimensions().y * viewPort.m_mins.y;

    m_viewPort = AABB2(Vec2(x1, y1), Vec2(x, y));
}
