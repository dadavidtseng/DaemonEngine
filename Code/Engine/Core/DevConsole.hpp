//----------------------------------------------------------------------------------------------------
// DevConsole.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once

#include "EventSystem.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/StringUtils.hpp"

//----------------------------------------------------------------------------------------------------
class BitmapFont;
class Renderer;
struct AABB2;

struct DevConsoleLine
{
    Rgba8  m_color;
    String m_text;
    int    m_frameNumberPrinted;
    double m_timePrinted;
};

enum DevConsoleMode
{
    OPEN_FULL,
    OPEN_PARTIAL,
    HIDDEN,
    COMMAND_LINE_PROMPT,
    NUM
};

struct DevConsoleConfig
{
    Renderer* m_defaultRenderer = nullptr;
    String    m_defaultFontName = "System";
    // float m_lineOnScreen = 
};

//----------------------------------------------------------------------------------------------------
class DevConsole
{
public:
    DevConsole(DevConsoleConfig const& config);
    ~DevConsole();
    void StartUp();
    void Shutdown();
    void BeginFrame();
    void EndFrame();

    void Execute(String const& consoleCommandText);
    void AddLine(Rgba8 const& color, String const& text);
    void Render(AABB2 const& bounds, Renderer* rendererOverride = nullptr) const;

    DevConsoleMode GetMode() const;
    void           SetMode(DevConsoleMode mode);
    void           ToggleMode(DevConsoleMode mode);
    bool           Command_Test(EventArgs& args);

    // static const Rgba8 ERROR;
    // static const Rgba8 WARNING;
    static const Rgba8 INFO_MAJOR;
    // static const Rgba8 INFO_MINOR;

protected:
    void Render_OpenFull(AABB2 const& bounds, Renderer& renderer, BitmapFont& font, float fontAspect = 1.f) const;

    DevConsoleConfig            m_config;
    DevConsoleMode              m_mode = HIDDEN;
    std::vector<DevConsoleLine> m_lines;     //TODO: support a max limited # of lines (e.g. fixed circular buffer)
    int                         m_frameNumber = 0;
};
