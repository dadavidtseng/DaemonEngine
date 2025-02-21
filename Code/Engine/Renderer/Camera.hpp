//----------------------------------------------------------------------------------------------------
// Camera.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Math/Mat44.hpp"
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

    //----------------------------------------------------------------------------------------------------
    // void SetOrthoView(Vec2 const& bottomLeft, Vec2 const& topRight);
    // Vec2 GetOrthoBottomLeft() const;
    // Vec2 GetOrthoTopRight() const;
    // void Translate2D(Vec2 const& translation);
    //----------------------------------------------------------------------------------------------------

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

private:
    Mode m_mode = eMode_Orthographic;

    Vec3        m_position = Vec3::ZERO;
    EulerAngles m_orientation;

    Vec2  m_orthographicBottomLeft = Vec2::ZERO;
    Vec2  m_orthographicTopRight   = Vec2::ZERO;
    float m_orthographicNear       = 0.f;
    float m_orthographicFar        = 0.f;

    float m_perspectiveAspect = 0.f;
    float m_perspectiveFOV    = 0.f;
    float m_perspectiveNear   = 0.f;
    float m_perspectiveFar    = 0.f;

    Mat44 m_cameraToRenderTransform;

    //-old-variables----------------------------------------------------------------------------------------------------
    // Vec2 m_bottomLeft;
    // Vec2 m_topRight;
};
