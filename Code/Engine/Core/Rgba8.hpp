#pragma once

//-----------------------------------------------------------------------------------------------
struct Rgba8
{
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;

    static Rgba8 WHITE;

    Rgba8();
    explicit Rgba8(
        unsigned char red,
        unsigned char green,
        unsigned char blue,
        unsigned char alpha = 255
    );
};
