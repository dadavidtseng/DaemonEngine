//----------------------------------------------------------------------------------------------------
// WindowEx.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Platform/WindowEx.hpp"

#include <iostream>

#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/AABB2.hpp"

// #define CONSOLE_HANDLER
#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#include "Engine/Core/ErrorWarningAssert.hpp"

//----------------------------------------------------------------------------------------------------
// STATIC WindowEx* WindowEx::s_mainWindowEx = nullptr;

//----------------------------------------------------------------------------------------------------
WindowEx::WindowEx(sWindowExConfig const& config)
{
    m_config = config;

    // if (s_mainWindowEx == nullptr)
    // {
    //     s_mainWindowEx = this;
    // }
}

//----------------------------------------------------------------------------------------------------
void WindowEx::Startup()
{
#ifdef CONSOLE_HANDLER
    CreateConsole();
#endif

    // CreateOSWindow();
    CreateOSWindow();
}

//----------------------------------------------------------------------------------------------------
void WindowEx::Shutdown()
{
    ReleaseDC((HWND)m_windowHandle, (HDC)m_displayContext);
}

//----------------------------------------------------------------------------------------------------
void WindowEx::BeginFrame()
{
    RunMessagePump(); // calls our own WindowsMessageHandlingProcedure() function for us!
}

//----------------------------------------------------------------------------------------------------
void WindowEx::EndFrame()
{
}

//----------------------------------------------------------------------------------------------------
// Handles Windows (Win32) messages/events; i.e. the OS is trying to tell us something happened.
// This function is called back by Windows whenever we tell it to (by calling DispatchMessage).
//
LRESULT CALLBACK WindowsExMessageHandlingProcedure(HWND const   windowHandle,
                                                   UINT const   wmMessageCode,
                                                   WPARAM const wParam,
                                                   LPARAM const lParam)
{
    InputSystem* input = nullptr;

    // if (WindowEx::s_mainWindowEx != nullptr &&
    //     WindowEx::s_mainWindowEx->GetConfig().m_inputSystem)
    // {
    //     input = WindowEx::s_mainWindowEx->GetConfig().m_inputSystem;
    // }

    switch (wmMessageCode)
    {
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC         hdc = BeginPaint(windowHandle, &ps);
            // 渲染由 WindowKillRenderer 處理
            EndPaint(windowHandle, &ps);
            return 0;
        }
    case WM_SIZE:
        {
            UINT newWidth  = LOWORD(lParam);
            UINT newHeight = HIWORD(lParam);
            DebuggerPrintf("(%d, %d)\n", newWidth, newHeight);
            EventArgs args;
            args.SetValue("newWidth", Stringf("%d", static_cast<int>(newWidth)));
            args.SetValue("newHeight", Stringf("%d", static_cast<int>(newHeight)));
            FireEvent("OnWindowSizeChanged", args);
            return 0;
        }
    // App close requested via "X" button, or right-click "Close Window" on task bar, or "Close" from system menu, or Alt-F4
    case WM_CLOSE:
        {
            // if (g_theDevConsole == nullptr)
            // {
            //     return 0;
            // }

            g_theEventSystem->FireEvent("OnCloseButtonClicked");

            return 0; // "Consumes" this message (tells Windows "okay, we handled it")
        }

    // Raw physical keyboard "key-was-just-depressed" event (case-insensitive, not translated)
    case WM_KEYDOWN:
        {
            // if (g_theDevConsole == nullptr)
            // {
            //     return 0;
            // }

            EventArgs args;
            args.SetValue("OnWindowKeyPressed", Stringf("%d", static_cast<unsigned char>(wParam)));
            FireEvent("OnWindowKeyPressed", args);

            return 0;
        }

    // Raw physical keyboard "key-was-just-released" event (case-insensitive, not translated)
    case WM_KEYUP:
        {
            // if (g_theDevConsole == nullptr)
            // {
            //     return 0;
            // }

            EventArgs args;
            args.SetValue("OnWindowKeyReleased", Stringf("%d", static_cast<unsigned char>(wParam)));
            FireEvent("OnWindowKeyReleased", args);

            return 0;
        }

    case WM_CHAR:
        {
            // if (g_theDevConsole == nullptr)
            // {
            //     return 0;
            // }

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
void WindowEx::RunMessagePump()
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
sWindowExConfig const& WindowEx::GetConfig() const
{
    return m_config;
}

//----------------------------------------------------------------------------------------------------
void* WindowEx::GetDisplayContext() const
{
    return m_displayContext;
}

//----------------------------------------------------------------------------------------------------
void* WindowEx::GetWindowHandle() const
{
    return m_windowHandle;
}

//----------------------------------------------------------------------------------------------------
//	Returns the mouse cursor's current position relative to the interior client area of our
//	window, in normalized UV coordinates -- (0,0) is bottom-left, (1,1) is top-right.
//
Vec2 WindowEx::GetNormalizedMouseUV() const
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
IntVec2 WindowEx::GetClientDimensions() const
{
    // RECT       desktopRect;
    // HWND const desktopWindowHandle = GetDesktopWindow();
    // GetClientRect(desktopWindowHandle, &desktopRect);
    // float const desktopWidth  = static_cast<float>(desktopRect.right - desktopRect.left);
    // float const desktopHeight = static_cast<float>(desktopRect.bottom - desktopRect.top);
    // float const desktopAspect = desktopWidth / desktopHeight;
    // return IntVec2(desktopWidth, desktopHeight);
    return m_clientDimensions;
}

bool WindowEx::IsWindowClassRegistered()
{
    WNDCLASSEX wc = {};
    return GetClassInfoEx(GetModuleHandle(nullptr), L"GameWindow", &wc) != 0;
}

WindowEx* WindowEx::CreateChildWindow(wchar_t const* title, int x, int y, int width, int height)
{
    HINSTANCE hInst = GetModuleHandle(NULL);
    WNDCLASS      wc                        = {};
    wc.lpfnWndProc                          = WindowsExMessageHandlingProcedure;
    wc.hInstance                            = hInst;
    wc.lpszClassName                        = L"GameWindow";
    wc.hbrBackground                        = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor                              = LoadCursor(nullptr, IDC_ARROW);

    RegisterClass(&wc);

    // 首先確保窗口類別已註冊
    if (!IsWindowClassRegistered())
    {
        DebuggerPrintf("Error: The window is not registered\n");
        // return nullptr;
    }

    HWND hwnd = CreateWindowEx(
        0,
        L"GameWindow",
        title,
        WS_OVERLAPPEDWINDOW,
        x, y, width, height,
        nullptr,
        nullptr,
        hInst,
        nullptr
    );

    if (!hwnd)
    {
        DWORD error = GetLastError();
        printf("錯誤：CreateWindowEx 失敗，錯誤碼：%lu\n", error);
        // return nullptr;
    }

    if (hwnd)
    {
        ShowWindow(hwnd, SW_SHOW);
        UpdateWindow(hwnd);

        // WindowEx* childWindow       = new WindowEx();
        // childWindow->m_windowHandle = hwnd;
        //
        // printf("成功：子視窗 HWND 創建成功 - %p\n", hwnd);
        // return childWindow; // 返回新物件
    }
    printf("成功：子視窗 HWND 創建成功 - %p\n", hwnd);
    return this;
}

void WindowEx::UpdateWindowPosition(int const sceneWidth,
                                    int const sceneHeight,
                                    int const virtualScreenWidth,
                                    int const virtualScreenHeight)
{
    RECT windowRect;
    GetWindowRect((HWND)m_windowHandle, &windowRect);

    // if (memcmp(&windowRect, &lastRect, sizeof(sRECT)) != 0)
    // {
    lastRect.bottom = windowRect.bottom;
    lastRect.left   = windowRect.left;
    lastRect.right  = windowRect.right;
    lastRect.top    = windowRect.top;

    needsUpdate = true;

    RECT clientRect;
    GetClientRect((HWND)m_windowHandle, &clientRect);
    width  = clientRect.right - clientRect.left;
    height = clientRect.bottom - clientRect.top;

    viewportX      = (float)windowRect.left / (float)virtualScreenWidth;
    viewportY      = (float)windowRect.top / (float)virtualScreenHeight;
    viewportWidth  = (float)width / (float)virtualScreenWidth;
    viewportHeight = (float)height / (float)virtualScreenHeight;

    // 確保座標對齊到像素邊界
    float pixelAlignX = 1.0f / (float)sceneWidth;
    float pixelAlignY = 1.0f / (float)sceneHeight;

    viewportX      = floor(viewportX / pixelAlignX) * pixelAlignX;
    viewportY      = floor(viewportY / pixelAlignY) * pixelAlignY;
    viewportWidth  = ceil(viewportWidth / pixelAlignX) * pixelAlignX;
    viewportHeight = ceil(viewportHeight / pixelAlignY) * pixelAlignY;

    viewportX      = max(0.0f, min(1.0f,viewportX));
    viewportY      = max(0.0f, min(1.0f, viewportY));
    viewportWidth  = max(0.0f, min(1.0f - viewportX, viewportWidth));
    viewportHeight = max(0.0f, min(1.0f - viewportY, viewportHeight));
    // }
}

//----------------------------------------------------------------------------------------------------
void WindowEx::CreateOSWindow()
{
    // Sets the current process to a specified dots per inch (dpi) awareness context.
    // The DPI awareness contexts are from the DPI_AWARENESS_CONTEXT value.
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    // Define a window style/class
    WNDCLASSEX windowClassEx  = {};                                                         // Contains window class information. It is used with the `RegisterClassEx` and `GetClassInfoEx` functions.
    windowClassEx.cbSize      = sizeof(windowClassEx);                                      // The size, in bytes, of this structure. Set this member to sizeof(WNDCLASSEX). Be sure to set this member before calling the GetClassInfoEx function.
    windowClassEx.style       = CS_OWNDC;                                                   // The class style(s). This member can be any combination of the Class Styles. https://learn.microsoft.com/en-us/windows/win32/winmsg/window-class-styles
    windowClassEx.lpfnWndProc = static_cast<WNDPROC>(WindowsExMessageHandlingProcedure);      // Long Pointer to the Windows Procedure function.

    // Register our Windows message-handling function
    windowClassEx.hInstance = GetModuleHandle(nullptr);                                 // A handle to the instance that contains the window procedure for the class.
    windowClassEx.hIcon     = (HICON)LoadImage(
        NULL,                  // hInstance = NULL 表示從檔案
        m_config.m_iconFilePath,         // 檔案路徑
        IMAGE_ICON,
        32, 32,                // 大小 (通常是 32x32)
        LR_LOADFROMFILE
    );
    windowClassEx.hCursor       = nullptr;
    windowClassEx.lpszClassName = L"GameWindow";
    RegisterClassEx(&windowClassEx);
    // #SD1ToDo: Add support for fullscreen mode (requires different window style flags than windowed mode)
    DWORD constexpr windowStyleFlags   = WS_CAPTION | WS_POPUP | WS_SYSMENU | WS_OVERLAPPEDWINDOW;
    DWORD constexpr windowStyleExFlags = WS_EX_APPWINDOW;
    // DWORD constexpr windowStyleFlags   = WS_POPUP;
    // DWORD constexpr windowStyleExFlags = WS_EX_APPWINDOW;

    // Get desktop rect, dimensions, aspect
    RECT       desktopRect;
    HWND const desktopWindowHandle = GetDesktopWindow();
    GetClientRect(desktopWindowHandle, &desktopRect);
    float const desktopWidth  = static_cast<float>(desktopRect.right - desktopRect.left);
    float const desktopHeight = static_cast<float>(desktopRect.bottom - desktopRect.top);
    float const desktopAspect = desktopWidth / desktopHeight;

    // Calculate maximum client size (as some % of desktop size)
    float constexpr maxClientFractionOfDesktop = 0.90f;
    float const     clientAspect               = m_config.m_aspectRatio;
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
    // RECT clientRect;
    // clientRect.left   = 0;
    // clientRect.top    = 0;
    // clientRect.right  = static_cast<LONG>(desktopWidth );
    // clientRect.bottom = static_cast<LONG>(desktopHeight);

    // Calculate the outer dimensions of the physical window, including frame et al.
    RECT windowRect = clientRect;
    AdjustWindowRectEx(&windowRect, windowStyleFlags, FALSE, windowStyleExFlags);

    WCHAR windowTitle[1024];
    MultiByteToWideChar(GetACP(),
                        0,
                        m_config.m_windowTitle.c_str(),
                        -1,
                        windowTitle,
                        sizeof(windowTitle) / sizeof(windowTitle[0]));

    HMODULE const applicationInstanceHandle = GetModuleHandle(nullptr);
    m_windowHandle                          = CreateWindowEx(
        windowStyleExFlags,                     // Extended window style
        windowClassEx.lpszClassName,            // Window class name, here "Simple Window Class"
        windowTitle,                            // Window title
        windowStyleFlags,                       // Window style
        windowRect.left,                        // X-coordinate of the window's top-left corner
        windowRect.top,                         // Y-coordinate of the window's top-left corner
        windowRect.right - windowRect.left,     // Width of the window
        windowRect.bottom - windowRect.top,     // Height of the window
        nullptr,                                // Handle to the parent window (null if no parent)
        nullptr,                                // Handle to the menu (null if no menu)
        applicationInstanceHandle,              // Handle to the application instance
        nullptr                                 // Additional parameters passed to WM_CREATE (null if none)
    );

    HWND const windowHandle = static_cast<HWND>(m_windowHandle);

    ShowWindow(windowHandle, SW_SHOW);
    // SetWindowLong((HWND)m_windowHandle, GWL_STYLE, windowStyleFlags);
    // SetWindowLong((HWND)m_windowHandle, GWL_EXSTYLE, windowStyleExFlags);

    // Optional: 塞滿畫面（如果你想像 fullscreen 一樣）
    // SetWindowPos((HWND)m_windowHandle, HWND_TOP, 0, 0, desktopWidth, desktopHeight, SWP_FRAMECHANGED);
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
