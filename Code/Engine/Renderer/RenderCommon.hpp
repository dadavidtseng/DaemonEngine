//----------------------------------------------------------------------------------------------------
// RenderCommon.hpp
//----------------------------------------------------------------------------------------------------

#pragma once
#include <cstdint>

#include "Light.hpp"
#include "Engine/Math/Mat44.hpp"

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
    float  CameraWorldPosition[3];
    float _padding;
};

int constexpr MAX_LIGHTS = 8;

//----------------------------------------------------------------------------------------------------
struct sLightConstants
{
    float        LightColor[4];
    float        SunDirection[3];
    float        AmbientIntensity;
    int          NumLights;
    float        padding[3];
    GPULightData lightArray[MAX_LIGHTS];
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
