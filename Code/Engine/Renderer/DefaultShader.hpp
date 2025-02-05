//----------------------------------------------------------------------------------------------------
// DefaultShader.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once

//----------------------------------------------------------------------------------------------------
const char* const DEFAULT_SHADER_SOURCE = R"(
struct vs_input_t
{
    float3 localPosition : POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
};

struct v2p_t
{
    float4 position : SV_Position;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
};

cbuffer CameraConstants : register(b2)
{
    float OrthoMinX;
    float OrthoMinY;
    float OrthoMinZ;
    float OrthoMaxX;
    float OrthoMaxY;
    float OrthoMaxZ;
    float pad0;
    float pad1;
}

float RangeMap(float value, float inMin, float inMax, float outMin, float outMax)
{
    return outMin + (outMax - outMin) * ((value - inMin) / (inMax - inMin));
}

v2p_t VertexMain(vs_input_t input)
{
    float4 localPosition = float4(input.localPosition, 1);

    float4 clipPosition;
    clipPosition.x = RangeMap(localPosition.x, OrthoMinX, OrthoMaxX, -1.f, 1.f);
    clipPosition.y = RangeMap(localPosition.y, OrthoMinY, OrthoMaxY, -1.f, 1.f);
    clipPosition.z = RangeMap(localPosition.z, OrthoMinZ, OrthoMaxZ, 0.f, 1.f);
    clipPosition.w = localPosition.w;

    v2p_t v2p;
    v2p.position = clipPosition;
    v2p.color = input.color;
    v2p.uv = input.uv;
    return v2p;
}

Texture2D diffuseTexture : register(t0);
SamplerState diffuseSampler : register(s0);

float4 PixelMain(v2p_t input) : SV_Target0
{
    float4 textureColor = diffuseTexture.Sample(diffuseSampler, input.uv);
    float4 vertexColor = input.color;
    float4 color = textureColor * vertexColor;
    clip(color.a - 0.1f);

    return float4(color);
})";
