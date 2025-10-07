//----------------------------------------------------------------------------------------------------
// ShaderResource.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include "Engine/Resource/IResource.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Renderer/RenderCommon.hpp"

//----------------------------------------------------------------------------------------------------
class Shader;

//----------------------------------------------------------------------------------------------------
class ShaderResource : public IResource
{
public:
    ShaderResource(String const& path, eResourceType type);
    ~ShaderResource() override;

    // IResource interface implementation
    bool   Load() override;
    void   Unload() override;
    size_t CalculateMemorySize() const override;

    // Shader-specific interface
    String GetName() const { return m_name; }

    // Renderer::Shader access (for Renderer integration)
    Shader* GetRendererShader() const { return m_rendererShader; }

    // Set vertex type for shader compilation
    void SetVertexType(eVertexType vertexType) { m_vertexType = vertexType; }
    eVertexType GetVertexType() const { return m_vertexType; }

private:
    friend class ShaderLoader;
    friend class Renderer; // Allow Renderer to access resources

    // Shader properties
    String m_name;
    eVertexType m_vertexType = eVertexType::VERTEX_PCU;

    // Wrapped Renderer::Shader
    Shader* m_rendererShader = nullptr;

    // Resource creation methods (called by ShaderLoader)
    void SetRendererShader(Shader* shader);
    void SetName(String const& name) { m_name = name; }
};