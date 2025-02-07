

#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Core/StringUtils.hpp"
#include <sstream>

#include "Time.hpp"
#include "Game/GameCommon.hpp"

DevConsole* g_theDevConsole = nullptr;

// Static color constants for different message types
// Rgba8 const DevConsole::ERROR = Rgba8(255, 0, 0);
// Rgba8 const DevConsole::WARNING = Rgba8(255, 255, 0);
Rgba8 const DevConsole::INFO_MAJOR = Rgba8(0, 255, 0);
// Rgba8 const DevConsole::INFO_MINOR = Rgba8(0, 255, 255);

DevConsole::DevConsole(DevConsoleConfig const& config)
    : m_config(config)
{
}

DevConsole::~DevConsole()
{
}

void DevConsole::StartUp()
{
    // Initialize any necessary resources for the console (fonts, etc.)
}

void DevConsole::Shutdown()
{
    // Clean up any allocated resources (like font objects, etc.)
}

void DevConsole::BeginFrame()
{
    m_frameNumber++;
}

void DevConsole::EndFrame()
{
    // Optionally clean up or limit the console history
}

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
            auto pos = arg.find('=');
            if (pos != std::string::npos)
            {
                String key = arg.substr(0, pos);
                String value = arg.substr(pos + 1);
                args[key] = value;
            }
        }

        // Fire the event for the command with the parsed arguments
        EventArgs eventArgs;
        g_theEventSystem->FireEvent(command, eventArgs);
    }
}

void DevConsole::AddLine(Rgba8 const& color, String const& text)
{
    DevConsoleLine line;
    line.m_color = color;
    line.m_text = text;
    line.m_frameNumberPrinted = m_frameNumber;
    line.m_timePrinted = GetCurrentTimeSeconds();
    m_lines.push_back(line);
}

void DevConsole::Render(AABB2 const& bounds, Renderer* rendererOverride) const
{
    // Render the console depending on the current mode
    switch (m_mode)
    {
    case OPEN_FULL:
        Render_OpenFull(bounds, *m_config.m_defaultRenderer, *g_theBitmapFont, 1.f);
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

DevConsoleMode DevConsole::GetMode() const
{
    return m_mode;
}

void DevConsole::SetMode(DevConsoleMode const mode)
{
    m_mode = mode;
}

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

bool DevConsole::Command_Test(EventArgs& args)
{
    UNUSED(args)
    AddLine(INFO_MAJOR, "Test command received");
    return false; // Continue calling other subscribers
}

void DevConsole::Render_OpenFull(AABB2 const& bounds, Renderer& renderer, BitmapFont& font, float fontAspect) const
{
    VertexList boxVerts;
    AABB2      box = AABB2(Vec2::ZERO, Vec2(1600.f, 800.f));
    AddVertsForAABB2D(boxVerts, box, Rgba8::TRANSLUCENT_BLACK);
    renderer.BindTexture(nullptr);

    
    renderer.DrawVertexArray(static_cast<int>(boxVerts.size()), boxVerts.data());

    std::vector<Vertex_PCU> textVerts;

    // 計算每行文字的高度
    float lineHeight = 50.f; // 每行的文字高度（可根據需求調整）

    // 計算文字繪製起點（根據控制台界限）
    AABB2 textBounds = bounds;

    // 遍歷所有的行，逐行添加文字
    for (size_t i = 0; i < m_lines.size(); ++i)
    {
        const DevConsoleLine& line = m_lines[i];

        // 每行的文字位置逐漸向上移動
        textBounds.m_maxs.y = bounds.m_maxs.y + (i * lineHeight);
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
        // printf("(%f, %f) (%f, %f)\n", textBounds.m_mins.x, textBounds.m_mins.y, textBounds.m_maxs.x, textBounds.m_maxs.y);
    }

    renderer.BindTexture(&font.GetTexture());
    renderer.DrawVertexArray(static_cast<int>(textVerts.size()), textVerts.data());
}
