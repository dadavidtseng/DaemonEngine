//----------------------------------------------------------------------------------------------------
// Rgba8.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Core/Rgba8.hpp"

#include "Engine/Core/StringUtils.hpp"
#include "Engine/Math/MathUtils.hpp"

//----------------------------------------------------------------------------------------------------
Rgba8::Rgba8()
    : r(255),
      g(255),
      b(255),
      a(255)
{
}

Rgba8 Rgba8::WHITE             = Rgba8(255, 255, 255);
Rgba8 Rgba8::BLACK             = Rgba8(0, 0, 0);
Rgba8 Rgba8::TRANSLUCENT_BLACK = Rgba8(0, 0, 0, 200);
Rgba8 Rgba8::GREY              = Rgba8(50, 50, 50);
Rgba8 Rgba8::RED               = Rgba8(255, 0, 0);
Rgba8 Rgba8::GREEN             = Rgba8(0, 255, 0);
Rgba8 Rgba8::MAGENTA           = Rgba8(255, 0, 255);
Rgba8 Rgba8::CYAN              = Rgba8(0, 255, 255);
Rgba8 Rgba8::YELLOW            = Rgba8(255, 255, 0);
Rgba8 Rgba8::BLUE              = Rgba8(0, 0, 255);

//----------------------------------------------------------------------------------------------------
Rgba8::Rgba8
(
    const unsigned char red,
    const unsigned char green,
    const unsigned char blue,
    const unsigned char alpha
)
    : r(red), g(green), b(blue), a(alpha)
{
}

//----------------------------------------------------------------------------------------------------
void Rgba8::SetFromText(char const* text)
{
    // Use SplitStringOnDelimiter to divide the input text into parts based on the delimiter ','
    Strings const parts = SplitStringOnDelimiter(text, ',');

    // Input must contain either 3 or 4 parts; otherwise, reset to default values
    if (parts.size() < 3 ||
        parts.size() > 4)
    {
        r = g = b = 255;
        a = 255;

        return;
    }

    // Convert the first three parts to unsigned char for RGB values
    r = static_cast<unsigned char>(atoi(parts[0].c_str()));
    g = static_cast<unsigned char>(atoi(parts[1].c_str()));
    b = static_cast<unsigned char>(atoi(parts[2].c_str()));

    // If a fourth part exists, use it for the alpha value; otherwise, set alpha to 255
    a = parts.size() == 4 ? static_cast<unsigned char>(atoi(parts[3].c_str())) : 255;
}

//----------------------------------------------------------------------------------------------------
Rgba8 Interpolate(Rgba8 const startColor, Rgba8 const endColor, float const fractionOfEnd)
{
    float const red   = Interpolate(NormalizeByte(startColor.r), NormalizeByte(endColor.r), fractionOfEnd);
    float const green = Interpolate(NormalizeByte(startColor.g), NormalizeByte(endColor.g), fractionOfEnd);
    float const blue  = Interpolate(NormalizeByte(startColor.b), NormalizeByte(endColor.b), fractionOfEnd);
    float const alpha = Interpolate(NormalizeByte(startColor.a), NormalizeByte(endColor.a), fractionOfEnd);

    return Rgba8(DenormalizeByte(red), DenormalizeByte(green), DenormalizeByte(blue), DenormalizeByte(alpha));
}
