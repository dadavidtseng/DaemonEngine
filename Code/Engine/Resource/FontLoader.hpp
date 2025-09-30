//----------------------------------------------------------------------------------------------------
// FontLoader.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once

#include "Engine/Resource/IResourceLoader.hpp"
#include "Engine/Core/StringUtils.hpp"

//----------------------------------------------------------------------------------------------------
class Renderer;
class FontResource;

//----------------------------------------------------------------------------------------------------
class FontLoader : public IResourceLoader
{
public:
    explicit FontLoader(Renderer* renderer);
    virtual ~FontLoader();

    // IResourceLoader interface implementation
    bool                           CanLoad(String const& extension) const override;
    std::shared_ptr<IResource>     Load(String const& path) override;
    StringList                     GetSupportedExtensions() const override;

private:
    Renderer* m_renderer = nullptr;

    // Helper methods
    bool LoadFontFromFile(String const& path, FontResource* fontResource);
};
