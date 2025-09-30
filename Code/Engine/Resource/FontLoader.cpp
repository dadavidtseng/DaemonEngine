//----------------------------------------------------------------------------------------------------
// FontLoader.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Resource/FontLoader.hpp"
#include "Engine/Resource/FontResource.hpp"
#include "Engine/Resource/ResourceCommon.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include <algorithm>

//----------------------------------------------------------------------------------------------------
FontLoader::FontLoader(Renderer* renderer)
    : m_renderer(renderer)
{
    GUARANTEE_OR_DIE(m_renderer != nullptr, "FontLoader requires a valid Renderer instance");
}

//----------------------------------------------------------------------------------------------------
FontLoader::~FontLoader()
{
}

//----------------------------------------------------------------------------------------------------
bool FontLoader::CanLoad(String const& extension) const
{
    String lowerExt = extension;
    std::transform(lowerExt.begin(), lowerExt.end(), lowerExt.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    // Accept specific font file extensions
    if (lowerExt == ".fnt" || lowerExt == ".font")
    {
        return true;
    }

    // For empty extensions, only accept if it's likely a font path
    if (lowerExt.empty())
    {
        // This is a heuristic - we'll need to check the path context
        // For now, return true and let Load() method handle the validation
        return true;
    }

    return false;
}

//----------------------------------------------------------------------------------------------------
std::shared_ptr<IResource> FontLoader::Load(String const& path)
{
    auto fontResource = std::make_shared<FontResource>(path, ResourceType::Font);

    if (LoadFontFromFile(path, fontResource.get()))
    {
        return fontResource;
    }

    return nullptr;
}

//----------------------------------------------------------------------------------------------------
StringList FontLoader::GetSupportedExtensions() const
{
    return { ".fnt", ".font", "" }; // Empty string for extension-less font names
}

//----------------------------------------------------------------------------------------------------
bool FontLoader::LoadFontFromFile(String const& path, FontResource* fontResource)
{
    // Use the Renderer's existing bitmap font loading functionality
    BitmapFont* rendererBitmapFont = m_renderer->CreateOrGetBitmapFontFromFile(path.c_str());

    if (!rendererBitmapFont)
    {
        DebuggerPrintf("Error: FontLoader could not load bitmap font '%s'.\n", path.c_str());
        return false;
    }

    // Set the font resource properties
    fontResource->SetName(path);
    fontResource->SetRendererBitmapFont(rendererBitmapFont);

    // Call Load() to validate the resource is properly loaded
    bool loadResult = fontResource->Load();

    if (!loadResult)
    {
        DebuggerPrintf("Error: FontResource::Load() failed for '%s'\n", path.c_str());
        return false;
    }

    DebuggerPrintf("Info: FontLoader successfully loaded font '%s'.\n", path.c_str());
    return true;
}
