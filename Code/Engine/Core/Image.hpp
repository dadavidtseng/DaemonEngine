//----------------------------------------------------------------------------------------------------
// Image.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Math/IntVec2.hpp"

//----------------------------------------------------------------------------------------------------
class Image
{
public:
    explicit Image(char const* imageFilePath);
    Image(IntVec2 const& size, Rgba8 color);

    // Accessors
    String const& GetImageFilePath() const;
    IntVec2       GetDimensions() const;
    Rgba8         GetTexelColor(IntVec2 const& texelCoords) const;
    void const*   GetRawData() const;

    // Mutators
    void SetTexelColor(IntVec2 const& texelCoords, Rgba8 const& newColor);

private:
    String             m_imageFilePath;
    IntVec2            m_dimensions = IntVec2::ZERO;
    std::vector<Rgba8> m_rgbaTexels;  // or Rgba8* m_rgbaTexels = nullptr; if you prefer new[] and delete[]
};
