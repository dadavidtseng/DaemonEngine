//----------------------------------------------------------------------------------------------------
// TextureResource.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Resource/TextureResource.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Renderer/Texture.hpp"

//----------------------------------------------------------------------------------------------------
TextureResource::TextureResource(String const& path, eResourceType type)
    : IResource(path, type)
{
    // Initialize texture properties
    m_name = path;
}

//----------------------------------------------------------------------------------------------------
TextureResource::~TextureResource()
{
    // Don't call DebuggerPrintf during destruction - logging system may be shut down
    TextureResource::Unload();
}

//----------------------------------------------------------------------------------------------------
bool TextureResource::Load()
{
    // Note: Actual loading will be implemented by TextureLoader
    // This method should be called after Renderer::Texture is created
    if (m_rendererTexture)
    {
        return true;
    }
    return false;
}

//----------------------------------------------------------------------------------------------------
void TextureResource::Unload()
{
    // TextureResource OWNS the Renderer::Texture - we must delete it
    // Don't call DebuggerPrintf during destruction - logging system may be shut down
    if (m_rendererTexture)
    {
        delete m_rendererTexture;
        m_rendererTexture = nullptr;
    }
}

//----------------------------------------------------------------------------------------------------
size_t TextureResource::CalculateMemorySize() const
{
    // Estimate memory size
    size_t totalSize = sizeof(TextureResource);

    if (m_rendererTexture)
    {
        // Estimate texture memory based on dimensions
        IntVec2 dimensions = GetDimensions();
        // Assume RGBA8 format (4 bytes per pixel)
        totalSize += dimensions.x * dimensions.y * 4;
    }

    return totalSize;
}

//----------------------------------------------------------------------------------------------------
IntVec2 TextureResource::GetDimensions() const
{
    if (m_rendererTexture)
    {
        return m_rendererTexture->GetDimensions();
    }
    return IntVec2::ZERO;
}

//----------------------------------------------------------------------------------------------------
void* TextureResource::GetD3DTexture() const
{
    // This is used for validation - we need to access private members
    // This will require friend access or public accessors in Texture class
    if (m_rendererTexture)
    {
        // For now, return a placeholder - this needs proper implementation
        // based on how Texture class exposes DirectX resources
        return reinterpret_cast<void*>(1); // Non-null indicates valid texture
    }
    return nullptr;
}

//----------------------------------------------------------------------------------------------------
void* TextureResource::GetShaderResourceView() const
{
    // Similar to GetD3DTexture, this needs proper implementation
    if (m_rendererTexture)
    {
        return reinterpret_cast<void*>(1); // Non-null indicates valid SRV
    }
    return nullptr;
}

//----------------------------------------------------------------------------------------------------
void TextureResource::SetRendererTexture(Texture* texture)
{
    m_rendererTexture = texture;
}
