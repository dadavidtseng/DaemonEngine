//----------------------------------------------------------------------------------------------------
// DefaultShader.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once

//----------------------------------------------------------------------------------------------------
char const* const DEFAULT_SHADER_SOURCE = R"(
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

inline const char* shadowDepthMapShaderSource = R"(
//------------------------------------------------------------------------------------------------
struct vs_input_t
{
	float3 localPosition : POSITION;
	float4 color : COLOR;
	float2 uv : TEXCOORD;
	float3 localTangent : TANGENT;
	float3 localBitangent : BITANGENT;
	float3 localNormal : NORMAL;
};

//------------------------------------------------------------------------------------------------
struct v2p_t
{
	float4 position : SV_Position;
};

//------------------------------------------------------------------------------------------------
cbuffer LightConstants : register(b0)
{
	float3 LightPosition;
	float ambient;
	float4x4 LightViewMatrix;
	float4x4 LightProjectionMatrix;
};

//------------------------------------------------------------------------------------------------
cbuffer ModelConstants : register(b3)
{
	float4x4 ModelMatrix;
	float4 ModelColor;
};

//------------------------------------------------------------------------------------------------
v2p_t VertexMain(vs_input_t input)
{
	float4 localPosition = float4(input.localPosition, 1);
	float4 worldPosition = mul(ModelMatrix, localPosition);
	float4 viewPosition = mul(LightViewMatrix, worldPosition);
	float4 clipPosition = mul(LightProjectionMatrix, viewPosition);

	v2p_t v2p;
	v2p.position = clipPosition;
	return v2p;
}



//------------------------------------------------------------------------------------------------
float4 PixelMain(v2p_t input) : SV_Target0
{
	float depth = input.position.z / input.position.w;
	float4 color = float4(depth, depth, depth, 1.f);
	return color;
}


)";

inline const char* blurDownShaderSource = R"(


Texture2D sourceTexture : register(t0);
SamplerState sourceSampler : register(s0);

struct vs_input_t
{
	float3 localPosition : VERTEX_POSITION;
	float4 color : VERTEX_COLOR;
	float2 uv : VERTEX_UVTEXCOORDS;
};

struct v2p_t
{
	float4 position : SV_Position;
	float2 uv : TEXCOORD;
};

struct BlurSample{
	float2 offset;
	float weight;
	float padding;
};

#define MAX_SAMPLES 64
cbuffer BlurConstants : register(b5){
	float2 texelSize;
	float lerpT;
	int numOfSamples;
	BlurSample samples[MAX_SAMPLES];
};

v2p_t VertexMain(vs_input_t input)
{
	v2p_t v2p;
	v2p.position = float4(input.localPosition, 1.f);
	v2p.uv = input.uv;
	return v2p;
}

float4 PixelMain(v2p_t input) : SV_Target0
{
	float4 color = float4(0.f, 0.f, 0.f, 1.f);
	for(int i = 0; i < numOfSamples; i++){
		color.rgb += sourceTexture.Sample(sourceSampler, input.uv + samples[i].offset * texelSize).rgb * samples[i].weight;
	}

	return color;
}

)";

inline const char* blurUpShaderSource = R"(


Texture2D downTexture : register(t0);
Texture2D upTexture : register(t1);
SamplerState sourceSampler : register(s0);

struct vs_input_t
{
	float3 localPosition : VERTEX_POSITION;
	float4 color : VERTEX_COLOR;
	float2 uv : VERTEX_UVTEXCOORDS;
};

struct v2p_t
{
	float4 position : SV_Position;
	float2 uv : TEXCOORD;
};

struct BlurSample{
	float2 offset;
	float weight;
	float padding;
};

#define MAX_SAMPLES 64
cbuffer BlurConstants : register(b5){
	float2 texelSize;
	float lerpT;
	int numOfSamples;
	BlurSample samples[MAX_SAMPLES];
};

v2p_t VertexMain(vs_input_t input)
{
	v2p_t v2p;
	v2p.position = float4(input.localPosition, 1.f);
	v2p.uv = input.uv;
	return v2p;
}

float4 PixelMain(v2p_t input) : SV_Target0
{
	float4 downColor = downTexture.Sample(sourceSampler, input.uv);
	float4 upColor = float4(0.f, 0.f, 0.f, 1.f);
	for(int i = 0; i < numOfSamples; i++){
		upColor.rgb += upTexture.Sample(sourceSampler, input.uv + samples[i].offset * texelSize).rgb * samples[i].weight;
	}
	float4 color = lerp(downColor, upColor, lerpT);

	return color;
}

)";

inline const char* blurCompositeShaderSource = R"(


Texture2D blurTexture : register(t0);
SamplerState blurSampler : register(s0);

struct vs_input_t
{
    float3 localPosition : VERTEX_POSITION;
    float4 color : VERTEX_COLOR;
    float2 uv : VERTEX_UVTEXCOORDS;
};

struct v2p_t
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD;
};

v2p_t VertexMain(vs_input_t input)
{
    v2p_t v2p;
    v2p.position = float4(input.localPosition, 1.f);
    v2p.uv = input.uv;
    return v2p;
}

float4 PixelMain(v2p_t input) : SV_Target0
{
    float4 color = blurTexture.Sample(blurSampler, input.uv);
    return color;
}

)";
