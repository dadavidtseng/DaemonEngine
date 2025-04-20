//----------------------------------------------------------------------------------------------------
// Shader.hpp
//----------------------------------------------------------------------------------------------------
#pragma once

#include "Engine/Core/StringUtils.hpp"

//----------------------------------------------------------------------------------------------------
struct ID3D11VertexShader;
struct ID3D11PixelShader;
struct ID3D11InputLayout;

//----------------------------------------------------------------------------------------------------
struct ShaderConfig
{
    String m_name;
    String m_vertexEntryPoint = "VertexMain";
    String m_pixelEntryPoint  = "PixelMain";
};

//----------------------------------------------------------------------------------------------------
class Shader
{
    friend class Renderer;

public:
    explicit Shader(ShaderConfig const& config);
    Shader(Shader& copy) = delete;
    ~Shader();

    String const& GetName() const;

private:
    ShaderConfig        m_config;
    ID3D11VertexShader* m_vertexShader = nullptr;
    ID3D11PixelShader*  m_pixelShader  = nullptr;
    ID3D11InputLayout*  m_inputLayout  = nullptr;
};
