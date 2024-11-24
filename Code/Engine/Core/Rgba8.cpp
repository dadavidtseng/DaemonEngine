//----------------------------------------------------------------------------------------------------
// Rgba8.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Core/Rgba8.hpp"

#include "StringUtils.hpp"

//----------------------------------------------------------------------------------------------------
Rgba8::Rgba8()
    : r(255), g(255), b(255), a(255)
{
}

Rgba8 Rgba8::WHITE = Rgba8(255, 255, 255, 255);

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
