//----------------------------------------------------------------------------------------------------
// DevConsole.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once

#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/StringUtils.hpp"

//----------------------------------------------------------------------------------------------------
class BitmapFont;
class Renderer;
struct AABB2;

//----------------------------------------------------------------------------------------------------
class DevConsole
{
public:
    // DevConsole(DevConsoleConfig const& config);
    // ~DevConsole();
    // void StartUp();
    // void Shutdown();
    // void BeginFrame();
    // void EndFrame();
    //
    // void Execute(String const& consoleCommandText);
    // void AddLine(Rgba8 const& color, String const& text);
    // void Render(AABB2 const& bounds, Renderer* rendererOverride = nullptr) const;

//     DevConsoleMode GetMode() const;
//     void           SetMode(DevConsoleMode mode);
//     void           ToggleMode(DevConsoleMode mode);
//
//     static const Rgba8::ERROR;
//     static const Rgba8::WARNING;
//     static const Rgba8::INFO_MAJOR;
//     static const Rgba8::INFO_MINOR;
//
// protected:
//     void Render_OpenFull(AABB2 const& bounds, Renderer& renderer, BitmapFont& font, float fontAspect = 1.f) const;
//
//     DevConsoleConfig            m_config;
//     DevConsoleMode              m_mode = DevConsoleMode::HIDDEN;
//     std::vector<DevConsoleLine> m_lines;     //TODO: support a max limited # of lines (e.g. fixed circular buffer)
//     int                         m_frameNumber = 0;
};
