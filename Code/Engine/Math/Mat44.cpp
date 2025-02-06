//----------------------------------------------------------------------------------------------------
// Mat44.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Math/Mat44.hpp"

#include "Engine/Math/MathUtils.hpp"

//----------------------------------------------------------------------------------------------------
Mat44::Mat44()
{
    m_values[Ix] = 1.f;
    m_values[Iy] = 0.f;
    m_values[Iz] = 0.f;
    m_values[Iw] = 0.f;
    m_values[Jx] = 0.f;
    m_values[Jy] = 1.f;
    m_values[Jz] = 0.f;
    m_values[Jw] = 0.f;
    m_values[Kx] = 0.f;
    m_values[Ky] = 0.f;
    m_values[Kz] = 1.f;
    m_values[Kw] = 0.f;
    m_values[Tx] = 0.f;
    m_values[Ty] = 0.f;
    m_values[Tz] = 0.f;
    m_values[Tw] = 1.f;
}

//----------------------------------------------------------------------------------------------------
Mat44::Mat44(Vec2 const& iBasis2D, Vec2 const& jBasis2D, Vec2 const& translation2D)
{
    m_values[Ix] = iBasis2D.x;
    m_values[Iy] = iBasis2D.y;
    m_values[Iz] = 0.0f;
    m_values[Iw] = 0.0f;

    m_values[Jx] = jBasis2D.x;
    m_values[Jy] = jBasis2D.y;
    m_values[Jz] = 0.0f;
    m_values[Jw] = 0.0f;

    m_values[Kx] = 0.0f;
    m_values[Ky] = 0.0f;
    m_values[Kz] = 1.0f;
    m_values[Kw] = 0.0f;

    m_values[Tx] = translation2D.x;
    m_values[Ty] = translation2D.y;
    m_values[Tz] = 0.0f;
    m_values[Tw] = 1.0f;
}

//----------------------------------------------------------------------------------------------------
Mat44::Mat44(Vec3 const& iBasis3D, Vec3 const& jBasis3D, Vec3 const& kBasis3D, Vec3 const& translation3D)
{
    m_values[Ix] = iBasis3D.x;
    m_values[Iy] = iBasis3D.y;
    m_values[Iz] = iBasis3D.z;
    m_values[Iw] = 0.f;

    m_values[Jx] = jBasis3D.x;
    m_values[Jy] = jBasis3D.y;
    m_values[Jz] = jBasis3D.z;
    m_values[Jw] = 0.f;

    m_values[Kx] = kBasis3D.x;
    m_values[Ky] = kBasis3D.y;
    m_values[Kz] = kBasis3D.z;
    m_values[Kw] = 0.f;

    m_values[Tx] = translation3D.x;
    m_values[Ty] = translation3D.y;
    m_values[Tz] = translation3D.z;
    m_values[Tw] = 1.f;
}

//----------------------------------------------------------------------------------------------------
Mat44::Mat44(Vec4 const& iBasis4D, Vec4 const& jBasis4D, Vec4 const& kBasis4D, Vec4 const& translation4D)
{
    m_values[Ix] = iBasis4D.x;
    m_values[Iy] = iBasis4D.y;
    m_values[Iz] = iBasis4D.z;
    m_values[Iw] = iBasis4D.w;

    m_values[Jx] = jBasis4D.x;
    m_values[Jy] = jBasis4D.y;
    m_values[Jz] = jBasis4D.z;
    m_values[Jw] = jBasis4D.w;

    m_values[Kx] = kBasis4D.x;
    m_values[Ky] = kBasis4D.y;
    m_values[Kz] = kBasis4D.z;
    m_values[Kw] = kBasis4D.w;

    m_values[Tx] = translation4D.x;
    m_values[Ty] = translation4D.y;
    m_values[Tz] = translation4D.z;
    m_values[Tw] = translation4D.w;
}

//----------------------------------------------------------------------------------------------------
Mat44::Mat44(float const* sixteenValuesBasisMajor)
{
    for (int i = 0; i < 16; ++i)
    {
        m_values[i] = sixteenValuesBasisMajor[i];
    }
}

//----------------------------------------------------------------------------------------------------
Mat44 const Mat44::MakeTranslation2D(Vec2 const& translationXY)
{
    Mat44 matrix;

    matrix.m_values[Tx] = translationXY.x;
    matrix.m_values[Ty] = translationXY.y;

    return matrix;
}

//----------------------------------------------------------------------------------------------------
Mat44 const Mat44::MakeTranslation3D(Vec3 const& translationXYZ)
{
    Mat44 matrix;

    matrix.m_values[Tx] = translationXYZ.x;
    matrix.m_values[Ty] = translationXYZ.y;
    matrix.m_values[Tz] = translationXYZ.z;

    return matrix;
}

//----------------------------------------------------------------------------------------------------
Mat44 const Mat44::MakeUniformScale2D(float const uniformScaleXY)
{
    Mat44 matrix;

    matrix.m_values[Ix] = uniformScaleXY;
    matrix.m_values[Jy] = uniformScaleXY;

    return matrix;
}

//----------------------------------------------------------------------------------------------------
Mat44 const Mat44::MakeUniformScale3D(float const uniformScaleXYZ)
{
    Mat44 matrix;

    matrix.m_values[Ix] = uniformScaleXYZ;
    matrix.m_values[Jy] = uniformScaleXYZ;
    matrix.m_values[Kz] = uniformScaleXYZ;

    return matrix;
}

//----------------------------------------------------------------------------------------------------
Mat44 const Mat44::MakeNonUniformScale2D(Vec2 const& nonUniformScaleXY)
{
    Mat44 matrix;

    matrix.m_values[Ix] = nonUniformScaleXY.x;
    matrix.m_values[Jy] = nonUniformScaleXY.y;

    return matrix;
}

//----------------------------------------------------------------------------------------------------
Mat44 const Mat44::MakeNonUniformScale3D(Vec3 const& nonUniformScaleXYZ)
{
    Mat44 matrix;

    matrix.m_values[Ix] = nonUniformScaleXYZ.x;
    matrix.m_values[Jy] = nonUniformScaleXYZ.y;
    matrix.m_values[Kz] = nonUniformScaleXYZ.z;

    return matrix;
}

//----------------------------------------------------------------------------------------------------
Mat44 const Mat44::MakeZRotationDegrees(float const rotationDegreesAboutZ)
{
    float const cosTheta = CosDegrees(rotationDegreesAboutZ);
    float const sinTheta = SinDegrees(rotationDegreesAboutZ);

    Mat44 matrix;

    matrix.m_values[Ix] = cosTheta;
    matrix.m_values[Iy] = sinTheta;
    matrix.m_values[Jx] = -sinTheta;
    matrix.m_values[Jy] = cosTheta;

    return matrix;
}

//----------------------------------------------------------------------------------------------------
Mat44 const Mat44::MakeYRotationDegrees(float const rotationDegreesAboutY)
{
    float const cosTheta = CosDegrees(rotationDegreesAboutY);
    float const sinTheta = SinDegrees(rotationDegreesAboutY);

    Mat44 matrix;

    matrix.m_values[Ix] = cosTheta;
    matrix.m_values[Iz] = -sinTheta;
    matrix.m_values[Kx] = sinTheta;
    matrix.m_values[Kz] = cosTheta;

    return matrix;
}

//----------------------------------------------------------------------------------------------------
Mat44 const Mat44::MakeXRotationDegrees(float const rotationDegreesAboutX)
{
    float const cosTheta = CosDegrees(rotationDegreesAboutX);
    float const sinTheta = SinDegrees(rotationDegreesAboutX);

    Mat44 matrix;

    matrix.m_values[Jy] = cosTheta;
    matrix.m_values[Jz] = sinTheta;
    matrix.m_values[Ky] = -sinTheta;
    matrix.m_values[Kz] = cosTheta;

    return matrix;
}

Mat44 const Mat44::MakeOrthoProjection(float left, float right, float bottom, float top, float zNear, float zFar)
{
    return {};
}

Mat44 const Mat44::MakePerspectiveProjection(float fovYDegrees, float aspect, float zNear, float zFar)
{
    return {};
}

//----------------------------------------------------------------------------------------------------
Vec2 const Mat44::TransformVectorQuantity2D(Vec2 const& vectorQuantityXY) const
{
    float const x =
        m_values[Ix] * vectorQuantityXY.x +
        m_values[Jx] * vectorQuantityXY.y;

    float const y =
        m_values[Iy] * vectorQuantityXY.x +
        m_values[Jy] * vectorQuantityXY.y;

    return Vec2(x, y);
}

//----------------------------------------------------------------------------------------------------
Vec3 const Mat44::TransformVectorQuantity3D(Vec3 const& vectorQuantityXYZ) const
{
    float const x =
        m_values[Ix] * vectorQuantityXYZ.x +
        m_values[Jx] * vectorQuantityXYZ.y +
        m_values[Kx] * vectorQuantityXYZ.z;

    float const y =
        m_values[Iy] * vectorQuantityXYZ.x +
        m_values[Jy] * vectorQuantityXYZ.y +
        m_values[Ky] * vectorQuantityXYZ.z;

    float const z =
        m_values[Iz] * vectorQuantityXYZ.x +
        m_values[Jz] * vectorQuantityXYZ.y +
        m_values[Kz] * vectorQuantityXYZ.z;

    return Vec3(x, y, z);
}

//----------------------------------------------------------------------------------------------------
Vec2 const Mat44::TransformPosition2D(Vec2 const& positionXY) const
{
    float const x =
        m_values[Ix] * positionXY.x +
        m_values[Jx] * positionXY.y +
        m_values[Tx];

    float const y =
        m_values[Iy] * positionXY.x +
        m_values[Jy] * positionXY.y +
        m_values[Ty];

    return Vec2(x, y);
}

//----------------------------------------------------------------------------------------------------
Vec3 const Mat44::TransformPosition3D(Vec3 const& position3D) const
{
    float const x =
        m_values[Ix] * position3D.x +
        m_values[Jx] * position3D.y +
        m_values[Kx] * position3D.z +
        m_values[Tx];

    float const y =
        m_values[Iy] * position3D.x +
        m_values[Jy] * position3D.y +
        m_values[Ky] * position3D.z +
        m_values[Ty];

    float const z =
        m_values[Iz] * position3D.x +
        m_values[Jz] * position3D.y +
        m_values[Kz] * position3D.z +
        m_values[Tz];

    return Vec3(x, y, z);
}

//----------------------------------------------------------------------------------------------------
Vec4 const Mat44::TransformHomogeneous3D(Vec4 const& homogeneousPoint3D) const
{
    float const x =
        m_values[Ix] * homogeneousPoint3D.x +
        m_values[Jx] * homogeneousPoint3D.y +
        m_values[Kx] * homogeneousPoint3D.z +
        m_values[Tx] * homogeneousPoint3D.w;

    float const y =
        m_values[Iy] * homogeneousPoint3D.x +
        m_values[Jy] * homogeneousPoint3D.y +
        m_values[Ky] * homogeneousPoint3D.z +
        m_values[Ty] * homogeneousPoint3D.w;

    float const z =
        m_values[Iz] * homogeneousPoint3D.x +
        m_values[Jz] * homogeneousPoint3D.y +
        m_values[Kz] * homogeneousPoint3D.z +
        m_values[Tz] * homogeneousPoint3D.w;

    float const w =
        m_values[Iw] * homogeneousPoint3D.x +
        m_values[Jw] * homogeneousPoint3D.y +
        m_values[Kw] * homogeneousPoint3D.z +
        m_values[Tw] * homogeneousPoint3D.w;

    return Vec4(x, y, z, w);
}

//----------------------------------------------------------------------------------------------------
float* Mat44::GetAsFloatArray()
{
    return m_values;
}

//----------------------------------------------------------------------------------------------------
float const* Mat44::GetAsFloatArray() const
{
    return m_values;
}

//----------------------------------------------------------------------------------------------------
Vec2 const Mat44::GetIBasis2D() const
{
    return Vec2(m_values[Ix], m_values[Iy]);
}

//----------------------------------------------------------------------------------------------------
Vec2 const Mat44::GetJBasis2D() const
{
    return Vec2(m_values[Jx], m_values[Jy]);
}

//----------------------------------------------------------------------------------------------------
Vec2 const Mat44::GetTranslation2D() const
{
    return Vec2(m_values[Tx], m_values[Ty]);
}

//----------------------------------------------------------------------------------------------------
Vec3 const Mat44::GetIBasis3D() const
{
    return Vec3(m_values[Ix], m_values[Iy], m_values[Iz]);
}

//----------------------------------------------------------------------------------------------------
Vec3 const Mat44::GetJBasis3D() const
{
    return Vec3(m_values[Jx], m_values[Jy], m_values[Jz]);
}

//----------------------------------------------------------------------------------------------------
Vec3 const Mat44::GetKBasis3D() const
{
    return Vec3(m_values[Kx], m_values[Ky], m_values[Kz]);
}

//----------------------------------------------------------------------------------------------------
Vec3 const Mat44::GetTranslation3D() const
{
    return Vec3(m_values[Tx], m_values[Ty], m_values[Tz]);
}

//----------------------------------------------------------------------------------------------------
Vec4 const Mat44::GetIBasis4D() const
{
    return Vec4(m_values[Ix], m_values[Iy], m_values[Iz], m_values[Iw]);
}

//----------------------------------------------------------------------------------------------------
Vec4 const Mat44::GetJBasis4D() const
{
    return Vec4(m_values[Jx], m_values[Jy], m_values[Jz], m_values[Jw]);
}

//----------------------------------------------------------------------------------------------------
Vec4 const Mat44::GetKBasis4D() const
{
    return Vec4(m_values[Kx], m_values[Ky], m_values[Kz], m_values[Kw]);
}

//----------------------------------------------------------------------------------------------------
Vec4 const Mat44::GetTranslation4D() const
{
    return Vec4(m_values[Tx], m_values[Ty], m_values[Tz], m_values[Tw]);
}

Mat44 const Mat44::GetOrthonormalInverse() const
{
    return {};
}

//----------------------------------------------------------------------------------------------------
void Mat44::SetTranslation2D(Vec2 const& translationXY)
{
    m_values[Tx] = translationXY.x;
    m_values[Ty] = translationXY.y;
    m_values[Tz] = 0.f;
    m_values[Tw] = 1.f;
}

//----------------------------------------------------------------------------------------------------
void Mat44::SetTranslation3D(Vec3 const& translationXYZ)
{
    m_values[Tx] = translationXYZ.x;
    m_values[Ty] = translationXYZ.y;
    m_values[Tz] = translationXYZ.z;
    m_values[Tw] = 1.f;
}

//----------------------------------------------------------------------------------------------------
void Mat44::SetIJ2D(Vec2 const& iBasis2D, Vec2 const& jBasis2D)
{
    m_values[Ix] = iBasis2D.x;
    m_values[Iy] = iBasis2D.y;
    m_values[Iz] = 0.f;
    m_values[Iw] = 0.f;

    m_values[Jx] = jBasis2D.x;
    m_values[Jy] = jBasis2D.y;
    m_values[Jz] = 0.f;
    m_values[Jw] = 0.f;
}

//----------------------------------------------------------------------------------------------------
void Mat44::SetIJT2D(Vec2 const& iBasis2D, Vec2 const& jBasis2D, Vec2 const& translationXY)
{
    SetIJ2D(iBasis2D, jBasis2D);
    SetTranslation2D(translationXY);
}

//----------------------------------------------------------------------------------------------------
void Mat44::SetIJK3D(Vec3 const& iBasis3D, Vec3 const& jBasis3D, Vec3 const& kBasis3D)
{
    m_values[Ix] = iBasis3D.x;
    m_values[Iy] = iBasis3D.y;
    m_values[Iz] = iBasis3D.z;
    m_values[Iw] = 0.f;

    m_values[Jx] = jBasis3D.x;
    m_values[Jy] = jBasis3D.y;
    m_values[Jz] = jBasis3D.z;
    m_values[Jw] = 0.f;

    m_values[Kx] = kBasis3D.x;
    m_values[Ky] = kBasis3D.y;
    m_values[Kz] = kBasis3D.z;
    m_values[Kw] = 0.f;
}

//----------------------------------------------------------------------------------------------------
void Mat44::SetIJKT3D(Vec3 const& iBasis3D, Vec3 const& jBasis3D, Vec3 const& kBasis3D, Vec3 const& translationXYZ)
{
    SetIJK3D(iBasis3D, jBasis3D, kBasis3D);
    SetTranslation3D(translationXYZ);
}

//----------------------------------------------------------------------------------------------------
void Mat44::SetIJKT4D(Vec4 const& iBasis4D, Vec4 const& jBasis4D, Vec4 const& kBasis4D, Vec4 const& translation4D)
{
    m_values[Ix] = iBasis4D.x;
    m_values[Iy] = iBasis4D.y;
    m_values[Iz] = iBasis4D.z;
    m_values[Iw] = iBasis4D.w;

    m_values[Jx] = jBasis4D.x;
    m_values[Jy] = jBasis4D.y;
    m_values[Jz] = jBasis4D.z;
    m_values[Jw] = jBasis4D.w;

    m_values[Kx] = kBasis4D.x;
    m_values[Ky] = kBasis4D.y;
    m_values[Kz] = kBasis4D.z;
    m_values[Kw] = kBasis4D.w;

    m_values[Tx] = translation4D.x;
    m_values[Ty] = translation4D.y;
    m_values[Tz] = translation4D.z;
    m_values[Tw] = translation4D.w;
}

void Mat44::Transpose()
{
}

void Mat44::Orthonormalize_XFwd_YLeft_ZUp()
{
}

//----------------------------------------------------------------------------------------------------
// Subsequent point transformations will be affected by the last-appended matrix first,
// followed by the previously appended matrices, in reverse order.
//
// In different notations, the transformations are represented as follows:
//
//  this(append(p))     // Function notation: apply the 'append' matrix to point 'p' first, 
//                      // then apply the 'this' matrix.
//
//  [this][append][p]   // Column-major notation: the point 'p' is transformed by the 
//                      // 'append' matrix first, then by the 'this' matrix (right-to-left order).
//
//  [p][append][this]   // Row-major notation: the point 'p' is transformed by the 
//                      // 'append' matrix first, then by the 'this' matrix (left-to-right order).
//
void Mat44::Append(Mat44 const& appendThis)
{
    Mat44 const  copyOfThis = *this;
    float const* left       = &copyOfThis.m_values[0];
    float const* right      = &appendThis.m_values[0];

    m_values[Ix] = left[Ix] * right[Ix] + left[Jx] * right[Iy] + left[Kx] * right[Iz] + left[Tx] * right[Iw];
    m_values[Iy] = left[Iy] * right[Ix] + left[Jy] * right[Iy] + left[Ky] * right[Iz] + left[Ty] * right[Iw];
    m_values[Iz] = left[Iz] * right[Ix] + left[Jz] * right[Iy] + left[Kz] * right[Iz] + left[Tz] * right[Iw];
    m_values[Iw] = left[Iw] * right[Ix] + left[Jw] * right[Iy] + left[Kw] * right[Iz] + left[Tw] * right[Iw];

    m_values[Jx] = left[Ix] * right[Jx] + left[Jx] * right[Jy] + left[Kx] * right[Jz] + left[Tx] * right[Jw];
    m_values[Jy] = left[Iy] * right[Jx] + left[Jy] * right[Jy] + left[Ky] * right[Jz] + left[Ty] * right[Jw];
    m_values[Jz] = left[Iz] * right[Jx] + left[Jz] * right[Jy] + left[Kz] * right[Jz] + left[Tz] * right[Jw];
    m_values[Jw] = left[Iw] * right[Jx] + left[Jw] * right[Jy] + left[Kw] * right[Jz] + left[Tw] * right[Jw];

    m_values[Kx] = left[Ix] * right[Kx] + left[Jx] * right[Ky] + left[Kx] * right[Kz] + left[Tx] * right[Kw];
    m_values[Ky] = left[Iy] * right[Kx] + left[Jy] * right[Ky] + left[Ky] * right[Kz] + left[Ty] * right[Kw];
    m_values[Kz] = left[Iz] * right[Kx] + left[Jz] * right[Ky] + left[Kz] * right[Kz] + left[Tz] * right[Kw];
    m_values[Kw] = left[Iw] * right[Kx] + left[Jw] * right[Ky] + left[Kw] * right[Kz] + left[Tw] * right[Kw];

    m_values[Tx] = left[Ix] * right[Tx] + left[Jx] * right[Ty] + left[Kx] * right[Tz] + left[Tx] * right[Tw];
    m_values[Ty] = left[Iy] * right[Tx] + left[Jy] * right[Ty] + left[Ky] * right[Tz] + left[Ty] * right[Tw];
    m_values[Tz] = left[Iz] * right[Tx] + left[Jz] * right[Ty] + left[Kz] * right[Tz] + left[Tz] * right[Tw];
    m_values[Tw] = left[Iw] * right[Tx] + left[Jw] * right[Ty] + left[Kw] * right[Tz] + left[Tw] * right[Tw];
}

//----------------------------------------------------------------------------------------------------
void Mat44::AppendZRotation(float const degreesRotationAboutZ)
{
    Mat44 const rotationMatrix = MakeZRotationDegrees(degreesRotationAboutZ);
    Append(rotationMatrix);
}

//----------------------------------------------------------------------------------------------------
void Mat44::AppendYRotation(float const degreesRotationAboutY)
{
    Mat44 const rotationMatrix = MakeYRotationDegrees(degreesRotationAboutY);
    Append(rotationMatrix);
}

//----------------------------------------------------------------------------------------------------
void Mat44::AppendXRotation(float const degreesRotationAboutX)
{
    Mat44 const rotationMatrix = MakeXRotationDegrees(degreesRotationAboutX);
    Append(rotationMatrix);
}

//----------------------------------------------------------------------------------------------------
void Mat44::AppendTranslation2D(Vec2 const& translationXY)
{
    Mat44 const translationMatrix = MakeTranslation2D(translationXY);
    Append(translationMatrix);
}

//----------------------------------------------------------------------------------------------------
void Mat44::AppendTranslation3D(Vec3 const& translationXYZ)
{
    Mat44 const translationMatrix = MakeTranslation3D(translationXYZ);
    Append(translationMatrix);
}

//----------------------------------------------------------------------------------------------------
void Mat44::AppendScaleUniform2D(float const uniformScaleXY)
{
    Mat44 const scaleMatrix = MakeUniformScale2D(uniformScaleXY);
    Append(scaleMatrix);
}

//----------------------------------------------------------------------------------------------------
void Mat44::AppendScaleUniform3D(float const uniformScaleXYZ)
{
    Mat44 const scaleMatrix = MakeUniformScale3D(uniformScaleXYZ);
    Append(scaleMatrix);
}

//----------------------------------------------------------------------------------------------------
void Mat44::AppendScaleNonUniform2D(Vec2 const& nonUniformScaleXY)
{
    Mat44 const scaleMatrix = MakeNonUniformScale2D(nonUniformScaleXY);
    Append(scaleMatrix);
}

//----------------------------------------------------------------------------------------------------
void Mat44::AppendScaleNonUniform3D(Vec3 const& nonUniformScaleXYZ)
{
    Mat44 const scaleMatrix = MakeNonUniformScale3D(nonUniformScaleXYZ);
    Append(scaleMatrix);
}
