//----------------------------------------------------------------------------------------------------
// WindowCommon.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include <cstdint>

//----------------------------------------------------------------------------------------------------
// TODO: add more window types, such as WINDOWED_FIXED and WINDOWED_EXPANDABLE, etc...
enum class eWindowType : uint8_t
{
    INVALID,
    WINDOWED,
    BORDERLESS,
    FULLSCREEN_LETTERBOX,
    FULLSCREEN_STRETCH,
    FULLSCREEN_CROP,
    MINIMIZED,
    HIDDEN
};

//
