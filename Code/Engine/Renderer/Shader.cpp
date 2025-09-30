//----------------------------------------------------------------------------------------------------
// Shader.cpp
//----------------------------------------------------------------------------------------------------
#include "Engine/Renderer/Shader.hpp"

#include <d3d11.h>

#include "Engine/Renderer/Renderer.hpp"

//----------------------------------------------------------------------------------------------------
Shader::Shader(sShaderConfig config)
    : m_config(std::move(config))
{
}

//----------------------------------------------------------------------------------------------------
Shader::~Shader()
{
    DX_SAFE_RELEASE(m_vertexShader)
    DX_SAFE_RELEASE(m_pixelShader)
    DX_SAFE_RELEASE(m_inputLayout)
}

//----------------------------------------------------------------------------------------------------
String const& Shader::GetName() const
{
    return m_config.m_name;
}
