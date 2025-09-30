//----------------------------------------------------------------------------------------------------
// FontResource.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Resource/FontResource.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Renderer/BitmapFont.hpp"

//----------------------------------------------------------------------------------------------------
FontResource::FontResource(String const& path, ResourceType type)
    : IResource(path, type)
{
    // Initialize font properties
    m_name = path;
}

//----------------------------------------------------------------------------------------------------
FontResource::~FontResource()
{
    Unload();
}

//----------------------------------------------------------------------------------------------------
bool FontResource::Load()
{
    // Note: Actual loading will be implemented by FontLoader
    // This method should be called after Renderer::BitmapFont is created
    if (m_rendererBitmapFont)
    {
        return true;
    }
    return false;
}

//----------------------------------------------------------------------------------------------------
void FontResource::Unload()
{
    // Note: Renderer::BitmapFont is managed by the Renderer
    // We don't directly delete it here as it might be shared
    m_rendererBitmapFont = nullptr;
}

//----------------------------------------------------------------------------------------------------
size_t FontResource::CalculateMemorySize() const
{
    // Estimate memory size
    size_t totalSize = sizeof(FontResource);

    // The actual font and texture data are managed by Renderer
    // Just account for the resource wrapper overhead
    return totalSize;
}

//----------------------------------------------------------------------------------------------------
void FontResource::SetRendererBitmapFont(BitmapFont* font)
{
    m_rendererBitmapFont = font;
}
