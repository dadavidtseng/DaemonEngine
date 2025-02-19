//----------------------------------------------------------------------------------------------------
// Mat44.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Math/Mat44.hpp"

#include "Engine/Core/EngineCommon.hpp"
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
Mat44::Mat44(Vec2 const& iBasis2D,
             Vec2 const& jBasis2D,
             Vec2 const& translation2D)
{
    m_values[Ix] = iBasis2D.x;
    m_values[Iy] = iBasis2D.y;
    m_values[Iz] = 0.f;
    m_values[Iw] = 0.f;

    m_values[Jx] = jBasis2D.x;
    m_values[Jy] = jBasis2D.y;
    m_values[Jz] = 0.f;
    m_values[Jw] = 0.f;

    m_values[Kx] = 0.f;
    m_values[Ky] = 0.f;
    m_values[Kz] = 1.f;
    m_values[Kw] = 0.f;

    m_values[Tx] = translation2D.x;
    m_values[Ty] = translation2D.y;
    m_values[Tz] = 0.f;
    m_values[Tw] = 1.f;
}

//----------------------------------------------------------------------------------------------------
Mat44::Mat44(Vec3 const& iBasis3D,
             Vec3 const& jBasis3D,
             Vec3 const& kBasis3D,
             Vec3 const& translation3D)
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
Mat44::Mat44(Vec4 const& iBasis4D,
             Vec4 const& jBasis4D,
             Vec4 const& kBasis4D,
             Vec4 const& translation4D)
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
    Mat44 result;

    result.m_values[Tx] = translationXY.x;
    result.m_values[Ty] = translationXY.y;

    return result;
}

//----------------------------------------------------------------------------------------------------
Mat44 const Mat44::MakeTranslation3D(Vec3 const& translationXYZ)
{
    Mat44 result;

    result.m_values[Tx] = translationXYZ.x;
    result.m_values[Ty] = translationXYZ.y;
    result.m_values[Tz] = translationXYZ.z;

    return result;
}

//----------------------------------------------------------------------------------------------------
Mat44 const Mat44::MakeUniformScale2D(float const uniformScaleXY)
{
    Mat44 result;

    result.m_values[Ix] = uniformScaleXY;
    result.m_values[Jy] = uniformScaleXY;

    return result;
}

//----------------------------------------------------------------------------------------------------
Mat44 const Mat44::MakeUniformScale3D(float const uniformScaleXYZ)
{
    Mat44 result;

    result.m_values[Ix] = uniformScaleXYZ;
    result.m_values[Jy] = uniformScaleXYZ;
    result.m_values[Kz] = uniformScaleXYZ;

    return result;
}

//----------------------------------------------------------------------------------------------------
Mat44 const Mat44::MakeNonUniformScale2D(Vec2 const& nonUniformScaleXY)
{
    Mat44 result;

    result.m_values[Ix] = nonUniformScaleXY.x;
    result.m_values[Jy] = nonUniformScaleXY.y;

    return result;
}

//----------------------------------------------------------------------------------------------------
Mat44 const Mat44::MakeNonUniformScale3D(Vec3 const& nonUniformScaleXYZ)
{
    Mat44 result;

    result.m_values[Ix] = nonUniformScaleXYZ.x;
    result.m_values[Jy] = nonUniformScaleXYZ.y;
    result.m_values[Kz] = nonUniformScaleXYZ.z;

    return result;
}

//----------------------------------------------------------------------------------------------------
Mat44 const Mat44::MakeZRotationDegrees(float const rotationDegreesAboutZ)
{
    float const cosTheta = CosDegrees(rotationDegreesAboutZ);
    float const sinTheta = SinDegrees(rotationDegreesAboutZ);

    Mat44 result;

    result.m_values[Ix] = cosTheta;
    result.m_values[Iy] = sinTheta;
    result.m_values[Jx] = -sinTheta;
    result.m_values[Jy] = cosTheta;

    return result;
}

//----------------------------------------------------------------------------------------------------
Mat44 const Mat44::MakeYRotationDegrees(float const rotationDegreesAboutY)
{
    float const cosTheta = CosDegrees(rotationDegreesAboutY);
    float const sinTheta = SinDegrees(rotationDegreesAboutY);

    Mat44 result;

    result.m_values[Ix] = cosTheta;
    result.m_values[Iz] = -sinTheta;
    result.m_values[Kx] = sinTheta;
    result.m_values[Kz] = cosTheta;

    return result;
}

//----------------------------------------------------------------------------------------------------
Mat44 const Mat44::MakeXRotationDegrees(float const rotationDegreesAboutX)
{
    float const cosTheta = CosDegrees(rotationDegreesAboutX);
    float const sinTheta = SinDegrees(rotationDegreesAboutX);

    Mat44 result;

    result.m_values[Jy] = cosTheta;
    result.m_values[Jz] = sinTheta;
    result.m_values[Ky] = -sinTheta;
    result.m_values[Kz] = cosTheta;

    return result;
}

//----------------------------------------------------------------------------------------------------
Mat44 const Mat44::MakeOrthoProjection(float const left,
                                       float const right,
                                       float const bottom,
                                       float const top,
                                       float const zNear,
                                       float const zFar)
{
    Mat44 result;

    result.m_values[Ix] = 2.f / (right - left);
    result.m_values[Jy] = 2.f / (top - bottom);
    result.m_values[Kz] = 1.f / (zFar - zNear);

    result.m_values[Tx] = -(right + left) / (right - left);
    result.m_values[Ty] = -(top + bottom) / (top - bottom);
    result.m_values[Tz] = -zNear / (zFar - zNear);

    result.m_values[Tw] = 1.f;

    return result;
}

//----------------------------------------------------------------------------------------------------
Mat44 const Mat44::MakePerspectiveProjection(float const fovYDegrees,
                                             float const aspect,
                                             float const zNear,
                                             float const zFar)
{
    Mat44 perspective;

    float const c          = CosDegrees(fovYDegrees * 0.5f);
    float const s          = SinDegrees(fovYDegrees * 0.5f);
    float const scaleY     = c / s;
    float const scaleX     = scaleY / aspect;
    float const scaleZ     = zFar / (zFar - zNear);
    float const translateZ = (zNear * zFar) / (zNear - zFar);

    perspective.m_values[Ix] = scaleX;
    perspective.m_values[Jy] = scaleY;
    perspective.m_values[Kz] = scaleZ;
    perspective.m_values[Kw] = 1.f;
    perspective.m_values[Tz] = translateZ;
    perspective.m_values[Tw] = 0.f;

    return perspective;

    /*Mat44 result;

    float const fovYRadians = ConvertDegreesToRadians(fovYDegrees);
    float const tanHalfFovY = tanf(fovYRadians / 2.f);

    result.m_values[Ix] = 1.f / (aspect * tanHalfFovY);
    result.m_values[Iy] = 0.f;
    result.m_values[Iz] = 0.f;
    result.m_values[Iw] = 0.f;

    result.m_values[Jx] = 0.f;
    result.m_values[Jy] = 1.f / tanHalfFovY;
    result.m_values[Jz] = 0.f;
    result.m_values[Jw] = 0.f;

    result.m_values[Kx] = 0.f;
    result.m_values[Ky] = 0.f;
    result.m_values[Kz] = zFar / (zFar - zNear);
    result.m_values[Kw] = 1.f;

    result.m_values[Tx] = 0.f;
    result.m_values[Ty] = 0.f;
    result.m_values[Tz] = -(zFar * zNear) / (zFar - zNear);
    result.m_values[Tw] = 0.f;

    return result;*/
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
    Mat44 result;

    // Transpose the rotation part (i.e., the 3x3 top-left submatrix)
    result.m_values[Ix] = m_values[Ix];
    result.m_values[Jx] = m_values[Iy];
    result.m_values[Kx] = m_values[Iz];
    result.m_values[Tx] = 0.f;  // We'll handle translation below

    result.m_values[Iy] = m_values[Jx];
    result.m_values[Jy] = m_values[Jy];
    result.m_values[Ky] = m_values[Jz];
    result.m_values[Ty] = 0.f;

    result.m_values[Iz] = m_values[Kx];
    result.m_values[Jz] = m_values[Ky];
    result.m_values[Kz] = m_values[Kz];
    result.m_values[Tz] = 0.f;

    // Negate and apply rotation to translation
    result.m_values[Tx] = -(result.m_values[Ix] * m_values[Tx] + result.m_values[Jx] * m_values[Ty] + result.m_values[Kx] * m_values[Tz]);
    result.m_values[Ty] = -(result.m_values[Iy] * m_values[Tx] + result.m_values[Jy] * m_values[Ty] + result.m_values[Ky] * m_values[Tz]);
    result.m_values[Tz] = -(result.m_values[Iz] * m_values[Tx] + result.m_values[Jz] * m_values[Ty] + result.m_values[Kz] * m_values[Tz]);

    //  bottom row for an affine transformation is always [0, 0, 0, 1]
    result.m_values[Tw] = 1.f;

    return result;
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
void Mat44::SetIJ2D(Vec2 const& iBasis2D,
                    Vec2 const& jBasis2D)
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
void Mat44::SetIJT2D(Vec2 const& iBasis2D,
                     Vec2 const& jBasis2D,
                     Vec2 const& translationXY)
{
    SetIJ2D(iBasis2D, jBasis2D);
    SetTranslation2D(translationXY);
}

//----------------------------------------------------------------------------------------------------
void Mat44::SetIJK3D(Vec3 const& iBasis3D,
                     Vec3 const& jBasis3D,
                     Vec3 const& kBasis3D)
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
void Mat44::SetIJKT3D(Vec3 const& iBasis3D,
                      Vec3 const& jBasis3D,
                      Vec3 const& kBasis3D,
                      Vec3 const& translationXYZ)
{
    SetIJK3D(iBasis3D, jBasis3D, kBasis3D);
    SetTranslation3D(translationXYZ);
}

//----------------------------------------------------------------------------------------------------
void Mat44::SetIJKT4D(Vec4 const& iBasis4D,
                      Vec4 const& jBasis4D,
                      Vec4 const& kBasis4D,
                      Vec4 const& translation4D)
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
void Mat44::Transpose()
{
    float temp   = m_values[Iy];
    m_values[Iy] = m_values[Jx];
    m_values[Jx] = temp;

    temp         = m_values[Iz];
    m_values[Iz] = m_values[Kx];
    m_values[Kx] = temp;

    temp         = m_values[Iw];
    m_values[Iw] = m_values[Tx];
    m_values[Tx] = temp;

    temp         = m_values[Jz];
    m_values[Jz] = m_values[Ky];
    m_values[Ky] = temp;

    temp         = m_values[Jw];
    m_values[Jw] = m_values[Ty];
    m_values[Ty] = temp;

    temp         = m_values[Kw];
    m_values[Kw] = m_values[Tz];
    m_values[Tz] = temp;
}

//----------------------------------------------------------------------------------------------------
void Mat44::Orthonormalize_IFwd_JLeft_KUp()
{
    // Assuming m_values is a 1D array representing the 4x4 matrix in row-major order
    Vec3 i(m_values[Ix], m_values[Iy], m_values[Iz]);
    Vec3 j(m_values[Jx], m_values[Jy], m_values[Jz]);
    Vec3 k(m_values[Kx], m_values[Ky], m_values[Kz]);

    // Normalize i
    i = i.GetNormalized();

    // Make j orthogonal to i
    j -= DotProduct3D(j, i) * i;
    j = j.GetNormalized();

    // Make k orthogonal to both i and j
    k -= DotProduct3D(k, i) * i;
    k -= DotProduct3D(k, j) * j;
    k = k.GetNormalized();

    // Update the matrix with the orthonormalized vectors
    m_values[Ix] = i.x;
    m_values[Iy] = i.y;
    m_values[Iz] = i.z;
    m_values[Jx] = j.x;
    m_values[Jy] = j.y;
    m_values[Jz] = j.z;
    m_values[Kx] = k.x;
    m_values[Ky] = k.y;
    m_values[Kz] = k.z;
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
