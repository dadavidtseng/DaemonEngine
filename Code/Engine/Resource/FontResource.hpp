//----------------------------------------------------------------------------------------------------
// FontResource.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include "Engine/Resource/IResource.hpp"
#include "Engine/Core/StringUtils.hpp"

//----------------------------------------------------------------------------------------------------
class BitmapFont;
class Renderer;

//----------------------------------------------------------------------------------------------------
class FontResource : public IResource
{
public:
    FontResource(String const& path, eResourceType type);
    ~FontResource() override;

    // IResource interface implementation
    bool   Load() override;
    void   Unload() override;
    size_t CalculateMemorySize() const override;

    // Font-specific interface
    String  GetName() const { return m_name; }

    // Renderer::BitmapFont access (for Renderer integration)
    BitmapFont* GetRendererBitmapFont() const { return m_rendererBitmapFont; }

private:
    friend class FontLoader;
    // friend class Renderer; // Allow Renderer to access resources

    // Font properties
    String  m_name;

    // Wrapped Renderer::BitmapFont
    BitmapFont* m_rendererBitmapFont = nullptr;

    // Resource creation methods (called by FontLoader)
    void SetRendererBitmapFont(BitmapFont* font);
    void SetName(String const& name) { m_name = name; }
};
