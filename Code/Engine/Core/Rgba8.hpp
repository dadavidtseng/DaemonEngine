#pragma once

//-----------------------------------------------------------------------------------------------
struct Rgba8
{
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;

    // TODO: ADD colors 
    static Rgba8 WHITE;
    static Rgba8 BLACK;
    static Rgba8 GREY;
    static Rgba8 RED;
    static Rgba8 GREEN;
    static Rgba8 MAGENTA;
    static Rgba8 CYAN;
    static Rgba8 YELLOW;
    static Rgba8 BLUE;

    Rgba8();
    explicit Rgba8(unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha = 255);

    void SetFromText(char const* text);

    // Rgba8 Interpolate(Rgba8 startColor, Rgba8 endColor, float fractionOfEnd)
    // {
    //     Rgba8 blendedColor;
    //     float red = Interpolate(NormalizeByte(startColor.r), NormalizeByte(endColor.r), fractionOfEnd.r);
    //     float green = Interpolate(NormalizeByte(startColor.r), NormalizeByte(endColor.r), fractionOfEnd.r);
    //     float blue = Interpolate(NormalizeByte(startColor.r), NormalizeByte(endColor.r), fractionOfEnd.r);
    //     float alpha = Interpolate(NormalizeByte(startColor.r), NormalizeByte(endColor.r), fractionOfEnd.r);
    //
    //     blendedColor.r = DenormalizeByte(red);
    //     blendedColor.r = DenormalizeByte(green);
    //     blendedColor.r = DenormalizeByte(blue);
    //     blendedColor.r = DenormalizeByte(alpha);
    //     return blendedColor;
    // }
};
