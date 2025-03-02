//----------------------------------------------------------------------------------------------------
// Rgba8.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Core/Rgba8.hpp"

#include "Engine/Core/StringUtils.hpp"
#include "Engine/Math/MathUtils.hpp"

//----------------------------------------------------------------------------------------------------
Rgba8 Rgba8::WHITE             = Rgba8(255, 255, 255);
Rgba8 Rgba8::TRANSLUCENT_WHITE = Rgba8(255, 255, 255, 30);
Rgba8 Rgba8::BLACK             = Rgba8(0, 0, 0);
Rgba8 Rgba8::TRANSLUCENT_BLACK = Rgba8(0, 0, 0, 200);
Rgba8 Rgba8::DARK_GREY         = Rgba8(100, 100, 100);
Rgba8 Rgba8::GREY              = Rgba8(50, 50, 50);
Rgba8 Rgba8::RED               = Rgba8(255, 0, 0);
Rgba8 Rgba8::GREEN             = Rgba8(0, 255, 0);
Rgba8 Rgba8::MAGENTA           = Rgba8(255, 0, 255);
Rgba8 Rgba8::CYAN              = Rgba8(0, 255, 255);
Rgba8 Rgba8::YELLOW            = Rgba8(255, 255, 0);
Rgba8 Rgba8::BLUE              = Rgba8(0, 0, 255);
Rgba8 Rgba8::LIGHT_BLUE        = Rgba8(100, 150, 255);
Rgba8 Rgba8::ORANGE            = Rgba8(255, 127, 0);

//----------------------------------------------------------------------------------------------------
Rgba8::Rgba8
(
    unsigned const char red,
    unsigned const char green,
    unsigned const char blue,
    unsigned const char alpha
)
    : r(red), g(green), b(blue), a(alpha)
{
}

//----------------------------------------------------------------------------------------------------
void Rgba8::SetFromText(char const* text)
{
    // Use SplitStringOnDelimiter to divide the input text into parts based on the delimiter ','
    StringList const parts = SplitStringOnDelimiter(text, ',');

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

// TODO: ask what to do if the given rgba is out of range
//----------------------------------------------------------------------------------------------------
void Rgba8::GetAsFloats(float* colorAsFloats) const
{
    colorAsFloats[0] = NormalizeByte(r);    // Red
    colorAsFloats[1] = NormalizeByte(g);    // Green
    colorAsFloats[2] = NormalizeByte(b);    // Blue
    colorAsFloats[3] = NormalizeByte(a);    // Alpha
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
