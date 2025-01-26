//----------------------------------------------------------------------------------------------------
// Rgba8.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once

//----------------------------------------------------------------------------------------------------
struct Rgba8
{
    unsigned char r = 255;
    unsigned char g = 255;
    unsigned char b = 255;
    unsigned char a = 255;

    // TODO: ADD colors 
    static Rgba8 WHITE;
    static Rgba8 BLACK;
    static Rgba8 TRANSLUCENT_BLACK;
    static Rgba8 GREY;
    static Rgba8 RED;
    static Rgba8 GREEN;
    static Rgba8 MAGENTA;
    static Rgba8 CYAN;
    static Rgba8 YELLOW;
    static Rgba8 BLUE;

    Rgba8() = default;
    explicit Rgba8(unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha = 255);

    void SetFromText(char const* text);
    void GetAsFloats(float* colorAsFloats) const;
};

//----------------------------------------------------------------------------------------------------
Rgba8 Interpolate(Rgba8 startColor, Rgba8 endColor, float fractionOfEnd);
