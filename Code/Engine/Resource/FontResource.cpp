//----------------------------------------------------------------------------------------------------
// FontResource.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Resource/FontResource.hpp"
//----------------------------------------------------------------------------------------------------
#include "Engine/Renderer/BitmapFont.hpp"

//----------------------------------------------------------------------------------------------------
FontResource::FontResource(String const& path,
                           eResourceType const type)
    : IResource(path, type)
{
    // Initialize font properties
    m_name = path;
}

//----------------------------------------------------------------------------------------------------
FontResource::~FontResource()
{
    FontResource::Unload();
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
    // FontResource OWNS the Renderer::BitmapFont - we must delete it
    // Don't call DebuggerPrintf during destruction - logging system may be shut down
    if (m_rendererBitmapFont)
    {
        delete m_rendererBitmapFont;
        m_rendererBitmapFont = nullptr;
    }
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
