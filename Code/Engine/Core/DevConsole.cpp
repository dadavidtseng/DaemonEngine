//----------------------------------------------------------------------------------------------------
// DevConsole.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Core/DevConsole.hpp"
#include <sstream>
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/Time.hpp"
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
}

//----------------------------------------------------------------------------------------------------
DevConsole::~DevConsole()
{
}

//----------------------------------------------------------------------------------------------------
void DevConsole::StartUp()
{
    // Initialize any necessary resources for the console (fonts, etc.)
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
void DevConsole::Execute(String const& consoleCommandText)
{
    std::istringstream stream(consoleCommandText);
    std::string        commandLine;

    while (std::getline(stream, commandLine))
    {
        std::istringstream lineStream(commandLine);
        std::string        command;
        lineStream >> command; // First word is the command

        std::map<String, String> args;
        std::string              arg;

        while (lineStream >> arg)
        {
            // Parse arguments key=value
            size_t pos = arg.find('=');

            if (pos != std::string::npos)
            {
                String key   = arg.substr(0, pos);
                String value = arg.substr(pos + 1);
                args[key]    = value;
            }
        }

        // Fire the event for the command with the parsed arguments
        EventArgs eventArgs;
        g_theEventSystem->FireEvent(command, eventArgs);
    }
}

//----------------------------------------------------------------------------------------------------
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
bool DevConsole::IsOpened() const
{
    return m_isOpen;
}

//----------------------------------------------------------------------------------------------------
bool DevConsole::Command_Test(EventArgs& args)
{
    UNUSED(args)
    AddLine(INFO_MAJOR, "Test command received");
    return false; // Continue calling other subscribers
}

bool DevConsole::Event_KeyPressed(EventArgs& args)
{
    return false;
}

bool DevConsole::Event_CharInput(EventArgs& args)
{
    return false;
}

bool DevConsole::Command_Clear(EventArgs& args)
{
    return false;
}

bool DevConsole::Command_Help(EventArgs& args)
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

    AABB2           textBounds = bounds;
    float constexpr lineHeight = 50.f;

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

    renderer.BindTexture(&font.GetTexture());
    renderer.DrawVertexArray(static_cast<int>(textVerts.size()), textVerts.data());
}
