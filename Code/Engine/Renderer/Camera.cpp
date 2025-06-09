//----------------------------------------------------------------------------------------------------
// Camera.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Renderer/Camera.hpp"

#include "Window.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include <Engine/Core/EngineCommon.hpp>

#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/RaycastUtils.hpp"

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

//----------------------------------------------------------------------------------------------------
AABB2 Camera::GetViewPortUnnormalized(Vec2 const& space) const
{
    UNUSED(space)
    return m_viewPort;
}

//----------------------------------------------------------------------------------------------------
// viewport = AABB2(Vec2(0, 0.5f), Vec2::ONE)
void Camera::SetNormalizedViewport(AABB2 const& newViewPort)
{
    float x  = (float)Window::s_mainWindow->GetClientDimensions().x * newViewPort.m_maxs.x;
    float y  = (float)Window::s_mainWindow->GetClientDimensions().y * newViewPort.m_maxs.y;
    float x1 = (float)Window::s_mainWindow->GetClientDimensions().x * newViewPort.m_mins.x;
    float y1 = (float)Window::s_mainWindow->GetClientDimensions().y * newViewPort.m_mins.y;

    m_viewPort = AABB2(Vec2(x1, y1), Vec2(x, y));
}

void Camera::SetViewport(AABB2 const& newViewPort)
{
    m_viewPort = newViewPort;
}

//----------------------------------------------------------------------------------------------------
Vec2 Camera::PerspectiveWorldPosToScreen(Vec3 const& worldPos) const
{
    // Step 1: Camera Basis（依照你的座標系）
    Vec3 iBasis, jBasis, kBasis;
    m_orientation.GetAsVectors_IFwd_JLeft_KUp(iBasis, jBasis, kBasis); // X: forward, Y: left, Z: up

    // Step 2: 將 worldPos 轉換為 camera space
    Vec3  camToWorld = worldPos - m_position;
    float x_cam      = DotProduct3D(camToWorld, iBasis);  // forward (depth)
    float y_cam      = DotProduct3D(camToWorld, jBasis);  // left-right
    float z_cam      = DotProduct3D(camToWorld, kBasis);  // up-down

    if (x_cam <= 0.0001f)
    {
        // 在相機後面或太接近相機
        return Vec2(-9999.f, -9999.f); // 或可視範圍外的 sentinel 值
    }

    // Step 3: 計算投影比例
    float halfFOVDeg = m_perspectiveFOV * 0.5f;
    float halfFOVRad = ConvertDegreesToRadians(halfFOVDeg);
    float tanHalfFOV = tanf(halfFOVRad);

    // Step 4: 投影至 NDC 空間（[-1, 1]）
    float ndcY = y_cam / (x_cam * tanHalfFOV * m_perspectiveAspect); // 左右方向
    float ndcZ = z_cam / (x_cam * tanHalfFOV);                       // 上下方向

    // Step 5: 映射至螢幕座標 [0, 1]
    float screenX = 0.5f - ndcY * 0.5f; // 注意 Y 是 left，因此螢幕 X 要反過來
    float screenY = 0.5f + ndcZ * 0.5f;

    return Vec2(screenX, screenY);
}

//----------------------------------------------------------------------------------------------------
Vec3 Camera::PerspectiveScreenPosToWorld(Vec2 const& screenPos) const
{
    // // make sure this is normalized to 0-1
    // Vec3 iBasis, jBasis, kBasis;
    // m_orientation.GetAsVectors_IFwd_JLeft_KUp(iBasis, jBasis, kBasis);
    // Vec3  worldPos = m_position + iBasis * m_perspectiveNear;
    // float h        = 2.f * m_perspectiveNear * SinDegrees(m_perspectiveFOV * 0.5f) / CosDegrees(m_perspectiveFOV * 0.5f);
    // float w        = h * m_perspectiveAspect;
    // worldPos -= jBasis * (screenPos.x - 0.5f) * w;
    // worldPos += kBasis * (screenPos.y - 0.5f) * h;
    // return worldPos;
    // Step 1: 拿取 I, J, K 向量
    Vec3 iBasis, jBasis, kBasis;
    m_orientation.GetAsVectors_IFwd_JLeft_KUp(iBasis, jBasis, kBasis); // i: X (forward), j: Y (left), k: Z (up)

    // Step 2: 算出 near plane 的中心點
    Vec3 nearCenter = m_position + iBasis * m_perspectiveNear;

    // Step 3: 計算 near plane 高度和寬度（根據 FOV 與 aspect ratio）
    float halfFOV = m_perspectiveFOV * 0.5f;
    float h       = 2.f * m_perspectiveNear * SinDegrees(halfFOV) / CosDegrees(halfFOV);
    float w       = h * m_perspectiveAspect;

    // Step 4: 根據螢幕座標偏移 (0.5, 0.5) 為中心
    Vec3 worldPos = nearCenter;
    worldPos -= jBasis * ((screenPos.x - 0.5f) * w); // 往右是 -j（因為 j 是 left）
    worldPos += kBasis * ((screenPos.y - 0.5f) * h); // 往上是 +k（因為 k 是 up）

    return worldPos;
}

Ray3 Camera::ScreenPosToWorldRay(Vec2 const& screenPos) const
{
    // Get basis vectors from camera orientation
    Vec3 iBasis, jBasis, kBasis;
    m_orientation.GetAsVectors_IFwd_JLeft_KUp(iBasis, jBasis, kBasis);

    // Near plane world position
    Vec3 nearWorld = PerspectiveScreenPosToWorld(screenPos);

    // Compute far plane world position (manually, similar to PerspectiveScreenPosToWorld but using far)
    float h        = 2.f * m_perspectiveFar * SinDegrees(m_perspectiveFOV * 0.5f) / CosDegrees(m_perspectiveFOV * 0.5f);
    float w        = h * m_perspectiveAspect;
    Vec3  farWorld = m_position + iBasis * m_perspectiveFar;
    farWorld -= jBasis * (screenPos.x - 0.5f) * w;
    farWorld += kBasis * (screenPos.y - 0.5f) * h;

    // Create ray from near to far
    Ray3 ray = Ray3(nearWorld, (farWorld - nearWorld).GetNormalized(), 10.f);
    // ray.m_startPosition = nearWorld;
    // ray.m_forwardNormal = (farWorld - nearWorld).GetNormalized();

    return ray;
}
