//----------------------------------------------------------------------------------------------------
// TextureResource.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include "Engine/Resource/IResource.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Math/IntVec2.hpp"

//----------------------------------------------------------------------------------------------------
class Texture;
class Renderer;

//----------------------------------------------------------------------------------------------------
class TextureResource : public IResource
{
public:
    TextureResource(String const& path, eResourceType type);
    ~TextureResource() override;

    // IResource interface implementation
    bool   Load() override;
    void   Unload() override;
    size_t CalculateMemorySize() const override;

    // Texture-specific interface
    String  GetName() const { return m_name; }
    IntVec2 GetDimensions() const;

    // Renderer::Texture access (for Renderer integration)
    Texture* GetRendererTexture() const { return m_rendererTexture; }

    // DirectX resource access (for validation)
    void* GetD3DTexture() const;
    void* GetShaderResourceView() const;

private:
    friend class TextureLoader;
    friend class Renderer; // Allow Renderer to access resources
    friend class ResourceSubsystem; // Allow ResourceSubsystem to create default texture

    // Texture properties
    String m_name;

    // Wrapped Renderer::Texture
    Texture* m_rendererTexture = nullptr;

    // Resource creation methods (called by TextureLoader)
    void SetRendererTexture(Texture* texture);
    void SetName(String const& name) { m_name = name; }
};
