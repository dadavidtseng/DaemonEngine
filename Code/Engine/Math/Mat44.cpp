//----------------------------------------------------------------------------------------------------
// Mat44.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Math/Mat44.hpp"
#include <cmath>
#include <cstdio>

#include "MathUtils.hpp"

//----------------------------------------------------------------------------------------------------
Mat44::Mat44()
{
    for (int i = 0; i < 16; ++i)
    {
        m_values[i] = (i % 5 == 0) ? 1.0f : 0.0f;
    }
}

//----------------------------------------------------------------------------------------------------
Mat44::Mat44(Vec2 const& iBasis2D, Vec2 const& jBasis2D, Vec2 const& translation2D)
{
    // Initialize the matrix with the 2D basis vectors and translation in column-major order
    m_values[0]  = iBasis2D.x; // First column, first row (Ix)
    m_values[4]  = iBasis2D.y; // First column, second row (Iy)
    m_values[8]  = 0.0f;       // First column, third row (Iz, 2D)
    m_values[12] = 0.0f;       // First column, fourth row (Iw)

    m_values[1]  = jBasis2D.x; // Second column, first row (Jx)
    m_values[5]  = jBasis2D.y; // Second column, second row (Jy)
    m_values[9]  = 0.0f;       // Second column, third row (Jz, 2D)
    m_values[13] = 0.0f;       // Second column, fourth row (Jw)

    m_values[2]  = 0.0f;       // Third column, first row (Kx, 2D)
    m_values[6]  = 0.0f;       // Third column, second row (Ky, 2D)
    m_values[10] = 1.0f;       // Third column, third row (Kz, identity for 2D)
    m_values[14] = 0.0f;       // Third column, fourth row (Kw)

    m_values[3]  = translation2D.x; // Fourth column, first row (Tx)
    m_values[7]  = translation2D.y; // Fourth column, second row (Ty)
    m_values[11] = 0.0f;            // Fourth column, third row (Tz, 2D)
    m_values[15] = 1.0f;            // Fourth column, fourth row (Tw, homogeneous coordinate)
}

//----------------------------------------------------------------------------------------------------
Mat44::Mat44(Vec3 const& iBasis3D, Vec3 const& jBasis3D, Vec3 const& kBasis3D, Vec3 const& translation3D)
{
    m_values[0] = iBasis3D.x;  // First row, first column
    m_values[4] = iBasis3D.y;  // First row, second column
    m_values[8] = iBasis3D.z;  // First row, third column
    m_values[12] = 0.0f;        // First row, fourth column

    m_values[1] = jBasis3D.x;  // Second row, first column
    m_values[5] = jBasis3D.y;  // Second row, second column
    m_values[9] = jBasis3D.z;  // Second row, third column
    m_values[13] = 0.0f;        // Second row, fourth column

    m_values[2]  = kBasis3D.x; // Third row, first column
    m_values[6]  = kBasis3D.y; // Third row, second column
    m_values[10] = kBasis3D.z; // Third row, third column
    m_values[14] = 0.0f;       // Third row, fourth column

    m_values[3] = translation3D.x; // Fourth row, first column (translation x)
    m_values[7] = translation3D.y; // Fourth row, second column (translation y)
    m_values[11] = translation3D.z; // Fourth row, third column (translation z)
    m_values[15] = 1.0f;            // Fourth row, fourth column (homogeneous coordinate)
}
Mat44::Mat44(Vec4 const& iBasis4D, Vec4 const& jBasis4D, Vec4 const& kBasis4D, Vec4 const& translation4D)
{
    m_values[0] = iBasis4D.x;  // First row, first column
    m_values[4] = iBasis4D.y;  // First row, second column
    m_values[8] = iBasis4D.z;  // First row, third column
    m_values[12] = iBasis4D.w;  // First row, fourth column

    m_values[1] = jBasis4D.x;  // Second row, first column
    m_values[5] = jBasis4D.y;  // Second row, second column
    m_values[9] = jBasis4D.z;  // Second row, third column
    m_values[13] = jBasis4D.w;  // Second row, fourth column

    m_values[2]  = kBasis4D.x; // Third row, first column
    m_values[6]  = kBasis4D.y; // Third row, second column
    m_values[10] = kBasis4D.z; // Third row, third column
    m_values[14] = kBasis4D.w; // Third row, fourth column

    m_values[3] = translation4D.x; // Fourth row, first column
    m_values[7] = translation4D.y; // Fourth row, second column
    m_values[11] = translation4D.z; // Fourth row, third column
    m_values[15] = translation4D.w; // Fourth row, fourth column
}
Mat44::Mat44(float const* sixteenValuesBasisMajor)
{
    for (int i = 0; i < 16; ++i)
    {
        m_values[i] = sixteenValuesBasisMajor[i];
    }
}
Mat44 const Mat44::MakeTranslation2D(Vec2 const& translationXY)
{
    // Create a 4x4 identity matrix and set the translation components
    float values[16] = {
        1.0f, 0.0f, 0.0f, 0.0f,  // First row
        0.0f, 1.0f, 0.0f, 0.0f,  // Second row
        0.0f, 0.0f, 1.0f, 0.0f,  // Third row
        translationXY.x, translationXY.y, 0.0f, 1.0f  // Fourth row
    };

    return Mat44(values);
}
Mat44 const Mat44::MakeTranslation3D(Vec3 const& translationXYZ)
{
    // Create a 4x4 identity matrix and set the translation components
    float values[16] = {
        1.0f, 0.0f, 0.0f, 0.0f,  // First row
        0.0f, 1.0f, 0.0f, 0.0f,  // Second row
        0.0f, 0.0f, 1.0f, 0.0f,  // Third row
        translationXYZ.x, translationXYZ.y, translationXYZ.z, 1.0f  // Fourth row
    };

    return Mat44(values);
}
Mat44 const Mat44::MakeUniformScale2D(float uniformScaleXY)
{
    float values[16] = {
        uniformScaleXY, 0.0f, 0.0f, 0.0f,  // First row
        0.0f, uniformScaleXY, 0.0f, 0.0f,  // Second row
        0.0f, 0.0f, 1.0f, 0.0f,           // Third row (identity for z-axis)
        0.0f, 0.0f, 0.0f, 1.0f            // Fourth row (homogeneous coordinate)
    };

    return Mat44(values);
}
Mat44 const Mat44::MakeUniformScale3D(float uniformScaleXYZ)
{
    float values[16] = {
        uniformScaleXYZ, 0.0f, 0.0f, 0.0f,  // First row
        0.0f, uniformScaleXYZ, 0.0f, 0.0f,  // Second row
        0.0f, 0.0f, uniformScaleXYZ, 0.0f,  // Third row
        0.0f, 0.0f, 0.0f, 1.0f             // Fourth row (homogeneous coordinate)
    };

    return Mat44(values);
}
Mat44 const Mat44::MakeNonUniformScale2D(Vec2 const& nonUniformScaleXY)
{
    float values[16] = {
        nonUniformScaleXY.x, 0.0f, 0.0f, 0.0f,  // First row
        0.0f, nonUniformScaleXY.y, 0.0f, 0.0f,  // Second row
        0.0f, 0.0f, 1.0f, 0.0f,                // Third row (identity for z-axis)
        0.0f, 0.0f, 0.0f, 1.0f                 // Fourth row (homogeneous coordinate)
    };

    return Mat44(values);
}
Mat44 const Mat44::MakeNonUniformScale3D(Vec3 const& nonUniformScaleXYZ)
{
    float values[16] = {
        nonUniformScaleXYZ.x, 0.0f, 0.0f, 0.0f,  // First row
        0.0f, nonUniformScaleXYZ.y, 0.0f, 0.0f,  // Second row
        0.0f, 0.0f, nonUniformScaleXYZ.z, 0.0f,  // Third row
        0.0f, 0.0f, 0.0f, 1.0f                  // Fourth row (homogeneous coordinate)
    };

    return Mat44(values);
}
Mat44 const Mat44::MakeZRotationDegrees(float rotationDegreesAboutZ)
{
    float cosTheta = CosDegrees(rotationDegreesAboutZ);
    float sinTheta = SinDegrees(rotationDegreesAboutZ);

    float values[16] = {
        cosTheta, sinTheta, 0.0f, 0.0f,  // First row
        -sinTheta, cosTheta, 0.0f, 0.0f,  // Second row
        0.0f, 0.0f, 1.0f, 0.0f,  // Third row
        0.0f, 0.0f, 0.0f, 1.0f   // Fourth row (homogeneous coordinate)
    };
    return Mat44(values);
}
Mat44 const Mat44::MakeYRotationDegrees(float rotationDegreesAboutY)
{
    float cosTheta = CosDegrees(rotationDegreesAboutY);
    float sinTheta = SinDegrees(rotationDegreesAboutY);

    float values[16] = {
        cosTheta, 0.0f, -sinTheta, 0.0f,  // First row
        0.0f, 1.0f, 0.0f, 0.0f,  // Second row
        sinTheta, 0.0f, cosTheta, 0.0f,  // Third row
        0.0f, 0.0f, 0.0f, 1.0f   // Fourth row (homogeneous coordinate)
    };

    return Mat44(values);
}
Mat44 const Mat44::MakeXRotationDegrees(float rotationDegreesAboutX)
{
    float cosTheta = CosDegrees(rotationDegreesAboutX);
    float sinTheta = SinDegrees(rotationDegreesAboutX);

    float values[16] = {
        1.0f, 0.0f, 0.0f, 0.0f,  // First row
        0.0f, cosTheta, sinTheta, 0.0f,  // Second row
        0.0f, -sinTheta, cosTheta, 0.0f,  // Third row
        0.0f, 0.0f, 0.0f, 1.0f   // Fourth row (homogeneous coordinate)
    };

    return Mat44(values);
}
Vec2 const Mat44::TransformVectorQuantity2D(Vec2 const& vectorQuantityXY) const
{
    float x = m_values[0] * vectorQuantityXY.x + m_values[4] * vectorQuantityXY.y;
    float y = m_values[1] * vectorQuantityXY.x + m_values[5] * vectorQuantityXY.y;
    return Vec2(x, y);
}
Vec3 const Mat44::TransformVectorQuantity3D(Vec3 const& vectorQuantityXYZ) const
{
    float x = m_values[0] * vectorQuantityXYZ.x + m_values[4] * vectorQuantityXYZ.y + m_values[8] * vectorQuantityXYZ.z;
    float y = m_values[1] * vectorQuantityXYZ.x + m_values[5] * vectorQuantityXYZ.y + m_values[9] * vectorQuantityXYZ.z;
    float z = m_values[2] * vectorQuantityXYZ.x + m_values[6] * vectorQuantityXYZ.y + m_values[10] * vectorQuantityXYZ.z;
    return Vec3(x, y, z);
}
Vec2 const Mat44::TransformPosition2D(Vec2 const& positionXY) const
{
    float x = m_values[0] * positionXY.x + m_values[4] * positionXY.y + m_values[12];
    float y = m_values[1] * positionXY.x + m_values[5] * positionXY.y + m_values[13];
    return Vec2(x, y);
}
Vec3 const Mat44::TransformPosition3D(Vec3 const& position3D) const
{
    float x = m_values[0] * position3D.x + m_values[4] * position3D.y + m_values[8] * position3D.z + m_values[12];
    float y = m_values[1] * position3D.x + m_values[5] * position3D.y + m_values[9] * position3D.z + m_values[13];
    float z = m_values[2] * position3D.x + m_values[6] * position3D.y + m_values[10] * position3D.z + m_values[14];
    return Vec3(x, y, z);
}
Vec4 const Mat44::TransformHomogeneous3D(Vec4 const& homogeneousPoint3D) const
{
    float x = m_values[0] * homogeneousPoint3D.x + m_values[4] * homogeneousPoint3D.y + m_values[8] * homogeneousPoint3D.z + m_values[12] * homogeneousPoint3D.w;
    float y = m_values[1] * homogeneousPoint3D.x + m_values[5] * homogeneousPoint3D.y + m_values[9] * homogeneousPoint3D.z + m_values[13] * homogeneousPoint3D.w;
    float z = m_values[2] * homogeneousPoint3D.x + m_values[6] * homogeneousPoint3D.y + m_values[10] * homogeneousPoint3D.z + m_values[14] * homogeneousPoint3D.w;
    float w = m_values[3] * homogeneousPoint3D.x + m_values[7] * homogeneousPoint3D.y + m_values[11] * homogeneousPoint3D.z + m_values[15] * homogeneousPoint3D.w;
    return Vec4(x, y, z, w);
}
float* Mat44::GetAsFloatArray()
{
    return m_values;
}
float const* Mat44::GetAsFloatArray() const
{
    return m_values;
}
Vec2 const Mat44::GetIBasis2D() const
{
    return Vec2(m_values[0], m_values[1]);
}
Vec2 const Mat44::GetJBasis2D() const
{
    return Vec2(m_values[4], m_values[5]);
}
Vec2 const Mat44::GetTranslation2D() const
{
    return Vec2(m_values[12], m_values[13]);
}
Vec3 const Mat44::GetIBasis3D() const
{
    return Vec3(m_values[0], m_values[1], m_values[2]);
}

Vec3 const Mat44::GetJBasis3D() const
{
    return Vec3(m_values[4], m_values[5], m_values[6]);
}

Vec3 const Mat44::GetKBasis3D() const
{
    return Vec3(m_values[8], m_values[9], m_values[10]);
}
Vec3 const Mat44::GetTranslation3D() const
{
    return Vec3(m_values[12], m_values[13], m_values[14]);
}
Vec4 const Mat44::GetIBasis4D() const
{
    return Vec4(m_values[0], m_values[1], m_values[2], m_values[3]);

}
Vec4 const Mat44::GetJBasis4D() const
{
    return Vec4(m_values[4], m_values[5], m_values[6], m_values[7]);

}
Vec4 const Mat44::GetKBasis4D() const
{
    return Vec4(m_values[8], m_values[9], m_values[10], m_values[11]);

}
Vec4 const Mat44::GetTranslation4D() const
{
    return Vec4(m_values[12], m_values[13], m_values[14], m_values[15]);

}
void Mat44::SetTranslation2D(Vec2 const& translationXY)
{
    m_values[12] = translationXY.x;
    m_values[13] = translationXY.y;
    m_values[14] = 0.0f;  // Typically, the z-component is 0 for 2D
    m_values[15] = 1.0f;  // Homogeneous coordinate
}
void Mat44::SetTranslation3D(Vec3 const& translationXYZ)
{
    m_values[12] = translationXYZ.x;
    m_values[13] = translationXYZ.y;
    m_values[14] = translationXYZ.z;
    m_values[15] = 1.0f;  // Homogeneous coordinate
}
void Mat44::SetIJ2D(Vec2 const& iBasis2D, Vec2 const& jBasis2D)
{
    m_values[0] = iBasis2D.x;
    m_values[1] = iBasis2D.y;
    m_values[2] = 0.0f;  // Typically, the z-component is 0 for 2D
    m_values[3] = 0.0f;

    m_values[4] = jBasis2D.x;
    m_values[5] = jBasis2D.y;
    m_values[6] = 0.0f;  // Typically, the z-component is 0 for 2D
    m_values[7] = 0.0f;
}
void Mat44::SetIJT2D(Vec2 const& iBasis2D, Vec2 const& jBasis2D, Vec2 const& translationXY)
{
    SetIJ2D(iBasis2D, jBasis2D);
    SetTranslation2D(translationXY);
}
void Mat44::SetIJK3D(Vec3 const& iBasis3D, Vec3 const& jBasis3D, Vec3 const& kBasis3D)
{
    m_values[0] = iBasis3D.x;
    m_values[1] = iBasis3D.y;
    m_values[2] = iBasis3D.z;
    m_values[3] = 0.0f;

    m_values[4] = jBasis3D.x;
    m_values[5] = jBasis3D.y;
    m_values[6] = jBasis3D.z;
    m_values[7] = 0.0f;

    m_values[8]  = kBasis3D.x;
    m_values[9]  = kBasis3D.y;
    m_values[10] = kBasis3D.z;
    m_values[11] = 0.0f;
}
void Mat44::SetIJKT3D(Vec3 const& iBasis3D, Vec3 const& jBasis3D, Vec3 const& kBasis3D, Vec3 const& translationXYZ)
{
    SetIJK3D(iBasis3D, jBasis3D, kBasis3D);
    SetTranslation3D(translationXYZ);
}
void Mat44::SetIJKT4D(Vec4 const& iBasis4D, Vec4 const& jBasis4D, Vec4 const& kBasis4D, Vec4 const& translation4D)
{
    m_values[0] = iBasis4D.x;
    m_values[1] = iBasis4D.y;
    m_values[2] = iBasis4D.z;
    m_values[3] = iBasis4D.w;

    m_values[4] = jBasis4D.x;
    m_values[5] = jBasis4D.y;
    m_values[6] = jBasis4D.z;
    m_values[7] = jBasis4D.w;

    m_values[8]  = kBasis4D.x;
    m_values[9]  = kBasis4D.y;
    m_values[10] = kBasis4D.z;
    m_values[11] = kBasis4D.w;

    m_values[12] = translation4D.x;
    m_values[13] = translation4D.y;
    m_values[14] = translation4D.z;
    m_values[15] = translation4D.w;
}
void Mat44::Append(Mat44 const& appendThis)
{
    // Temporary matrix to store the result
    float result[16] = { 0.0f };

    // Perform the matrix multiplication assuming column-major storage
    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            result[i * 4 + j] = 0.0f;
            for (int k = 0; k < 4; ++k)
            {
                result[i * 4 + j] += m_values[k * 4 + i] * appendThis.m_values[i * 4 + j];
            }
        }
    }

    // Safely copy the result back to m_values
    for (int i = 0; i < 16; ++i)
    {
        m_values[i] = result[i];
    }
}

void Mat44::AppendZRotation(float degreesRotationAboutZ)
{
    Mat44 rotationMatrix = MakeZRotationDegrees(degreesRotationAboutZ);
    Append(rotationMatrix);
}
void Mat44::AppendYRotation(float degreesRotationAboutY)
{
    Mat44 rotationMatrix = MakeYRotationDegrees(degreesRotationAboutY);
    Append(rotationMatrix);
}
void Mat44::AppendXRotation(float degreesRotationAboutX)
{
    Mat44 rotationMatrix = Mat44::MakeXRotationDegrees(degreesRotationAboutX);
    Append(rotationMatrix);
}
void Mat44::AppendTranslation2D(Vec2 const& translationXY)
{
    // Extract translation components
    float tx = translationXY.x;
    float ty = translationXY.y;

    // Adjust the existing translation using the current basis vectors
    m_values[12] += tx * m_values[0] + ty * m_values[4];
    m_values[13] += tx * m_values[1] + ty * m_values[5];
    m_values[14] += tx * m_values[2] + ty * m_values[6];
    m_values[15] += tx * m_values[3] + ty * m_values[7];
}
void Mat44::AppendTranslation3D(Vec3 const& translationXYZ)
{
    Mat44 translationMatrix = MakeTranslation3D(translationXYZ);
    Append(translationMatrix);
}
void Mat44::AppendScaleUniform2D(float uniformScaleXY)
{
    Mat44 scaleMatrix = Mat44::MakeUniformScale2D(uniformScaleXY);
    Append(scaleMatrix);
}

void Mat44::AppendScaleUniform3D(float uniformScaleXYZ)
{
    Mat44 scaleMatrix = Mat44::MakeUniformScale3D(uniformScaleXYZ);
    Append(scaleMatrix);
}
void Mat44::AppendScaleNonUniform2D(Vec2 const& nonUniformScaleXY)
{
    Mat44 scaleMatrix = Mat44::MakeNonUniformScale2D(nonUniformScaleXY);
    Append(scaleMatrix);
}
void Mat44::AppendScaleNonUniform3D(Vec3 const& nonUniformScaleXYZ)
{
    Mat44 scaleMatrix = Mat44::MakeNonUniformScale3D(nonUniformScaleXYZ);
    Append(scaleMatrix);
}
