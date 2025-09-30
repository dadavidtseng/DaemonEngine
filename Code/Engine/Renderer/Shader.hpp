//----------------------------------------------------------------------------------------------------
// Shader.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once

#include "Engine/Core/StringUtils.hpp"

//----------------------------------------------------------------------------------------------------
struct ID3D11VertexShader;
struct ID3D11PixelShader;
struct ID3D11InputLayout;

//----------------------------------------------------------------------------------------------------
struct sShaderConfig
{
    String m_name;
    String m_vertexEntryPoint = "VertexMain";
    String m_pixelEntryPoint  = "PixelMain";
};

//----------------------------------------------------------------------------------------------------
class Shader
{
    friend class Renderer;
    friend class ShaderLoader; // Allow ShaderLoader to access protected members

public:
    explicit Shader(sShaderConfig config);
    Shader(Shader& copy) = delete;
    ~Shader();

    String const& GetName() const;

private:
    sShaderConfig m_config;
    /// A vertex-shader interface manages an executable program (a vertex shader) that controls the vertex-shader stage.
    ID3D11VertexShader* m_vertexShader = nullptr;
    /// A pixel-shader interface manages an executable program (a pixel shader) that controls the pixel-shader stage.
    ID3D11PixelShader* m_pixelShader = nullptr;
    /// An input-layout interface holds a definition of how to feed vertex data that is laid out in memory into the input-assembler stage of the graphics pipeline.
    ID3D11InputLayout* m_inputLayout = nullptr;
};
