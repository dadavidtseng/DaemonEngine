//----------------------------------------------------------------------------------------------------
// Window.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Renderer/Window.hpp"

#include <iostream>

#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/AABB2.hpp"

// #define CONSOLE_HANDLER
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

//----------------------------------------------------------------------------------------------------
STATIC Window* Window::s_mainWindow = nullptr;

//----------------------------------------------------------------------------------------------------
Window::Window(WindowConfig const& config)
{
    m_config     = config;
    s_mainWindow = this;
}

//----------------------------------------------------------------------------------------------------
void Window::Startup()
{
#ifdef CONSOLE_HANDLER
    CreateConsole();
#endif

    CreateOSWindow();
}

//----------------------------------------------------------------------------------------------------
void Window::Shutdown()
{
}

//----------------------------------------------------------------------------------------------------
void Window::BeginFrame()
{
    RunMessagePump(); // calls our own WindowsMessageHandlingProcedure() function for us!
}

//----------------------------------------------------------------------------------------------------
void Window::EndFrame()
{
}

//----------------------------------------------------------------------------------------------------
// Handles Windows (Win32) messages/events; i.e. the OS is trying to tell us something happened.
// This function is called back by Windows whenever we tell it to (by calling DispatchMessage).
//
LRESULT CALLBACK WindowsMessageHandlingProcedure(HWND const   windowHandle,
                                                 UINT const   wmMessageCode,
                                                 WPARAM const wParam,
                                                 LPARAM const lParam)
{
    InputSystem* input = nullptr;

    if (Window::s_mainWindow != nullptr &&
        Window::s_mainWindow->GetConfig().m_inputSystem)
    {
        input = Window::s_mainWindow->GetConfig().m_inputSystem;
    }

    switch (wmMessageCode)
    {
    // App close requested via "X" button, or right-click "Close Window" on task bar, or "Close" from system menu, or Alt-F4
    case WM_CLOSE:
        {
            if (g_theDevConsole == nullptr)
            {
                return 0;
            }

            g_theEventSystem->FireEvent("OnCloseButtonClicked");

            return 0; // "Consumes" this message (tells Windows "okay, we handled it")
        }

    // Raw physical keyboard "key-was-just-depressed" event (case-insensitive, not translated)
    case WM_KEYDOWN:
        {
            if (g_theDevConsole == nullptr)
            {
                return 0;
            }

            EventArgs args;
            args.SetValue("OnWindowKeyPressed", Stringf("%d", static_cast<unsigned char>(wParam)));
            FireEvent("OnWindowKeyPressed", args);

            return 0;
        }

    // Raw physical keyboard "key-was-just-released" event (case-insensitive, not translated)
    case WM_KEYUP:
        {
            if (g_theDevConsole == nullptr)
            {
                return 0;
            }

            EventArgs args;
            args.SetValue("OnWindowKeyReleased", Stringf("%d", static_cast<unsigned char>(wParam)));
            FireEvent("OnWindowKeyReleased", args);

            return 0;
        }

    case WM_CHAR:
        {
            if (g_theDevConsole == nullptr)
            {
                return 0;
            }

            EventArgs args;
            args.SetValue("OnWindowCharInput", Stringf("%d", static_cast<unsigned char>(wParam)));
            FireEvent("OnWindowCharInput", args);

            return 0;
        }

    // Mouse left & right button down and up events; treat as a fake keyboard key
    case WM_LBUTTONDOWN:
        {
            if (input != nullptr)
            {
                input->HandleKeyPressed(KEYCODE_LEFT_MOUSE);
            }

            return 0;
        }

    case WM_LBUTTONUP:
        {
            if (input != nullptr)
            {
                input->HandleKeyReleased(KEYCODE_LEFT_MOUSE);
            }

            return 0;
        }

    case WM_RBUTTONDOWN:
        {
            if (input != nullptr)
            {
                input->HandleKeyPressed(KEYCODE_RIGHT_MOUSE);
            }

            return 0;
        }

    case WM_RBUTTONUP:
        {
            if (input != nullptr)
            {
                input->HandleKeyReleased(KEYCODE_RIGHT_MOUSE);
            }

            return 0;
        }

    default: ;
    }


    // Send back to Windows any unhandled/unconsumed messages we want other apps to see (e.g. play/pause in music apps, etc.)
    return DefWindowProc(windowHandle, wmMessageCode, wParam, lParam);
}

//----------------------------------------------------------------------------------------------------
// Processes all Windows messages (WM_xxx) for this app that have queued up since last frame.
// For each message in the queue, our WindowsMessageHandlingProcedure (or "WinProc") function
//	is called, telling us what happened (key up/down, minimized/restored, gained/lost focus, etc.)
//
void Window::RunMessagePump()
{
    MSG queuedMessage;

    for (;;)
    {
        BOOL const wasMessagePresent = PeekMessage(&queuedMessage, nullptr, 0, 0, PM_REMOVE);

        if (!wasMessagePresent)
        {
            break;
        }

        TranslateMessage(&queuedMessage);
        // This tells Windows to call our "WindowsMessageHandlingProcedure" (a.k.a. "WinProc") function
        DispatchMessage(&queuedMessage);
    }
}

//----------------------------------------------------------------------------------------------------
// return a reference that is read only
//
WindowConfig const& Window::GetConfig() const
{
    return m_config;
}

//----------------------------------------------------------------------------------------------------
void* Window::GetDisplayContext() const
{
    return m_displayContext;
}

//----------------------------------------------------------------------------------------------------
void* Window::GetWindowHandle() const
{
    return m_windowHandle;
}

//----------------------------------------------------------------------------------------------------
//	Returns the mouse cursor's current position relative to the interior client area of our
//	window, in normalized UV coordinates -- (0,0) is bottom-left, (1,1) is top-right.
//
Vec2 Window::GetNormalizedMouseUV() const
{
    HWND const windowHandle = static_cast<HWND>(m_windowHandle);
    POINT      cursorCoords;
    RECT       clientRect;

    GetCursorPos(&cursorCoords);	                // in Window screen coordinates; (0,0) is top-left
    ScreenToClient(windowHandle, &cursorCoords);	// get relative to this window's client area
    GetClientRect(windowHandle, &clientRect);	    // dimensions of client area (0,0 to width, height)

    float const cursorX = static_cast<float>(cursorCoords.x) / static_cast<float>(clientRect.right);
    float const cursorY = static_cast<float>(cursorCoords.y) / static_cast<float>(clientRect.bottom);

    return Vec2(cursorX, 1.f - cursorY);	// Flip Y; we want (0,0) bottom-left, not top-left
}

//----------------------------------------------------------------------------------------------------
IntVec2 Window::GetClientDimensions() const
{
    return m_clientDimensions;
}

//----------------------------------------------------------------------------------------------------
void Window::CreateOSWindow()
{
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    HMODULE const applicationInstanceHandle = ::GetModuleHandle(nullptr);
    float const   clientAspect              = m_config.m_aspectRatio;

    // Define a window style/class
    WNDCLASSEX windowClassDescription  = {};
    windowClassDescription.cbSize      = sizeof(windowClassDescription);
    windowClassDescription.style       = CS_OWNDC; // Redraw on move, request own Display Context
    windowClassDescription.lpfnWndProc = static_cast<WNDPROC>(WindowsMessageHandlingProcedure);

    // long pointer of windows proc
    // Register our Windows message-handling function
    windowClassDescription.hInstance     = GetModuleHandle(nullptr);
    windowClassDescription.hIcon         = nullptr;
    windowClassDescription.hCursor       = nullptr;
    windowClassDescription.lpszClassName = TEXT("Simple Window Class");
    RegisterClassEx(&windowClassDescription);

    // #SD1ToDo: Add support for fullscreen mode (requires different window style flags than windowed mode)
    constexpr DWORD windowStyleFlags   = WS_CAPTION | WS_BORDER | WS_SYSMENU | WS_OVERLAPPED;
    constexpr DWORD windowStyleExFlags = WS_EX_APPWINDOW;

    // Get desktop rect, dimensions, aspect
    RECT       desktopRect;
    HWND const desktopWindowHandle = GetDesktopWindow();
    GetClientRect(desktopWindowHandle, &desktopRect);
    float const desktopWidth  = static_cast<float>(desktopRect.right - desktopRect.left);
    float const desktopHeight = static_cast<float>(desktopRect.bottom - desktopRect.top);
    float const desktopAspect = desktopWidth / desktopHeight;

    // Calculate maximum client size (as some % of desktop size)
    constexpr float maxClientFractionOfDesktop = 0.90f;
    float           clientWidth                = desktopWidth * maxClientFractionOfDesktop;
    float           clientHeight               = desktopHeight * maxClientFractionOfDesktop;

    if (clientAspect > desktopAspect)
    {
        // Client window has a wider aspect than desktop; shrink client height to match its width
        clientHeight = clientWidth / clientAspect;
    }
    else
    {
        // Client window has a taller aspect than desktop; shrink client width to match its height
        clientWidth = clientHeight * clientAspect;
    }

    // Calculate client rect bounds by centering the client area
    float const clientMarginX = 0.5f * (desktopWidth - clientWidth);
    float const clientMarginY = 0.5f * (desktopHeight - clientHeight);
    RECT        clientRect;
    clientRect.left   = static_cast<int>(clientMarginX);
    clientRect.right  = clientRect.left + static_cast<int>(clientWidth);
    clientRect.top    = static_cast<int>(clientMarginY);
    clientRect.bottom = clientRect.top + static_cast<int>(clientHeight);

    // Calculate the outer dimensions of the physical window, including frame et al.
    RECT windowRect = clientRect;
    // AdjustWindowRectEx(&windowRect, windowStyleFlags, FALSE, windowStyleExFlags);


    WCHAR windowTitle[1024];
    MultiByteToWideChar(GetACP(),
                        0,
                        m_config.m_windowTitle.c_str(),
                        -1,
                        windowTitle,
                        sizeof(windowTitle) / sizeof(windowTitle[0]));

    m_windowHandle = CreateWindowEx(
        windowStyleExFlags,                   // Extended window style
        windowClassDescription.lpszClassName, // Window class name, here "Simple Window Class"
        windowTitle,                          // Window title
        windowStyleFlags,                     // Window style
        windowRect.left,                      // X-coordinate of the window's top-left corner
        windowRect.top,                       // Y-coordinate of the window's top-left corner
        windowRect.right - windowRect.left,   // Width of the window
        windowRect.bottom - windowRect.top,   // Height of the window
        nullptr,                              // Handle to the parent window (null if no parent)
        nullptr,                              // Handle to the menu (null if no menu)
        applicationInstanceHandle,            // Handle to the application instance
        nullptr                               // Additional parameters passed to WM_CREATE (null if none)
    );

    HWND const windowHandle = static_cast<HWND>(m_windowHandle);

    ShowWindow(windowHandle, SW_SHOW);
    SetForegroundWindow(windowHandle);
    SetFocus(windowHandle);

    m_displayContext = GetDC(windowHandle);

    HCURSOR const cursor = LoadCursor(nullptr, IDC_ARROW);
    SetCursor(cursor);

    m_clientDimensions = IntVec2(static_cast<int>(clientWidth), static_cast<int>(clientHeight));
}

//----------------------------------------------------------------------------------------------------
#ifdef CONSOLE_HANDLER
HANDLE g_consoleHandle = nullptr;

void Window::CreateConsole()
{
    AllocConsole();

    FILE* stream;

    freopen_s(&stream, "CONOUT$", "w", stdout);
    freopen_s(&stream, "CONOUT$", "w", stderr);
    freopen_s(&stream, "CONIN$", "r", stdin);

    g_consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);

    if (g_consoleHandle == INVALID_HANDLE_VALUE)
    {
        std::cerr << "Failed to get console handle!" << '\n';
    }
    else
    {
        printf("[/] Initialize......\n");

        printf("%s", m_config.m_consoleTitle[0].c_str());
        printf("%s", m_config.m_consoleTitle[1].c_str());
        printf("%s", m_config.m_consoleTitle[2].c_str());
        printf("%s", m_config.m_consoleTitle[3].c_str());
        printf("%s", m_config.m_consoleTitle[4].c_str());
        printf("%s", m_config.m_consoleTitle[5].c_str());
        printf("%s", m_config.m_consoleTitle[6].c_str());
        printf("%s", m_config.m_consoleTitle[7].c_str());
        printf("%s", m_config.m_consoleTitle[8].c_str());
        printf("%s", m_config.m_consoleTitle[9].c_str());
        printf("%s", m_config.m_consoleTitle[10].c_str());
        printf("\n");
    }

    if (g_consoleHandle)
    {
        SetConsoleTextAttribute(g_consoleHandle, BACKGROUND_BLUE | FOREGROUND_INTENSITY);
    }
}
#endif
