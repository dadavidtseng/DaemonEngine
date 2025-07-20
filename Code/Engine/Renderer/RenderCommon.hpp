//----------------------------------------------------------------------------------------------------
// RenderCommon.hpp
//----------------------------------------------------------------------------------------------------

#pragma once
#include <cstdint>

#include "Engine/Math/Mat44.hpp"
#include "Engine/Renderer/Light.hpp"

#define DX_SAFE_RELEASE(dxObject) \
if ((dxObject) != nullptr) {    \
dxObject->Release();      \
dxObject = nullptr;       \
}

//----------------------------------------------------------------------------------------------------
enum class eVertexType : int8_t
{
    VERTEX_PCU,
    VERTEX_PCUTBN,
    COUNT
};

//----------------------------------------------------------------------------------------------------
enum class eDepthMode : int8_t
{
    DISABLED,
    READ_ONLY_ALWAYS,
    READ_ONLY_LESS_EQUAL,
    READ_WRITE_LESS_EQUAL,
    COUNT
};

//----------------------------------------------------------------------------------------------------
enum class eSamplerMode : int8_t
{
    POINT_CLAMP,
    BILINEAR_CLAMP,
    COUNT
};

//----------------------------------------------------------------------------------------------------
enum class eRasterizerMode : int8_t
{
    SOLID_CULL_NONE,
    SOLID_CULL_BACK,
    SOLID_CULL_FRONT,
    WIREFRAME_CULL_NONE,
    WIREFRAME_CULL_BACK,
    COUNT
};

//----------------------------------------------------------------------------------------------------
struct sCameraConstants
{
    Mat44 WorldToCameraTransform;       // View transform
    Mat44 CameraToRenderTransform;      // Non-standard transform from game to DirectX conventions;
    Mat44 RenderToClipTransform;        // Projection transform
    float CameraWorldPosition[3];
    float _padding;
};

//----------------------------------------------------------------------------------------------------
/// @brief Max light numbers for light array.
int constexpr MAX_LIGHTS = 8;

//----------------------------------------------------------------------------------------------------
struct sLightConstants
{
    int   NumLights;
    float padding[3];
    Light lightArray[MAX_LIGHTS];
};

//----------------------------------------------------------------------------------------------------
struct sModelConstants
{
    Mat44 ModelToWorldTransform;
    float ModelColor[4];
};

//----------------------------------------------------------------------------------------------------
struct sPerFrameConstants
{
    float c_time;
    int   c_debugInt;
    float c_debugFloat;
    float padding;
};

struct BlurSample {
    Vec2 m_offset;
    float m_weight;
    float m_padding;
};

constexpr int k_blurMaxSamples = 64;

struct BlurConstants {
    Vec2 m_texelSize;
    float m_lerpT;
    int m_numSamples;
    BlurSample m_samples[k_blurMaxSamples];
};
