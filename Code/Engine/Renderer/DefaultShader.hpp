//----------------------------------------------------------------------------------------------------
// DefaultShader.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once

//----------------------------------------------------------------------------------------------------
const char* const DEFAULT_SHADER_SOURCE = R"(
struct vs_input_t
{
    float3 modelSpacePosition : POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
};

struct v2p_t
{
    float4 clipSpacePosition : SV_Position;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
};

cbuffer ModelConstants: register(b3)
{
    float4x4 ModelToWorldTransform;
    float4 ModelColor;
}

cbuffer CameraConstants : register(b2)
{
    float4x4 WorldToCameraTransform;
    float4x4 CameraToRenderTransform;
    float4x4 RenderToClipTransform;
}

v2p_t VertexMain(vs_input_t input)
{
    float4 modelSpacePosition = float4(input.modelSpacePosition, 1);
    float4 worldSpacePosition = mul(ModelToWorldTransform, modelSpacePosition);
    float4 cameraSpacePosition = mul(WorldToCameraTransform, worldSpacePosition);
    float4 renderSpacePosition = mul(CameraToRenderTransform, cameraSpacePosition);
    float4 clipSpacePosition = mul(RenderToClipTransform, renderSpacePosition);

    v2p_t v2p;
    v2p.clipSpacePosition  = clipSpacePosition;
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
    float4 color = ModelColor * textureColor * vertexColor;
    clip(color.a - 0.1f);

    return float4(color);
})";
