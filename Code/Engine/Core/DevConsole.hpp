//----------------------------------------------------------------------------------------------------
// DevConsole.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once

#include "EventSystem.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/StringUtils.hpp"

class Timer;
//----------------------------------------------------------------------------------------------------
class BitmapFont;
class Camera;
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
    Renderer* m_defaultRenderer   = nullptr;
    Camera*   m_defaultCamera     = nullptr;
    String    m_defaultFontName   = "SquirrelFixedFont";
    float     m_defaultFontAspect = 1.f;
    float     m_maxLinesDisplay   = 29.5f;
    int       m_maxCommandHistory = 128;
    bool      m_startOpen         = false;
};

//----------------------------------------------------------------------------------------------------
class DevConsole
{
public:
    explicit DevConsole(DevConsoleConfig const& config);
    ~DevConsole();
    void StartUp();
    void Shutdown();
    void BeginFrame();
    void EndFrame();

    void Execute(String const& consoleCommandText);
    // void Execute(String const& consoleCommandText, bool echoCommand = true);
    void AddLine(Rgba8 const& color, String const& text);
    void Render(AABB2 const& bounds, Renderer* rendererOverride = nullptr) const;

    DevConsoleMode GetMode() const;
    void           SetMode(DevConsoleMode mode);
    void           ToggleMode(DevConsoleMode mode);
    bool           Command_Test(EventArgs& args);

    static bool Event_KeyPressed(EventArgs& args);
    static bool Event_CharInput(EventArgs& args);
    static bool Command_Clear(EventArgs& args);
    static bool Command_Help(EventArgs& args);

    static const Rgba8 ERROR;
    static const Rgba8 WARNING;
    static const Rgba8 INFO_MAJOR;
    static const Rgba8 INFO_MINOR;
    static const Rgba8 INPUT_TEXT;
    static const Rgba8 INPUT_INSERTION_POINT;

protected:
    void Render_OpenFull(AABB2 const& bounds, Renderer& renderer, BitmapFont const& font, float fontAspect = 1.f) const;

    DevConsoleConfig            m_config;
    DevConsoleMode              m_mode = HIDDEN;
    std::vector<DevConsoleLine> m_lines;     //TODO: support a max limited # of lines (e.g. fixed circular buffer)
    int                         m_frameNumber = 0;

    bool    m_isOpen = false;
    String  m_inputText;
    int     m_insertionPointPosition = 0;
    bool    m_insertionPointVisible  = false;
    Timer*  m_insertionPointBlinkTimer;
    Strings m_commandHistory;
    int     m_historyIndex = -1;
};
