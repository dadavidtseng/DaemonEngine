//----------------------------------------------------------------------------------------------------
// DevConsole.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Core/DevConsole.hpp"

#include <iostream>
#include <sstream>

#include "ErrorWarningAssert.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/Renderer.hpp"

//----------------------------------------------------------------------------------------------------
#if defined ERROR
#undef ERROR
#endif

//----------------------------------------------------------------------------------------------------
DevConsole* g_theDevConsole = nullptr;

//----------------------------------------------------------------------------------------------------
// Static color constants for different message types
STATIC Rgba8 const DevConsole::ERROR      = Rgba8(255, 0, 0);
STATIC Rgba8 const DevConsole::WARNING    = Rgba8(255, 255, 0);
STATIC Rgba8 const DevConsole::INFO_MAJOR = Rgba8(0, 255, 0);
STATIC Rgba8 const DevConsole::INFO_MINOR = Rgba8(0, 255, 255);

//----------------------------------------------------------------------------------------------------
DevConsole::DevConsole(DevConsoleConfig const& config)
    : m_config(config)
{
    AddLine(INFO_MINOR, "Welcome to DevConsole v0.1.0!");
}

//----------------------------------------------------------------------------------------------------
DevConsole::~DevConsole()
{
}

//----------------------------------------------------------------------------------------------------
// Subscribes to any events needed, prints an initial line of text, and starts the blink timer.
//
void DevConsole::StartUp()
{
    // Initialize any necessary resources for the console (fonts, etc.)
    g_theEventSystem->SubscribeEventCallbackFunction("WM_KEYDOWN", Event_KeyPressed);
    g_theEventSystem->SubscribeEventCallbackFunction("WM_CHAR", Event_CharInput);
}

//----------------------------------------------------------------------------------------------------
void DevConsole::Shutdown()
{
    // Clean up any allocated resources (like font objects, etc.)
}

//----------------------------------------------------------------------------------------------------
void DevConsole::BeginFrame()
{
    m_frameNumber++;
}

//----------------------------------------------------------------------------------------------------
void DevConsole::EndFrame()
{
    // Optionally clean up or limit the console history
}

//----------------------------------------------------------------------------------------------------
// Parses the current input line and executes it using the event system. Commands and arguments
// are delimited from each other with space (' ') and argument names and values are delimited
// with equals ('='). Echos the command to the dev console as well as any command output.
//
void DevConsole::Execute(String const& consoleCommandText, bool const echoCommand)
{
    // Create an input stream and initialize it
    std::istringstream stream;
    stream.str(consoleCommandText);

    std::string command;
    stream >> command; // Read first word and write it into command

    std::map<String, String> args;
    std::string              arg;

    while (stream >> arg)
    {
        // Parse arguments key=value
        size_t const pos = arg.find('=');

        if (pos != std::string::npos)
        {
            String       key   = arg.substr(0, pos);
            String const value = arg.substr(pos + 1);
            args[key]          = value;
        }
    }

    // Echo command if required
    if (echoCommand == true)
    {
        AddLine(INFO_MINOR, "Command: " + command);

        for (std::pair<String const, String> const& pair : args)
        {
            AddLine(INFO_MINOR, "Arg: " + pair.first + "=" + pair.second);
        }
    }

    // Fire the event for the command with the parsed arguments
    EventArgs eventArgs;

    for (std::pair<String const, String> const& pair : args)
    {
        eventArgs.SetValue(pair.first, pair.second);
    }

    g_theEventSystem->FireEvent(command, eventArgs);
}

//----------------------------------------------------------------------------------------------------
// Adds a line of text to the current list of lines being shown. Individual lines are delimited
// with the newline ('\n') character.
//
void DevConsole::AddLine(Rgba8 const& color, String const& text)
{
    DevConsoleLine line;
    line.m_color              = color;
    line.m_text               = text;
    line.m_frameNumberPrinted = m_frameNumber;
    line.m_timePrinted        = GetCurrentTimeSeconds();
    m_lines.push_back(line);
}

//----------------------------------------------------------------------------------------------------
// Renders just visible text lines within the bounds specified. Bounds are in terms of the
// camera being used to render. The current input line renders at the bottom with all other
// lines rendered above it, with the most recent lines at the bottom.
//
void DevConsole::Render(AABB2 const& bounds, Renderer* rendererOverride) const
{
    if (rendererOverride == nullptr)
    {
        rendererOverride = m_config.m_defaultRenderer;
    }

    BitmapFont const* font = rendererOverride->CreateOrGetBitmapFontFromFile(("Data/Fonts/" + m_config.m_defaultFontName).c_str());

    // Render the console depending on the current mode
    switch (m_mode)
    {
    case OPEN_FULL:
        Render_OpenFull(bounds, *rendererOverride, *font, m_config.m_defaultFontAspect);
        break;
    case OPEN_PARTIAL:
        // Render partial screen console if needed
        break;
    case HIDDEN:
        // Don't render
        break;
    case COMMAND_LINE_PROMPT:
        // Render the command line prompt
        break;
    }
}

//----------------------------------------------------------------------------------------------------
DevConsoleMode DevConsole::GetMode() const
{
    return m_mode;
}

//----------------------------------------------------------------------------------------------------
void DevConsole::SetMode(DevConsoleMode const mode)
{
    m_mode = mode;
}

//----------------------------------------------------------------------------------------------------
// Toggles between open and closed.
//
void DevConsole::ToggleMode(DevConsoleMode const mode)
{
    if (m_mode == mode)
    {
        m_mode = HIDDEN; // Hide console if already in that mode
    }
    else
    {
        m_mode = mode;
    }
}

//----------------------------------------------------------------------------------------------------
bool DevConsole::IsOpen() const
{
    return m_isOpen;
}

//----------------------------------------------------------------------------------------------------
// Handle key input.
//
STATIC bool DevConsole::Event_KeyPressed(EventArgs& args)
{
    String const keyCode   = args.GetValue("WM_KEYDOWN", "0");
    int const    inputChar = std::stoi(keyCode);

    if (inputChar == KEYCODE_ENTER)
    {
        g_theDevConsole->AddLine(INFO_MAJOR, g_theDevConsole->m_inputText);
    }

    return true;
}

//----------------------------------------------------------------------------------------------------
// Handle char input by appending valid characters to our current input line.
//
STATIC bool DevConsole::Event_CharInput(EventArgs& args)
{
    String const keyCode   = args.GetValue("WM_CHAR", "0");
    int const    inputChar = std::stoi(keyCode);

    if (inputChar >= KEYCODE_SPACE && inputChar < KEYCODE_TILDE)
    {
        g_theDevConsole->m_inputText += (char)inputChar;
    }

    return true;
}

//----------------------------------------------------------------------------------------------------
// Clear all lines of text.
//
STATIC bool DevConsole::Command_Clear(EventArgs& args)
{
    return false;
}

//----------------------------------------------------------------------------------------------------
// Display all currently registered commands in the event system.
//
STATIC bool DevConsole::Command_Help(EventArgs& args)
{
    return false;
}

//----------------------------------------------------------------------------------------------------
void DevConsole::Render_OpenFull(AABB2 const& bounds, Renderer& renderer, BitmapFont const& font, float const fontAspect) const
{
    VertexList  boxVerts;
    AABB2 const box = AABB2(Vec2::ZERO, Vec2(1600.f, 800.f));
    AddVertsForAABB2D(boxVerts, box, Rgba8::TRANSLUCENT_BLACK);
    renderer.BindTexture(nullptr);
    renderer.DrawVertexArray(static_cast<int>(boxVerts.size()), boxVerts.data());

    VertexList textVerts;

    AABB2 textBounds = bounds;

    // float lineHeight = box.GetDimensions().y / static_cast<float>(m_config.m_maxLinesDisplay);
    float lineHeight = 30.f;

    for (size_t i = 0; i < m_lines.size(); ++i)
    {
        DevConsoleLine const& line = m_lines[i];

        textBounds.m_maxs.y = bounds.m_maxs.y + static_cast<float>(i) * lineHeight;
        textBounds.m_mins.y = textBounds.m_maxs.y - lineHeight;

        font.AddVertsForTextInBox2D(
            textVerts,
            line.m_text,
            textBounds,
            lineHeight,
            line.m_color,
            fontAspect,
            Vec2::ZERO
        );
    }

    font.AddVertsForTextInBox2D(textVerts, m_inputText, textBounds, lineHeight, Rgba8::WHITE, fontAspect);

    renderer.BindTexture(&font.GetTexture());
    renderer.DrawVertexArray(static_cast<int>(textVerts.size()), textVerts.data());
}
