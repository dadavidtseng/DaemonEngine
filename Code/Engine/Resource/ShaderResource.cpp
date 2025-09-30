//----------------------------------------------------------------------------------------------------
// ShaderResource.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Resource/ShaderResource.hpp"
#include "Engine/Renderer/Shader.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"

//----------------------------------------------------------------------------------------------------
ShaderResource::ShaderResource(String const& path, ResourceType type)
    : IResource(path, type)
{
}

//----------------------------------------------------------------------------------------------------
ShaderResource::~ShaderResource()
{
    Unload();
}

//----------------------------------------------------------------------------------------------------
bool ShaderResource::Load()
{
    if (m_state == ResourceState::Loaded)
    {
        return true;
    }

    // ShaderLoader will handle the actual loading and call SetRendererShader
    // This method exists to fulfill the IResource interface
    return m_rendererShader != nullptr;
}

//----------------------------------------------------------------------------------------------------
void ShaderResource::Unload()
{
    if (m_rendererShader)
    {
        delete m_rendererShader;
        m_rendererShader = nullptr;
    }

    m_state = ResourceState::Unloaded;
}

//----------------------------------------------------------------------------------------------------
size_t ShaderResource::CalculateMemorySize() const
{
    // Basic estimation - shaders are typically small in memory
    // compared to textures, mainly just the compiled bytecode
    return sizeof(ShaderResource) + m_name.size() + 1024; // Rough estimate for shader bytecode
}

//----------------------------------------------------------------------------------------------------
void ShaderResource::SetRendererShader(Shader* shader)
{
    if (m_rendererShader && m_rendererShader != shader)
    {
        delete m_rendererShader;
    }

    m_rendererShader = shader;
    m_state = shader ? ResourceState::Loaded : ResourceState::Unloaded;
}
