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
#include "Engine/Core/Timer.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Game/GameCommon.hpp"

//----------------------------------------------------------------------------------------------------
#if defined ERROR
#undef ERROR
#endif

//----------------------------------------------------------------------------------------------------
DevConsole* g_theDevConsole = nullptr;

//----------------------------------------------------------------------------------------------------
// Static color constants for different message types
STATIC Rgba8 const DevConsole::ERROR                 = Rgba8(255, 0, 0);
STATIC Rgba8 const DevConsole::WARNING               = Rgba8(255, 255, 0);
STATIC Rgba8 const DevConsole::INFO_MAJOR            = Rgba8(0, 255, 0);
STATIC Rgba8 const DevConsole::INFO_MINOR            = Rgba8(0, 255, 255);
STATIC Rgba8 const DevConsole::INPUT_TEXT            = Rgba8(255, 255, 255);
STATIC Rgba8 const DevConsole::INPUT_INSERTION_POINT = Rgba8(255, 255, 255, 200);

//----------------------------------------------------------------------------------------------------
DevConsole::DevConsole(DevConsoleConfig const& config)
    : m_config(config)
{
    AddLine(INFO_MINOR, "Welcome to DevConsole v0.1.0!");

    AddLine(INFO_MAJOR, "Controls");
    AddLine(INFO_MINOR, "(Mouse) Aim");
    AddLine(INFO_MINOR, "(W/A)   Move");
    AddLine(INFO_MINOR, "(S/D)   Strafe");
    AddLine(INFO_MINOR, "(Q/E)   Roll");
    AddLine(INFO_MINOR, "(Z/C)   Elevate");
    AddLine(INFO_MINOR, "(Shift) Sprint");
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
    g_theEventSystem->SubscribeEventCallbackFunction("OnWindowKeyPressed", OnWindowKeyPressed);
    g_theEventSystem->SubscribeEventCallbackFunction("OnWindowCharInput", OnWindowCharInput);
    g_theEventSystem->SubscribeEventCallbackFunction("help", Command_Help);
    g_theEventSystem->SubscribeEventCallbackFunction("clear", Command_Clear);

    m_insertionPointBlinkTimer = new Timer(0.5f);
    Clock::TickSystemClock();
    m_insertionPointBlinkTimer->Start();
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
    std::string arg;

    while (stream >> arg)
    {
        // Parse arguments key=value
        size_t const pos = arg.find('=');

        if (pos != std::string::npos)
        {
            String key         = arg.substr(0, pos);
            String const value = arg.substr(pos + 1);
            args[key]          = value;
        }
    }

    // Echo command if required
    if (echoCommand == true)
    {
        AddLine(INFO_MAJOR, g_theDevConsole->m_inputText);

        Strings registeredEventNames = g_theEventSystem->GetAllRegisteredEventNames();

        bool isCommandValid = std::find(registeredEventNames.begin(), registeredEventNames.end(), command) != registeredEventNames.end();

        if (!isCommandValid)
        {
            AddLine(ERROR, "Your command: '" + command + "' is not valid!");
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
void DevConsole::Render(AABB2 const& bounds, Renderer* rendererOverride)
{
    if (rendererOverride == nullptr)
    {
        rendererOverride = m_config.m_defaultRenderer;
    }

    rendererOverride->BeginCamera(*m_config.m_defaultCamera);

    if (m_insertionPointBlinkTimer->HasPeriodElapsed())
    {
        m_insertionPointVisible = !m_insertionPointVisible;
        m_insertionPointBlinkTimer->DecrementPeriodIfElapsed();
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

    rendererOverride->EndCamera(*m_config.m_defaultCamera);
}

//----------------------------------------------------------------------------------------------------
void DevConsole::PasteFromClipboard()
{
    if (!IsClipboardFormatAvailable(CF_TEXT)) return;
    if (!OpenClipboard(nullptr)) return;
    HANDLE hglb = GetClipboardData(CF_TEXT);
    if (hglb != NULL)
    {
        char* lptstr = static_cast<char*>(GlobalLock(hglb));
        if (lptstr != NULL)
        {
            String clipboardText = lptstr;
            GlobalUnlock(hglb);

            // Insert text at the current insertion point
            m_inputText.insert(m_insertionPointPosition, clipboardText);
            m_insertionPointPosition += static_cast<int>(clipboardText.length());
        }
    }
    CloseClipboard();
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

    if (m_mode == HIDDEN)
    {
        m_isOpen = false;
    }
    else
    {
        m_isOpen = true;
    }
}

//----------------------------------------------------------------------------------------------------
// Toggles between open and closed.
//
void DevConsole::ToggleMode(DevConsoleMode const mode)
{
    if (m_mode == mode)
    {
        m_mode   = HIDDEN; // Hide console if already in that mode
        m_isOpen = false;
    }
    else
    {
        m_mode   = mode;
        m_isOpen = true;
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
STATIC bool DevConsole::OnWindowKeyPressed(EventArgs& args)
{
    int const value             = args.GetValue("OnWindowKeyPressed", -1);
    unsigned char const keyCode = static_cast<unsigned char>(value);

    if (keyCode == KEYCODE_TILDE)
    {
        g_theDevConsole->ToggleMode(OPEN_FULL);
    }

    if (g_theDevConsole->m_isOpen == false)
    {
        return false;
    }

    if (keyCode == KEYCODE_ENTER)
    {
        if (g_theDevConsole->m_inputText.empty() == true)
        {
            g_theDevConsole->SetMode(HIDDEN);
        }
        else
        {
            g_theDevConsole->m_commandHistory.push_back(g_theDevConsole->m_inputText);
            g_theDevConsole->Execute(g_theDevConsole->m_inputText);
            g_theDevConsole->m_inputText.clear();
            g_theDevConsole->m_historyIndex           = -1;
            g_theDevConsole->m_insertionPointPosition = 0;
        }
    }

    if (keyCode == KEYCODE_BACKSPACE &&
        !g_theDevConsole->m_inputText.empty())
    {
        // float const lineWidth = 800.f / g_theDevConsole->m_config.m_maxLinesDisplay;
        //
        // if ((float)g_theDevConsole->m_inputText.size() * lineWidth > 1600.f)
        // {
        //     DebuggerPrintf("m_inputTextPosition: %f\n", g_theDevConsole->m_inputTextPosition);
        //     g_theDevConsole->m_inputTextPosition += lineWidth;
        //     g_theDevConsole->m_insertionPointPosition += 1;
        // }
        if (g_theDevConsole->m_insertionPointPosition == 0)
        {
            return false;
        }

        g_theDevConsole->m_inputText.erase(g_theDevConsole->m_insertionPointPosition - 1, 1);
        g_theDevConsole->m_insertionPointPosition -= 1;
        g_theDevConsole->m_historyIndex = -1;
    }

    if (keyCode == KEYCODE_DELETE &&
        !g_theDevConsole->m_inputText.empty())
    {
        // float const lineWidth = 800.f / g_theDevConsole->m_config.m_maxLinesDisplay;
        //
        // if ((float)g_theDevConsole->m_inputText.size() * lineWidth > 1600.f)
        // {
        //     DebuggerPrintf("m_inputTextPosition: %f\n", g_theDevConsole->m_inputTextPosition);
        //     g_theDevConsole->m_inputTextPosition += lineWidth;
        //     g_theDevConsole->m_insertionPointPosition += 1;
        // }

        g_theDevConsole->m_inputText.erase(g_theDevConsole->m_insertionPointPosition, 1);
        g_theDevConsole->m_historyIndex = -1;
    }

    if (keyCode == KEYCODE_UPARROW)
    {
        if (g_theDevConsole->m_historyIndex + 1 < (int)g_theDevConsole->m_commandHistory.size())
        {
            g_theDevConsole->m_historyIndex += 1;
            g_theDevConsole->m_inputText              = g_theDevConsole->m_commandHistory[g_theDevConsole->m_historyIndex];
            g_theDevConsole->m_insertionPointPosition = (int)g_theDevConsole->m_inputText.size();
        }
    }

    if (keyCode == KEYCODE_DOWNARROW)
    {
        if (g_theDevConsole->m_historyIndex != -1)
        {
            g_theDevConsole->m_historyIndex -= 1;

            if (g_theDevConsole->m_historyIndex == -1)
            {
                g_theDevConsole->m_inputText              = "";
                g_theDevConsole->m_insertionPointPosition = 0;
            }
            else
            {
                g_theDevConsole->m_inputText              = g_theDevConsole->m_commandHistory[g_theDevConsole->m_historyIndex];
                g_theDevConsole->m_insertionPointPosition = (int)g_theDevConsole->m_inputText.size();
            }
        }
    }

    if (keyCode == KEYCODE_LEFTARROW)
    {
        if (g_theDevConsole->m_insertionPointPosition != 0)
        {
            g_theDevConsole->m_insertionPointPosition -= 1;
        }
    }

    if (keyCode == KEYCODE_RIGHTARROW)
    {
        if (g_theDevConsole->m_insertionPointPosition != (int)g_theDevConsole->m_inputText.size())
        {
            g_theDevConsole->m_insertionPointPosition += 1;
        }
    }

    if (keyCode == KEYCODE_HOME)
    {
        g_theDevConsole->m_insertionPointPosition = 0;
    }

    if (keyCode == KEYCODE_END)
    {
        g_theDevConsole->m_insertionPointPosition = (int)g_theDevConsole->m_inputText.size();
    }

    if (keyCode == KEYCODE_ESC)
    {
        if (g_theDevConsole->m_inputText.empty() == true)
        {
            g_theDevConsole->SetMode(HIDDEN);
        }
        else
        {
            g_theDevConsole->m_inputText.clear();
            g_theDevConsole->m_historyIndex           = -1;
            g_theDevConsole->m_insertionPointPosition = 0;
        }
    }

    if (g_theInput->IsKeyDown(KEYCODE_CONTROL) && keyCode == KEYCODE_V)
    {
        DebuggerPrintf("Paste from clipboard\n");
        g_theDevConsole->PasteFromClipboard();
    }

    g_theDevConsole->m_insertionPointBlinkTimer->Start();
    g_theDevConsole->m_insertionPointVisible = true;

    return true;
}

//----------------------------------------------------------------------------------------------------
// Handle char input by appending valid characters to our current input line.
//
STATIC bool DevConsole::OnWindowCharInput(EventArgs& args)
{
    if (g_theDevConsole->m_isOpen == false)
    {
        return false;
    }

    int const value             = args.GetValue("OnWindowCharInput", -1);
    unsigned char const keyCode = static_cast<unsigned char>(value);

    if (keyCode >= 32 &&
        keyCode <= 126 &&
        keyCode != '~' &&
        keyCode != '`')
    {
        float const lineWidth = 800.f / g_theDevConsole->m_config.m_maxLinesDisplay;


        if ((float)g_theDevConsole->m_inputText.size() * lineWidth >= 1600.f)
        {
            return false;
        }

        g_theDevConsole->m_inputText += static_cast<char>(keyCode);
        g_theDevConsole->m_insertionPointPosition += 1;
        g_theDevConsole->m_historyIndex = -1;
        g_theDevConsole->m_insertionPointBlinkTimer->Start();

        // float const lineWidth = 800.f / g_theDevConsole->m_config.m_maxLinesDisplay;
        //
        //
        // if ((float)g_theDevConsole->m_inputText.size() * lineWidth > 1600.f)
        // {
        //     DebuggerPrintf("m_inputTextPosition: %f\n", g_theDevConsole->m_inputTextPosition);
        //     g_theDevConsole->m_inputTextPosition -= lineWidth;
        //     g_theDevConsole->m_insertionPointPosition -= 1;
        //
        // }
    }

    return true;
}

//----------------------------------------------------------------------------------------------------
// Clear all lines of text.
//
STATIC bool DevConsole::Command_Clear(EventArgs& args)
{
    UNUSED(args)

    g_theDevConsole->m_lines.clear();

    return true;
}

//----------------------------------------------------------------------------------------------------
// Display all currently registered commands in the event system.
//
STATIC bool DevConsole::Command_Help(EventArgs& args)
{
    UNUSED(args)

    Strings const registeredEventNames = g_theEventSystem->GetAllRegisteredEventNames();

    for (int i = 0; i < static_cast<int>(registeredEventNames.size()); i++)
    {
        g_theDevConsole->AddLine(INFO_MINOR, registeredEventNames[i]);
    }

    return true;
}

//----------------------------------------------------------------------------------------------------
void DevConsole::Render_OpenFull(AABB2 const& bounds, Renderer& renderer, BitmapFont const& font, float const fontAspect)
{
    // Render background box
    VertexList backgroundBoxVerts;
    AABB2 const backgroundBox = AABB2(Vec2::ZERO, Vec2(1600.f, 800.f));

    AddVertsForAABB2D(backgroundBoxVerts, backgroundBox, Rgba8::TRANSLUCENT_BLACK);
    g_theRenderer->SetBlendMode(BlendMode::ALPHA);

    renderer.BindTexture(nullptr);
    renderer.DrawVertexArray(static_cast<int>(backgroundBoxVerts.size()), backgroundBoxVerts.data());

    VertexList textVerts;

    float const lineHeight = backgroundBox.GetDimensions().y / m_config.m_maxLinesDisplay;

    AABB2 const inputTextBounds = AABB2(Vec2(m_inputTextPosition, 0.f), Vec2(1600.f / lineHeight * static_cast<float>(m_inputText.size()), lineHeight));

    if (m_historyIndex == -1)
    {
        font.AddVertsForTextInBox2D(textVerts, m_inputText, inputTextBounds, lineHeight, INPUT_TEXT, fontAspect);
    }
    else if (m_historyIndex >= 0 && m_historyIndex < static_cast<int>(g_theDevConsole->m_commandHistory.size()))
    {
        AABB2 const commandHistoryTextBounds =
            AABB2(Vec2::ZERO, Vec2(1600.f / lineHeight * static_cast<float>(m_commandHistory[m_historyIndex].size()), lineHeight));

        font.AddVertsForTextInBox2D(textVerts, m_commandHistory[m_historyIndex], commandHistoryTextBounds, lineHeight, Rgba8::WHITE, fontAspect);
    }

    AABB2 commandTextBounds = bounds;

    VertexList insertionPointVerts;
    AABB2 const insertionPointBound = AABB2(commandTextBounds.m_mins + Vec2((float)m_insertionPointPosition * lineHeight, 0.f),
                                            Vec2(5.f, commandTextBounds.m_maxs.y) + Vec2((float)m_insertionPointPosition * lineHeight, 0.f));

    if (m_insertionPointVisible == true)
    {
        AddVertsForAABB2D(insertionPointVerts, insertionPointBound, INPUT_INSERTION_POINT);
        renderer.DrawVertexArray(static_cast<int>(insertionPointVerts.size()), insertionPointVerts.data());
    }

    std::vector<DevConsoleLine> reversedLines = m_lines;
    std::reverse(reversedLines.begin(), reversedLines.end());

    for (size_t i = 0; i < m_lines.size(); ++i)
    {
        commandTextBounds.m_maxs.y = bounds.m_maxs.y + static_cast<float>(i + 1) * lineHeight;
        commandTextBounds.m_mins.y = commandTextBounds.m_maxs.y - lineHeight;

        if (i > (size_t)m_config.m_maxLinesDisplay - 1)
        {
            break;
        }

        DevConsoleLine const& reversedLine = reversedLines[i];

        font.AddVertsForTextInBox2D(
            textVerts,
            reversedLine.m_text,
            commandTextBounds,
            lineHeight,
            reversedLine.m_color,
            fontAspect,
            Vec2::ZERO
        );
    }

    renderer.BindTexture(&font.GetTexture());
    renderer.DrawVertexArray(static_cast<int>(textVerts.size()), textVerts.data());
}
