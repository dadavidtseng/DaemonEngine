//----------------------------------------------------------------------------------------------------
// TextureLoader.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Resource/TextureLoader.hpp"
#include <algorithm>
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Renderer/Image.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Resource/ResourceCommon.hpp"
#include "Engine/Resource/TextureResource.hpp"
#include "ThirdParty/stb/stb_image.h"
#include <d3d11.h>

//----------------------------------------------------------------------------------------------------
TextureLoader::TextureLoader(ID3D11Device* device)
    : m_device(device)
{
    GUARANTEE_OR_DIE(m_device != nullptr, "TextureLoader requires a valid D3D11 device");
}

//----------------------------------------------------------------------------------------------------
TextureLoader::~TextureLoader()
{
}

//----------------------------------------------------------------------------------------------------
bool TextureLoader::CanLoad(String const& extension) const
{
    String lowerExt = extension;
    std::transform(lowerExt.begin(), lowerExt.end(), lowerExt.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    // Accept common image file extensions
    if (lowerExt == ".png" || lowerExt == ".jpg" || lowerExt == ".jpeg" ||
        lowerExt == ".bmp" || lowerExt == ".tga" || lowerExt == ".dds")
    {
        return true;
    }

    return false;
}

//----------------------------------------------------------------------------------------------------
std::shared_ptr<IResource> TextureLoader::Load(String const& path)
{
    auto textureResource = std::make_shared<TextureResource>(path, ResourceType::Texture);

    if (LoadTextureFromFile(path, textureResource.get()))
    {
        return textureResource;
    }

    return nullptr;
}

//----------------------------------------------------------------------------------------------------
StringList TextureLoader::GetSupportedExtensions() const
{
    return {".png", ".jpg", ".jpeg", ".bmp", ".tga", ".dds"};
}

//----------------------------------------------------------------------------------------------------
Texture* TextureLoader::CreateTextureFromFile(char const* imageFilePath)
{
    IntVec2 dimensions    = IntVec2::ZERO; // This will be filled in for us to indicate image width & height
    int     bytesPerTexel = 0;
    // This will be filled in for us to indicate how many color components the image had (e.g. 3=RGB=24bit, 4=RGBA=32bit)
    int constexpr numComponentsRequested = 0; // don't care; we support 3 (24-bit RGB) or 4 (32-bit RGBA)

    // Load (and decompress) the image RGB(A) bytes from a file on disk into a memory buffer (array of bytes)
    stbi_set_flip_vertically_on_load(1); // We prefer uvTexCoords has origin (0,0) at BOTTOM LEFT
    unsigned char* texelData = stbi_load(imageFilePath,
                                         &dimensions.x,
                                         &dimensions.y,
                                         &bytesPerTexel,
                                         numComponentsRequested);

    GUARANTEE_OR_DIE(texelData, Stringf("Failed to load image \"%s\"", imageFilePath))

    Image const fileImage(imageFilePath);
    Texture*    newTexture = CreateTextureFromImage(fileImage);

    // Free the raw image texel data now that we've sent a copy of it down to the GPU to be stored in video memory
    stbi_image_free(texelData);

    return newTexture;
}

//----------------------------------------------------------------------------------------------------
Texture* TextureLoader::CreateTextureFromImage(Image const& image)
{
    Texture* newTexture      = new Texture();
    newTexture->m_name       = image.GetImageFilePath();
    newTexture->m_dimensions = image.GetDimensions();

    D3D11_TEXTURE2D_DESC textureDesc = {};
    textureDesc.Width                = image.GetDimensions().x;
    textureDesc.Height               = image.GetDimensions().y;
    textureDesc.MipLevels            = 1;
    textureDesc.ArraySize            = 1;
    textureDesc.Format               = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.SampleDesc.Count     = 1;
    textureDesc.Usage                = D3D11_USAGE_IMMUTABLE;
    textureDesc.BindFlags            = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA textureData;
    textureData.pSysMem     = image.GetRawData();
    textureData.SysMemPitch = 4 * image.GetDimensions().x;

    HRESULT hr = m_device->CreateTexture2D(&textureDesc, &textureData, &newTexture->m_texture);

    if (!SUCCEEDED(hr))
    {
        ERROR_AND_DIE(Stringf("CreateTextureFromImage failed for image file \"%s\".", image.GetImageFilePath().c_str()))
    }

    hr = m_device->CreateShaderResourceView(newTexture->m_texture, NULL, &newTexture->m_shaderResourceView);

    if (!SUCCEEDED(hr))
    {
        ERROR_AND_DIE(Stringf("CreateShaderResourceView failed for image file \"%s\".", image.GetImageFilePath().c_str()))
    }

    // No caching here - ResourceCache handles that!
    return newTexture;
}

//----------------------------------------------------------------------------------------------------
bool TextureLoader::LoadTextureFromFile(String const& path, TextureResource* textureResource)
{
    // Create texture directly (no caching - ResourceCache handles that)
    Texture* rendererTexture = CreateTextureFromFile(path.c_str());

    if (!rendererTexture)
    {
        DebuggerPrintf("Error: TextureLoader could not load texture '%s'.\n", path.c_str());
        return false;
    }

    // Set the texture resource properties
    textureResource->SetName(path);
    textureResource->SetRendererTexture(rendererTexture);

    // Call Load() to validate the resource is properly loaded
    bool loadResult = textureResource->Load();

    if (!loadResult)
    {
        DebuggerPrintf("Error: TextureResource::Load() failed for '%s'\n", path.c_str());
        return false;
    }

    DebuggerPrintf("Info: TextureLoader successfully loaded texture '%s'.\n", path.c_str());
    return true;
}
