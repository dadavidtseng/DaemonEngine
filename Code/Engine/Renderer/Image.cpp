//----------------------------------------------------------------------------------------------------
// Image.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#define STB_IMAGE_IMPLEMENTATION    // Exactly one .CPP (this Image.cpp) should #define this before #including stb_image.h
#include "Engine/Renderer/Image.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"
#include "ThirdParty/stb/stb_image.h"

//----------------------------------------------------------------------------------------------------
Image::Image(char const* imageFilePath)
    : m_imageFilePath(imageFilePath)
{
    int width         = 0;
    int height        = 0;
    int numComponents = 0;

    // Load image using stb_image
    unsigned char* imageData = stbi_load(imageFilePath,
                                         &width,
                                         &height,
                                         &numComponents,
                                         4);    // Force 4 components (RGBA)

    GUARANTEE_OR_DIE(imageData != nullptr,
                     "Failed to load image")

    // Set dimensions
    m_dimensions = IntVec2(width, height);

    // Copy image data to rgbaTexels
    size_t const numTexels = static_cast<size_t>(width) * static_cast<size_t>(height);
    m_rgbaTexels.resize(numTexels);

    for (size_t i = 0; i < numTexels; ++i)
    {
        unsigned char const* const pixel = &imageData[i * 4];
        m_rgbaTexels[i]                  = Rgba8(pixel[0], pixel[1], pixel[2], pixel[3]);
    }

    // Free the image memory
    stbi_image_free(imageData);
}

//----------------------------------------------------------------------------------------------------
Image::Image(IntVec2 const& size, Rgba8 const color)
    : m_dimensions(size)
{
    m_rgbaTexels.resize(static_cast<unsigned long long>(size.x) * size.y, color);
}

//----------------------------------------------------------------------------------------------------
String const& Image::GetImageFilePath() const
{
    return m_imageFilePath;
}

//----------------------------------------------------------------------------------------------------
IntVec2 Image::GetDimensions() const
{
    return m_dimensions;
}

//----------------------------------------------------------------------------------------------------
Rgba8 Image::GetTexelColor(IntVec2 const& texelCoords) const
{
    GUARANTEE_OR_DIE(texelCoords.x >= 0 &&
                     texelCoords.x < m_dimensions.x &&
                     texelCoords.y >= 0 &&
                     texelCoords.y < m_dimensions.y,
                     "Texel coordinates out of bounds")

    size_t const index = texelCoords.y * m_dimensions.x + texelCoords.x;

    return m_rgbaTexels[index];
}

//----------------------------------------------------------------------------------------------------
void const* Image::GetRawData() const
{
    return m_rgbaTexels.data();
}

//----------------------------------------------------------------------------------------------------
void Image::SetTexelColor(IntVec2 const& texelCoords, Rgba8 const& newColor)
{
    GUARANTEE_OR_DIE(texelCoords.x >= 0 &&
                     texelCoords.x < m_dimensions.x &&
                     texelCoords.y >= 0 &&
                     texelCoords.y < m_dimensions.y,
                     "Texel coordinates out of bounds")

    size_t const index  = texelCoords.y * m_dimensions.x + texelCoords.x;
    m_rgbaTexels[index] = newColor;
}
