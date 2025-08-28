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
Rgba8 Rgba8::DARK_RED          = Rgba8(139, 0, 0);
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
Vec3 Rgba8::GetAsVec3() const
{
    Vec3 result;

    result.x = NormalizeByte(r);
    result.y = NormalizeByte(g);
    result.z = NormalizeByte(b);

    return result;
}

//----------------------------------------------------------------------------------------------------
Vec4 Rgba8::GetAsVec4() const
{
    Vec4 result;

    result.x = NormalizeByte(r);
    result.y = NormalizeByte(g);
    result.z = NormalizeByte(b);
    result.w = NormalizeByte(a);

    return result;
}

//----------------------------------------------------------------------------------------------------
bool Rgba8::operator==(Rgba8 const& compare) const
{
    return
        r == compare.r &&
        g == compare.g &&
        b == compare.b &&
        a == compare.a;
}

//----------------------------------------------------------------------------------------------------
Rgba8 Interpolate(Rgba8 const start,
                  Rgba8 const end,
                  float const fractionOfEnd)
{
    float const red   = Interpolate(NormalizeByte(start.r), NormalizeByte(end.r), fractionOfEnd);
    float const green = Interpolate(NormalizeByte(start.g), NormalizeByte(end.g), fractionOfEnd);
    float const blue  = Interpolate(NormalizeByte(start.b), NormalizeByte(end.b), fractionOfEnd);
    float const alpha = Interpolate(NormalizeByte(start.a), NormalizeByte(end.a), fractionOfEnd);

    return Rgba8(DenormalizeByte(red), DenormalizeByte(green), DenormalizeByte(blue), DenormalizeByte(alpha));
}
