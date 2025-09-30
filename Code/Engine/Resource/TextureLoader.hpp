//----------------------------------------------------------------------------------------------------
// TextureLoader.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once

#include "Engine/Core/StringUtils.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Resource/IResourceLoader.hpp"

//-Forward-Declaration--------------------------------------------------------------------------------
class Image;
class TextureResource;

//----------------------------------------------------------------------------------------------------
class TextureLoader : public IResourceLoader
{
public:
    explicit TextureLoader(ID3D11Device* device);
    ~TextureLoader() override;

    // IResourceLoader interface implementation
    bool                       CanLoad(String const& extension) const override;
    std::shared_ptr<IResource> Load(String const& path) override;
    StringList                 GetSupportedExtensions() const override;

private:
    ID3D11Device* m_device = nullptr;

    // Internal texture creation (no caching - ResourceCache handles that)
    Texture* CreateTextureFromFile(char const* imageFilePath);
    Texture* CreateTextureFromImage(Image const& image);

    // Helper methods
    bool LoadTextureFromFile(String const& path, TextureResource* textureResource);
};
