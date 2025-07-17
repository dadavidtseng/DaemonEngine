//----------------------------------------------------------------------------------------------------
// Window.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Platform/Window.hpp"

#include <iostream>

#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/MathUtils.hpp"

// #define CONSOLE_HANDLER
#define WIN32_LEAN_AND_MEAN

#include <chrono>
#include <dxgi1_2.h>
#include <windows.h>


//----------------------------------------------------------------------------------------------------
STATIC Window* Window::s_mainWindow = nullptr;

//----------------------------------------------------------------------------------------------------
Window::Window(sWindowConfig config)
    : m_config(std::move(config))
{
    if (s_mainWindow == nullptr)
    {
        s_mainWindow = this;
    }
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
    DX_SAFE_RELEASE(m_swapChain)
    DX_SAFE_RELEASE(m_renderTargetView)
    ShowWindow((HWND)m_windowHandle, SW_HIDE);
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
void Window::RunMessagePump() const
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
sWindowConfig const& Window::GetConfig() const
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

// //----------------------------------------------------------------------------------------------------
// //	Returns the mouse cursor's current position relative to the interior client area of our
// //	window, in normalized UV coordinates -- (0,0) is bottom-left, (1,1) is top-right.
// //
// Vec2 Window::GetNormalizedMouseUV() const
// {
//     HWND const windowHandle = static_cast<HWND>(m_windowHandle);
//     POINT      cursorCoords;
//     RECT       clientRect;
//
//     GetCursorPos(&cursorCoords);	                // in Window screen coordinates; (0,0) is top-left
//     ScreenToClient(windowHandle, &cursorCoords);	// get relative to this window's client area
//     GetClientRect(windowHandle, &clientRect);	    // dimensions of client area (0,0 to width, height)
//
//     float const cursorX = static_cast<float>(cursorCoords.x) / static_cast<float>(clientRect.right);
//     float const cursorY = static_cast<float>(cursorCoords.y) / static_cast<float>(clientRect.bottom);
//
//     return Vec2(cursorX, 1.f - cursorY);	// Flip Y; we want (0,0) bottom-left, not top-left
// }

//----------------------------------------------------------------------------------------------------
Vec2 Window::GetClientDimensions() const
{
    // RECT       desktopRect;
    // HWND const desktopWindowHandle = GetDesktopWindow();
    // GetClientRect(desktopWindowHandle, &desktopRect);
    // float const desktopWidth  = static_cast<float>(desktopRect.right - desktopRect.left);
    // float const desktopHeight = static_cast<float>(desktopRect.bottom - desktopRect.top);
    // float const desktopAspect = desktopWidth / desktopHeight;
    // return Vec2(desktopWidth, desktopHeight);
    return m_clientDimensions;
}

Vec2 Window::GetClientPosition() const
{
    return m_clientPosition;
}

void Window::SetClientDimensions(Vec2 const& newDimensions)
{
    m_clientDimensions = newDimensions;

    // 計算所需的視窗大小來達到指定的client大小
    RECT desiredClientRect = {0, 0, (int)newDimensions.x, (int)newDimensions.y};

    // 取得當前視窗樣式
    DWORD windowStyle = GetWindowLong((HWND)m_windowHandle, GWL_STYLE);
    DWORD windowExStyle = GetWindowLong((HWND)m_windowHandle, GWL_EXSTYLE);

    // 計算達到此client大小所需的視窗大小
    AdjustWindowRectEx(&desiredClientRect, windowStyle, FALSE, windowExStyle);

    // 計算新的視窗尺寸
    int newWindowWidth = desiredClientRect.right - desiredClientRect.left;
    int newWindowHeight = desiredClientRect.bottom - desiredClientRect.top;

    // 保持當前位置，只改變大小
    RECT currentWindowRect;
    GetWindowRect((HWND)m_windowHandle, &currentWindowRect);

    SetWindowPos((HWND)m_windowHandle, nullptr,
                 currentWindowRect.left, currentWindowRect.top,
                 newWindowWidth, newWindowHeight,
                 SWP_NOZORDER | SWP_NOACTIVATE);

    // 更新視窗尺寸記錄
    m_windowDimensions.x = newWindowWidth;
    m_windowDimensions.y = newWindowHeight;

    // 更新viewport dimensions (通常與client dimensions相同)
    m_viewportDimensions = newDimensions;

    // 標記需要更新
    m_shouldUpdateDimension = true;
}

void Window::SetClientPosition(Vec2 const& newPosition)
{
    m_clientPosition = newPosition;

    // 計算對應的 window position
    Vec2 borderOffset = GetBorderOffset();
    m_windowPosition  = newPosition - borderOffset;

    // 移動視窗
    SetWindowPos((HWND)m_windowHandle, nullptr,
                 (int)m_windowPosition.x, (int)m_windowPosition.y, 0, 0,
                 SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

    // 更新 viewport position
    m_viewportPosition.x = newPosition.x / m_screenDimensions.x;
    m_viewportPosition.y = newPosition.y / m_screenDimensions.y;

    m_shouldUpdatePosition = true;
}

Vec2 Window::GetWindowPosition() const
{
    return m_windowPosition;
}

Vec2 Window::GetWindowDimensions() const
{
    return m_windowDimensions;
}

void Window::SetWindowDimensions(Vec2 const& newDimensions)
{
    m_windowDimensions = newDimensions;

    // 保持當前位置，只改變大小
    RECT currentWindowRect;
    GetWindowRect((HWND)m_windowHandle, &currentWindowRect);

    // 使用當前的左上角位置，設定新的大小
    SetWindowPos((HWND)m_windowHandle, nullptr,
                 currentWindowRect.left, currentWindowRect.top,
                 (int)newDimensions.x, (int)newDimensions.y,
                 SWP_NOZORDER | SWP_NOACTIVATE);

    // 更新client dimensions (會因為邊框而有所不同)
    RECT newClientRect;
    GetClientRect((HWND)m_windowHandle, &newClientRect);
    m_clientDimensions.x = newClientRect.right - newClientRect.left;
    m_clientDimensions.y = newClientRect.bottom - newClientRect.top;

    // 更新viewport dimensions (通常與client dimensions相同，除非有letterbox等模式)
    m_viewportDimensions = m_clientDimensions;

    // 標記需要更新
    m_shouldUpdateDimension = true;
}

void Window::SetWindowPosition(Vec2 const& newPosition)
{
    m_windowPosition = newPosition;  // 儲存引擎座標

    // 轉換為Windows座標並移動視窗
    Vec2 windowsPosition = EngineToWindowsCoords(newPosition);
    SetWindowPos((HWND)m_windowHandle, nullptr,
                 (int)windowsPosition.x, (int)windowsPosition.y, 0, 0,
                 SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

    // 更新相關座標（都用引擎座標儲存）
    Vec2 borderOffset = GetBorderOffset();
    // 注意：borderOffset也需要進行Y軸調整
    m_clientPosition     = newPosition + Vec2(borderOffset.x, -borderOffset.y);
    m_viewportPosition.x = m_clientPosition.x / m_screenDimensions.x;
    m_viewportPosition.y = m_clientPosition.y / m_screenDimensions.y;

    m_shouldUpdatePosition = true;
}

// 引擎座標 -> Windows座標 (Y軸翻轉)
Vec2 Window::EngineToWindowsCoords(Vec2 const& engineCoords) const
{
    float windowsX = engineCoords.x;
    float windowsY = m_screenDimensions.y - engineCoords.y - m_windowDimensions.y;
    return Vec2(windowsX, windowsY);
}

// Windows座標 -> 引擎座標 (Y軸翻轉)
Vec2 Window::WindowsToEngineCoords(Vec2 const& windowsCoords) const
{
     // Windows座標的左上角 -> 引擎座標的左下角
        float engineX = windowsCoords.x;
        float engineY = m_screenDimensions.y - windowsCoords.y - m_windowDimensions.y;
        return Vec2(engineX, engineY);
}

//----------------------------------------------------------------------------------------------------
// void Window::CreateOSWindow()
// {
//     // Sets the current process to a specified dots per inch (dpi) awareness context.
//     // The DPI awareness contexts are from the DPI_AWARENESS_CONTEXT value.
//     SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
//
//     // Define a window style/class
//     WNDCLASSEX windowClassEx  = {};                                                         // Contains window class information. It is used with the `RegisterClassEx` and `GetClassInfoEx` functions.
//     windowClassEx.cbSize      = sizeof(windowClassEx);                                      // The size, in bytes, of this structure. Set this member to sizeof(WNDCLASSEX). Be sure to set this member before calling the GetClassInfoEx function.
//     windowClassEx.style       = CS_OWNDC;                                                   // The class style(s). This member can be any combination of the Class Styles. https://learn.microsoft.com/en-us/windows/win32/winmsg/window-class-styles
//     windowClassEx.lpfnWndProc = static_cast<WNDPROC>(WindowsMessageHandlingProcedure);      // Long Pointer to the Windows Procedure function.
//
//     // Register our Windows message-handling function
//     windowClassEx.hInstance = GetModuleHandle(nullptr);                                 // A handle to the instance that contains the window procedure for the class.
//     windowClassEx.hIcon     = (HICON)LoadImage(
//         NULL,                  // hInstance = NULL 表示從檔案
//         m_config.m_iconFilePath,         // 檔案路徑
//         IMAGE_ICON,
//         32, 32,                // 大小 (通常是 32x32)
//         LR_LOADFROMFILE
//     );
//     windowClassEx.hCursor       = nullptr;
//     windowClassEx.lpszClassName = TEXT("Simple Window Class");
//     RegisterClassEx(&windowClassEx);
//     // #SD1ToDo: Add support for fullscreen mode (requires different window style flags than windowed mode)
//     DWORD constexpr windowStyleFlags   = WS_CAPTION | WS_POPUP | WS_SYSMENU | WS_OVERLAPPED;
//     DWORD constexpr windowStyleExFlags = WS_EX_APPWINDOW;
//     // DWORD constexpr windowStyleFlags   = WS_POPUP;
//     // DWORD constexpr windowStyleExFlags = WS_EX_APPWINDOW;
//
//     // Get desktop rect, dimensions, aspect
//     RECT       desktopRect;
//     HWND const desktopWindowHandle = GetDesktopWindow();
//     GetClientRect(desktopWindowHandle, &desktopRect);
//     float const desktopWidth  = static_cast<float>(desktopRect.right - desktopRect.left);
//     float const desktopHeight = static_cast<float>(desktopRect.bottom - desktopRect.top);
//     float const desktopAspect = desktopWidth / desktopHeight;
//
//     // Calculate maximum client size (as some % of desktop size)
//     float constexpr maxClientFractionOfDesktop = 0.90f;
//     float const     clientAspect               = m_config.m_aspectRatio;
//     float           clientWidth                = desktopWidth * maxClientFractionOfDesktop;
//     float           clientHeight               = desktopHeight * maxClientFractionOfDesktop;
//
//     if (clientAspect > desktopAspect)
//     {
//         // Client window has a wider aspect than desktop; shrink client height to match its width
//         clientHeight = clientWidth / clientAspect;
//     }
//     else
//     {
//         // Client window has a taller aspect than desktop; shrink client width to match its height
//         clientWidth = clientHeight * clientAspect;
//     }
//
//     // Calculate client rect bounds by centering the client area
//     float const clientMarginX = 0.5f * (desktopWidth - clientWidth);
//     float const clientMarginY = 0.5f * (desktopHeight - clientHeight);
//     RECT        clientRect;
//     clientRect.left   = static_cast<int>(clientMarginX);
//     clientRect.right  = clientRect.left + static_cast<int>(clientWidth);
//     clientRect.top    = static_cast<int>(clientMarginY);
//     clientRect.bottom = clientRect.top + static_cast<int>(clientHeight);
//     // RECT clientRect;
//     // clientRect.left   = 0;
//     // clientRect.top    = 0;
//     // clientRect.right  = static_cast<LONG>(desktopWidth );
//     // clientRect.bottom = static_cast<LONG>(desktopHeight);
//
//     // Calculate the outer dimensions of the physical window, including frame et al.
//     RECT windowRect = clientRect;
//     AdjustWindowRectEx(&windowRect, windowStyleFlags, FALSE, windowStyleExFlags);
//
//     WCHAR windowTitle[1024];
//     MultiByteToWideChar(GetACP(),
//                         0,
//                         m_config.m_windowTitle.c_str(),
//                         -1,
//                         windowTitle,
//                         sizeof(windowTitle) / sizeof(windowTitle[0]));
//
//     HMODULE const applicationInstanceHandle = GetModuleHandle(nullptr);
//     m_windowHandle                          = CreateWindowEx(
//         windowStyleExFlags,                     // Extended window style
//         windowClassEx.lpszClassName,            // Window class name, here "Simple Window Class"
//         windowTitle,                            // Window title
//         windowStyleFlags,                       // Window style
//         windowRect.left,                        // X-coordinate of the window's top-left corner
//         windowRect.top,                         // Y-coordinate of the window's top-left corner
//         windowRect.right - windowRect.left,     // Width of the window
//         windowRect.bottom - windowRect.top,     // Height of the window
//         nullptr,                                // Handle to the parent window (null if no parent)
//         nullptr,                                // Handle to the menu (null if no menu)
//         applicationInstanceHandle,              // Handle to the application instance
//         nullptr                                 // Additional parameters passed to WM_CREATE (null if none)
//     );
//
//     HWND const windowHandle = static_cast<HWND>(m_windowHandle);
//
//     ShowWindow(windowHandle, SW_SHOW);
//     // SetWindowLong((HWND)m_windowHandle, GWL_STYLE, windowStyleFlags);
//     // SetWindowLong((HWND)m_windowHandle, GWL_EXSTYLE, windowStyleExFlags);
//
//     // Optional: 塞滿畫面（如果你想像 fullscreen 一樣）
//     // SetWindowPos((HWND)m_windowHandle, HWND_TOP, 0, 0, desktopWidth, desktopHeight, SWP_FRAMECHANGED);
//     SetForegroundWindow(windowHandle);
//     SetFocus(windowHandle);
//
//     m_displayContext = GetDC(windowHandle);
//
//     HCURSOR const cursor = LoadCursor(nullptr, IDC_ARROW);
//     SetCursor(cursor);
//
//     m_clientDimensions = IntVec2(static_cast<int>(clientWidth), static_cast<int>(clientHeight));
// }

//----------------------------------------------------------------------------------------------------
void Window::CreateOSWindow()
{
    // Sets the current process to a specified dots per inch (dpi) awareness context.
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    // Define a window style/class
    WNDCLASSEX windowClassEx  = {};
    windowClassEx.cbSize      = sizeof(windowClassEx);
    windowClassEx.style       = CS_OWNDC;
    windowClassEx.lpfnWndProc = static_cast<WNDPROC>(WindowsMessageHandlingProcedure);
    windowClassEx.hInstance   = GetModuleHandle(nullptr);
    windowClassEx.hIcon       = (HICON)LoadImage(
        NULL,
        m_config.m_iconFilePath,
        IMAGE_ICON,
        32, 32,
        LR_LOADFROMFILE
    );
    windowClassEx.hCursor       = nullptr;
    windowClassEx.lpszClassName = TEXT("Simple Window Class");
    RegisterClassEx(&windowClassEx);

    // Get desktop dimensions (screen size)
    RECT       desktopRect;
    HWND const desktopWindowHandle = GetDesktopWindow();
    GetClientRect(desktopWindowHandle, &desktopRect);
    int const   desktopWidth  = desktopRect.right - desktopRect.left;
    int const   desktopHeight = desktopRect.bottom - desktopRect.top;
    float const desktopAspect = static_cast<float>(desktopWidth) / static_cast<float>(desktopHeight);

    // Determine window style and dimensions based on window type
    DWORD windowStyleFlags   = 0;
    DWORD windowStyleExFlags = WS_EX_APPWINDOW;
    RECT  clientRect         = {};

    switch (m_config.m_windowType)
    {
    case eWindowType::WINDOWED:
        {
            windowStyleFlags = WS_CAPTION | WS_SYSMENU | WS_OVERLAPPED | WS_MINIMIZEBOX;

            // Calculate windowed size (90% of desktop, maintaining aspect ratio)
            float constexpr maxClientFractionOfDesktop = 0.9f;
            float const     clientAspect               = m_config.m_aspectRatio;
            float           clientWidth                = static_cast<float>(desktopWidth) * maxClientFractionOfDesktop;
            float           clientHeight               = static_cast<float>(desktopHeight) * maxClientFractionOfDesktop;

            if (clientAspect > desktopAspect)
            {
                clientHeight = clientWidth / clientAspect;
            }
            else
            {
                clientWidth = clientHeight * clientAspect;
            }

            // Center the window
            float const clientMarginX = 0.5f * (static_cast<float>(desktopWidth) - clientWidth);
            float const clientMarginY = 0.5f * (static_cast<float>(desktopHeight) - clientHeight);

            clientRect.left   = static_cast<int>(clientMarginX);
            clientRect.right  = clientRect.left + static_cast<int>(clientWidth);
            clientRect.top    = static_cast<int>(clientMarginY);
            clientRect.bottom = clientRect.top + static_cast<int>(clientHeight);

            m_clientDimensions = Vec2(static_cast<int>(clientWidth), static_cast<int>(clientHeight));
            m_clientPosition   = Vec2(static_cast<int>(clientRect.bottom), static_cast<int>(clientRect.left));
        }
        break;

    case eWindowType::BORDERLESS:
        {
            windowStyleFlags = WS_POPUP;

            // Same size calculation as windowed but without borders
            float constexpr maxClientFractionOfDesktop = 0.90f;
            float const     clientAspect               = m_config.m_aspectRatio;
            float           clientWidth                = static_cast<float>(desktopWidth) * maxClientFractionOfDesktop;
            float           clientHeight               = static_cast<float>(desktopHeight) * maxClientFractionOfDesktop;

            if (clientAspect > desktopAspect)
            {
                clientHeight = clientWidth / clientAspect;
            }
            else
            {
                clientWidth = clientHeight * clientAspect;
            }

            float const clientMarginX = 0.5f * (static_cast<float>(desktopWidth) - clientWidth);
            float const clientMarginY = 0.5f * (static_cast<float>(desktopHeight) - clientHeight);

            clientRect.left   = static_cast<int>(clientMarginX);
            clientRect.right  = clientRect.left + static_cast<int>(clientWidth);
            clientRect.top    = static_cast<int>(clientMarginY);
            clientRect.bottom = clientRect.top + static_cast<int>(clientHeight);

            m_clientDimensions = Vec2(static_cast<int>(clientWidth), static_cast<int>(clientHeight));
            m_clientPosition   = Vec2(static_cast<int>(clientRect.bottom), static_cast<int>(clientRect.left));
        }
        break;

    case eWindowType::FULLSCREEN_STRETCH:
        {
            windowStyleFlags   = WS_POPUP;
            windowStyleExFlags = WS_EX_APPWINDOW | WS_EX_TOPMOST;

            // Fill entire screen
            clientRect.left   = 0;
            clientRect.top    = 0;
            clientRect.right  = desktopWidth;
            clientRect.bottom = desktopHeight;

            m_clientDimensions = Vec2(desktopWidth, desktopHeight);
        }
        break;

    case eWindowType::FULLSCREEN_LETTERBOX:
        {
            windowStyleFlags   = WS_POPUP;
            windowStyleExFlags = WS_EX_APPWINDOW | WS_EX_TOPMOST;

            // Calculate letterbox dimensions to maintain aspect ratio
            float const targetAspect = m_config.m_aspectRatio;
            int         clientWidth, clientHeight;

            if (targetAspect > desktopAspect)
            {
                // Fit to width, letterbox top/bottom
                clientWidth  = desktopWidth;
                clientHeight = static_cast<int>(static_cast<float>(desktopWidth) / targetAspect);
            }
            else
            {
                // Fit to height, letterbox left/right
                clientHeight = desktopHeight;
                clientWidth  = static_cast<int>(static_cast<float>(desktopHeight) * targetAspect);
            }

            // Center the content area
            int const offsetX = (desktopWidth - clientWidth) / 2;
            int const offsetY = (desktopHeight - clientHeight) / 2;

            clientRect.left   = offsetX;
            clientRect.top    = offsetY;
            clientRect.right  = offsetX + clientWidth;
            clientRect.bottom = offsetY + clientHeight;

            // Store the actual screen dimensions for rendering
            m_clientDimensions   = Vec2(desktopWidth, desktopHeight);
            m_viewportDimensions = Vec2(clientWidth, clientHeight);
            m_viewportOffset     = Vec2(offsetX, offsetY);
        }
        break;

    case eWindowType::FULLSCREEN_CROP:
        {
            windowStyleFlags   = WS_POPUP;
            windowStyleExFlags = WS_EX_APPWINDOW;

            // Fill screen and crop to maintain aspect ratio
            float const targetAspect = m_config.m_aspectRatio;
            int         clientWidth, clientHeight;

            if (targetAspect > desktopAspect)
            {
                // Fit to height, crop left/right
                clientHeight = desktopHeight;
                clientWidth  = static_cast<int>(static_cast<float>(desktopHeight) * targetAspect);
            }
            else
            {
                // Fit to width, crop top/bottom
                clientWidth  = desktopWidth;
                clientHeight = static_cast<int>(static_cast<float>(desktopWidth) / targetAspect);
            }
            DebuggerPrintf(Stringf("ClientWidth = %d | ClientHeight = %d", clientWidth, clientHeight).c_str());
            // Center the viewport
            int const offsetX = (desktopWidth - clientWidth) / 2;
            int const offsetY = (desktopHeight - clientHeight) / 2;

            clientRect.left   = 0;
            clientRect.top    = 0;
            clientRect.right  = desktopWidth;
            clientRect.bottom = desktopHeight;

            // Store rendering information
            m_clientDimensions   = Vec2(desktopWidth, desktopHeight);
            m_clientPosition     = Vec2(static_cast<int>(clientRect.bottom), static_cast<int>(clientRect.left));
            m_viewportDimensions = Vec2(clientWidth, clientHeight);
            m_viewportOffset     = Vec2(offsetX, offsetY);
        }
        break;

    case eWindowType::MINIMIZED:
        {
            windowStyleFlags = WS_CAPTION | WS_SYSMENU | WS_OVERLAPPED | WS_MINIMIZEBOX;

            // Create a normal window first, then minimize it
            float const clientWidth  = 800.0f;
            float const clientHeight = clientWidth / m_config.m_aspectRatio;

            clientRect.left   = 100;
            clientRect.top    = 100;
            clientRect.right  = clientRect.left + static_cast<int>(clientWidth);
            clientRect.bottom = clientRect.top + static_cast<int>(clientHeight);

            m_clientDimensions = Vec2(static_cast<int>(clientWidth), static_cast<int>(clientHeight));
        }
        break;

    case eWindowType::HIDDEN:
        {
            windowStyleFlags = WS_CAPTION | WS_SYSMENU | WS_OVERLAPPED;

            // Create a normal window but don't show it
            float const clientWidth  = 800.0f;
            float const clientHeight = clientWidth / m_config.m_aspectRatio;

            clientRect.left   = 100;
            clientRect.top    = 100;
            clientRect.right  = clientRect.left + static_cast<int>(clientWidth);
            clientRect.bottom = clientRect.top + static_cast<int>(clientHeight);

            m_clientDimensions = Vec2(static_cast<int>(clientWidth), static_cast<int>(clientHeight));
        }
        break;

    default:
        // Default to windowed mode
        m_config.m_windowType = eWindowType::WINDOWED;
        windowStyleFlags = WS_CAPTION | WS_SYSMENU | WS_OVERLAPPED | WS_MINIMIZEBOX;

        float const clientWidth  = 1600.0f;
        float const clientHeight = 800.0f;

        clientRect.left   = 100;
        clientRect.top    = 100;
        clientRect.right  = clientRect.left + static_cast<int>(clientWidth);
        clientRect.bottom = clientRect.top + static_cast<int>(clientHeight);

        m_clientDimensions = Vec2(static_cast<int>(clientWidth), static_cast<int>(clientHeight));
        break;

        break;
    }

    // Calculate the outer dimensions of the physical window, including frame
    RECT windowRect = clientRect;
    if (m_config.m_windowType == eWindowType::WINDOWED)
    {
        AdjustWindowRectEx(&windowRect, windowStyleFlags, FALSE, windowStyleExFlags);
    }

    // Convert window title to wide character
    WCHAR windowTitle[1024];
    MultiByteToWideChar(GetACP(), 0, m_config.m_windowTitle.c_str(), -1, windowTitle,
                        sizeof(windowTitle) / sizeof(windowTitle[0]));

    // Create the window
    HMODULE const applicationInstanceHandle = GetModuleHandle(nullptr);
    m_windowHandle                          = CreateWindowEx(
        windowStyleExFlags,
        windowClassEx.lpszClassName,
        windowTitle,
        windowStyleFlags,
        windowRect.left,
        windowRect.top,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        nullptr,
        nullptr,
        applicationInstanceHandle,
        nullptr
    );

    HWND const windowHandle = static_cast<HWND>(m_windowHandle);

    // Show window based on type
    switch (m_config.m_windowType)
    {
    case eWindowType::MINIMIZED:
        ShowWindow(windowHandle, SW_MINIMIZE);
        break;
    case eWindowType::HIDDEN:
        // Don't show the window
        break;
    default:
        ShowWindow(windowHandle, SW_SHOW);
        break;
    }

    // Set focus and foreground for visible windows
    if (m_config.m_windowType != eWindowType::HIDDEN)
    {
        SetForegroundWindow(windowHandle);
        SetFocus(windowHandle);
    }

    m_displayContext = GetDC(windowHandle);

    HCURSOR const cursor = LoadCursor(nullptr, IDC_ARROW);
    SetCursor(cursor);
}

//----------------------------------------------------------------------------------------------------
// Add these helper methods to support window type switching at runtime
void Window::SetWindowType(eWindowType newType)
{
    if (m_config.m_windowType == newType) return;

    m_config.m_windowType = newType;

    // For runtime switching, you might want to recreate the window
    // or modify the existing window properties
    ReconfigureWindow();
}

//----------------------------------------------------------------------------------------------------
void Window::SetWindowHandle(void* newWindowHandle)
{
    m_windowHandle = newWindowHandle;
}

//----------------------------------------------------------------------------------------------------
void Window::SetDisplayContext(void* newDisplayContext)
{
    m_displayContext = newDisplayContext;
}

//----------------------------------------------------------------------------------------------------
// TODO: FIX
void Window::ReconfigureWindow()
{
    if (!m_windowHandle) return;

    HWND const windowHandle = static_cast<HWND>(m_windowHandle);

    // Get desktop dimensions (screen size)
    RECT       desktopRect;
    HWND const desktopWindowHandle = GetDesktopWindow();
    GetClientRect(desktopWindowHandle, &desktopRect);
    int const   desktopWidth  = desktopRect.right - desktopRect.left;
    int const   desktopHeight = desktopRect.bottom - desktopRect.top;
    float const desktopAspect = static_cast<float>(desktopWidth) / static_cast<float>(desktopHeight);

    // IMPORTANT: Reset viewport-related variables to prevent contamination
    m_viewportDimensions = Vec2::ZERO;
    m_viewportOffset     = Vec2::ZERO;
    m_windowDimensions   = Vec2::ZERO;
    m_windowPosition     = Vec2::ZERO;
    m_clientPosition     = Vec2::ZERO;
    m_clientDimensions   = Vec2::ZERO;
    m_viewportOffset     = Vec2::ZERO;

    DWORD windowStyleFlags   = 0;
    DWORD windowStyleExFlags = WS_EX_APPWINDOW;
    RECT  newRect            = {};
    int   showCmd            = SW_SHOW;

    // Set new style and position based on window type
    switch (m_config.m_windowType)
    {
    case eWindowType::WINDOWED:
        {
            windowStyleFlags = WS_CAPTION | WS_SYSMENU | WS_OVERLAPPED | WS_MINIMIZEBOX;

            // Calculate windowed size (90% of desktop, maintaining aspect ratio) - consistent with CreateOSWindow
            float constexpr maxClientFractionOfDesktop = 0.9f;
            float const     clientAspect               = m_config.m_aspectRatio;
            float           clientWidth                = static_cast<float>(desktopWidth) * maxClientFractionOfDesktop;
            float           clientHeight               = static_cast<float>(desktopHeight) * maxClientFractionOfDesktop;

            if (clientAspect > desktopAspect)
            {
                clientHeight = clientWidth / clientAspect;
            }
            else
            {
                clientWidth = clientHeight * clientAspect;
            }

            // Center the window
            float const clientMarginX = 0.5f * (static_cast<float>(desktopWidth) - clientWidth);
            float const clientMarginY = 0.5f * (static_cast<float>(desktopHeight) - clientHeight);

            newRect.left   = static_cast<int>(clientMarginX);
            newRect.top    = static_cast<int>(clientMarginY);
            newRect.right  = newRect.left + static_cast<int>(clientWidth);
            newRect.bottom = newRect.top + static_cast<int>(clientHeight);

            m_clientDimensions = Vec2(static_cast<int>(clientWidth), static_cast<int>(clientHeight));
            // Windowed mode doesn't use viewport dimensions - they should remain zero
        }
        break;

    case eWindowType::BORDERLESS:
        {
            windowStyleFlags = WS_POPUP;

            // Same size calculation as windowed but without borders
            float constexpr maxClientFractionOfDesktop = 0.90f;
            float const     clientAspect               = m_config.m_aspectRatio;
            float           clientWidth                = static_cast<float>(desktopWidth) * maxClientFractionOfDesktop;
            float           clientHeight               = static_cast<float>(desktopHeight) * maxClientFractionOfDesktop;

            if (clientAspect > desktopAspect)
            {
                clientHeight = clientWidth / clientAspect;
            }
            else
            {
                clientWidth = clientHeight * clientAspect;
            }

            float const clientMarginX = 0.5f * (static_cast<float>(desktopWidth) - clientWidth);
            float const clientMarginY = 0.5f * (static_cast<float>(desktopHeight) - clientHeight);

            newRect.left   = static_cast<int>(clientMarginX);
            newRect.top    = static_cast<int>(clientMarginY);
            newRect.right  = newRect.left + static_cast<int>(clientWidth);
            newRect.bottom = newRect.top + static_cast<int>(clientHeight);

            m_clientDimensions = Vec2(static_cast<int>(clientWidth), static_cast<int>(clientHeight));
            // Borderless mode doesn't use viewport dimensions - they should remain zero
        }
        break;

    case eWindowType::FULLSCREEN_STRETCH:
        {
            windowStyleFlags   = WS_POPUP;
            windowStyleExFlags = WS_EX_APPWINDOW | WS_EX_TOPMOST;

            // Fill entire screen
            newRect.left   = 0;
            newRect.top    = 0;
            newRect.right  = desktopWidth;
            newRect.bottom = desktopHeight;

            m_clientDimensions = Vec2(desktopWidth, desktopHeight);
            // Stretch mode doesn't use viewport dimensions - they should remain zero
        }
        break;

    case eWindowType::FULLSCREEN_LETTERBOX:
        {
            windowStyleFlags   = WS_POPUP;
            windowStyleExFlags = WS_EX_APPWINDOW | WS_EX_TOPMOST;

            // Calculate letterbox dimensions to maintain aspect ratio
            float const targetAspect = m_config.m_aspectRatio;
            int         clientWidth, clientHeight;

            if (targetAspect > desktopAspect)
            {
                // Fit to width, letterbox top/bottom
                clientWidth  = desktopWidth;
                clientHeight = static_cast<int>(static_cast<float>(desktopWidth) / targetAspect);
            }
            else
            {
                // Fit to height, letterbox left/right
                clientHeight = desktopHeight;
                clientWidth  = static_cast<int>(static_cast<float>(desktopHeight) * targetAspect);
            }

            // Center the content area
            int const offsetX = (desktopWidth - clientWidth) / 2;
            int const offsetY = (desktopHeight - clientHeight) / 2;

            newRect.left   = 0;
            newRect.top    = 0;
            newRect.right  = desktopWidth;
            newRect.bottom = desktopHeight;

            // Store the actual screen dimensions for rendering
            m_clientDimensions   = Vec2(desktopWidth, desktopHeight);
            m_viewportDimensions = Vec2(clientWidth, clientHeight);
            m_viewportOffset     = Vec2(offsetX, offsetY);
        }
        break;

    case eWindowType::FULLSCREEN_CROP:
        {
            windowStyleFlags   = WS_POPUP;
            windowStyleExFlags = WS_EX_APPWINDOW | WS_EX_TOPMOST;

            // Fill screen and crop to maintain aspect ratio
            float const targetAspect = m_config.m_aspectRatio;
            int         clientWidth, clientHeight;

            if (targetAspect > desktopAspect)
            {
                // Fit to height, crop left/right
                clientHeight = desktopHeight;
                clientWidth  = static_cast<int>(static_cast<float>(desktopHeight) * targetAspect);
            }
            else
            {
                // Fit to width, crop top/bottom
                clientWidth  = desktopWidth;
                clientHeight = static_cast<int>(static_cast<float>(desktopWidth) / targetAspect);
            }

            DebuggerPrintf(Stringf("CROP Mode - ClientWidth = %d | ClientHeight = %d", clientWidth, clientHeight).c_str());

            // Center the viewport
            int const offsetX = (desktopWidth - clientWidth) / 2;
            int const offsetY = (desktopHeight - clientHeight) / 2;

            newRect.left   = 0;
            newRect.top    = 0;
            newRect.right  = desktopWidth;
            newRect.bottom = desktopHeight;

            // Store rendering information
            m_clientDimensions   = Vec2(desktopWidth, desktopHeight);
            m_viewportDimensions = Vec2(clientWidth, clientHeight);
            m_viewportOffset     = Vec2(offsetX, offsetY);
        }
        break;

    case eWindowType::MINIMIZED:
        ShowWindow(windowHandle, SW_MINIMIZE);
        return; // Early return, no style changes needed

    case eWindowType::HIDDEN:
        ShowWindow(windowHandle, SW_HIDE);
        return; // Early return, no style changes needed

    default:
        // Default to windowed mode if invalid type
        m_config.m_windowType = eWindowType::WINDOWED;
        return ReconfigureWindow(); // Recursive call with corrected type
    }

    // Apply new window style
    SetWindowLong(windowHandle, GWL_STYLE, windowStyleFlags);
    SetWindowLong(windowHandle, GWL_EXSTYLE, windowStyleExFlags);

    // Calculate the outer dimensions of the physical window, including frame
    RECT windowRect = newRect;
    if (m_config.m_windowType == eWindowType::WINDOWED || m_config.m_windowType == eWindowType::BORDERLESS)
    {
        AdjustWindowRectEx(&windowRect, windowStyleFlags, FALSE, windowStyleExFlags);
    }

    // Apply new position and size
    SetWindowPos(windowHandle,
                 (windowStyleExFlags & WS_EX_TOPMOST) ? HWND_TOPMOST : HWND_TOP,
                 windowRect.left, windowRect.top,
                 windowRect.right - windowRect.left, windowRect.bottom - windowRect.top,
                 SWP_FRAMECHANGED);

    // Show the window
    ShowWindow(windowHandle, showCmd);

    // Set focus and foreground for visible windows
    if (m_config.m_windowType != eWindowType::HIDDEN)
    {
        SetForegroundWindow(windowHandle);
        SetFocus(windowHandle);
    }
}


//----------------------------------------------------------------------------------------------------
// Add getter methods for render information (useful for letterbox/crop modes)
Vec2 Window::GetViewportDimensions() const
{
    // Check if render dimensions are set (non-zero)
    if (m_viewportDimensions.x > 0 && m_viewportDimensions.y > 0)
    {
        return m_viewportDimensions;
    }
    return m_clientDimensions;
}

//----------------------------------------------------------------------------------------------------
Vec2 Window::GetViewportOffset() const
{
    return m_viewportOffset;
}

Vec2 Window::GetScreenDimensions() const
{
    return m_screenDimensions;
}

Vec2 Window::GetBorderOffset()
{
    RECT windowRect, clientRect;
    GetWindowRect((HWND)m_windowHandle, &windowRect);
    GetClientRect((HWND)m_windowHandle, &clientRect);

    POINT clientTopLeft = {0, 0};
    ClientToScreen((HWND)m_windowHandle, &clientTopLeft);

    return Vec2((float)(clientTopLeft.x - windowRect.left),
                (float)(clientTopLeft.y - windowRect.top));
}

float Window::GetViewportAspectRatio() const
{
    return m_viewportDimensions.x / m_viewportDimensions.y;
}

void Window::UpdatePosition()
{
    RECT windowRect;
    GetWindowRect((HWND)m_windowHandle, &windowRect);

    // 只在實際視窗位置改變時才更新
    if (memcmp(&windowRect, &lastRect, sizeof(RECT)) != 0)
    {
        lastRect.top    = windowRect.top;
        lastRect.bottom = windowRect.bottom;
        lastRect.left   = windowRect.left;
        lastRect.right  = windowRect.right;

        // 同步所有內部座標變數（處理使用者手動拖拉視窗的情況）
        Vec2 windowsWindowPos = Vec2((float)windowRect.left,(float) windowRect.top);
        m_windowPosition = WindowsToEngineCoords(windowsWindowPos);

        POINT clientTopLeft = {0, 0};
        ClientToScreen((HWND)m_windowHandle, &clientTopLeft);
        Vec2 windowsClientPos = Vec2((float)clientTopLeft.x,(float) clientTopLeft.y);
        m_clientPosition = WindowsToEngineCoords(windowsClientPos);

        m_viewportPosition.x = m_clientPosition.x / m_screenDimensions.x;
        m_viewportPosition.y = m_clientPosition.y / m_screenDimensions.y;

        m_shouldUpdatePosition = true;
    }
}

void Window::UpdateDimension()
{
    RECT clientRect;
    GetClientRect((HWND)m_windowHandle, &clientRect);
    int newWidth  = clientRect.right - clientRect.left;
    int newHeight = clientRect.bottom - clientRect.top;

    if (newWidth != (int)m_windowDimensions.x || newHeight != (int)m_windowDimensions.y)
    {
        // m_windowDimensions.x    = (float)newWidth;
        // m_windowDimensions.y    = (float)newHeight;
        m_shouldUpdateDimension = true;
    }
}

//----------------------------------------------------------------------------------------------------
// Override GetNormalizedMouseUV to handle letterbox/crop modes
Vec2 Window::GetNormalizedMouseUV() const
{
    HWND const windowHandle = static_cast<HWND>(m_windowHandle);
    POINT      cursorCoords;
    RECT       clientRect;

    GetCursorPos(&cursorCoords);
    ScreenToClient(windowHandle, &cursorCoords);
    GetClientRect(windowHandle, &clientRect);

    // For letterbox/crop modes, adjust mouse coordinates to render area
    // if (m_config.m_windowType == eWindowType::FULLSCREEN_LETTERBOX ||
    //     m_config.m_windowType == eWindowType::FULLSCREEN_CROP)
    if (m_config.m_windowType == eWindowType::FULLSCREEN_LETTERBOX)
    {
        // Adjust cursor position relative to render area
        float const adjustedX = cursorCoords.x - m_viewportOffset.x;
        float const adjustedY = cursorCoords.y - m_viewportOffset.y;

        float const normalizedX = adjustedX / static_cast<float>(m_viewportDimensions.x);
        float const normalizedY = adjustedY / static_cast<float>(m_viewportDimensions.y);
        GetClampedZeroToOne(normalizedX);
        GetClampedZeroToOne(normalizedY);
        // Clamp to [0,1] range and flip Y
        return Vec2(normalizedX, 1.f - normalizedY);
    }

    // Standard mouse UV calculation
    float const cursorX = static_cast<float>(cursorCoords.x) / static_cast<float>(clientRect.right);
    float const cursorY = static_cast<float>(cursorCoords.y) / static_cast<float>(clientRect.bottom);

    return Vec2(cursorX, 1.f - cursorY);
}

Vec2 Window::GetCursorPositionOnScreen() const
{
    POINT cursorCoords;
    GetCursorPos(&cursorCoords);

    // 取得螢幕高度
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // 轉換座標：Y 軸反轉
    int x = static_cast<int>(cursorCoords.x);
    int y = screenHeight - static_cast<int>(cursorCoords.y);

    return Vec2(x, y);
}
void Window::AnimateToWindowDimensions(Vec2 const& targetDimensions, float duration)
{
    if (targetDimensions == m_windowDimensions) return;

    m_startWindowDimensions = m_windowDimensions;
    m_targetWindowDimensions = targetDimensions;
    m_animationDuration = duration;
    m_animationTimer = 0.0f;
    m_isAnimatingSize = true;
}

void Window::AnimateToWindowPosition(Vec2 const& targetPosition, float duration)
{
    if (targetPosition == m_windowPosition) return;

    m_startWindowPosition = m_windowPosition;
    m_targetWindowPosition = targetPosition;
    m_animationDuration = duration;
    m_animationTimer = 0.0f;
    m_isAnimatingPosition = true;
}

void Window::AnimateToWindowPositionAndDimensions(Vec2 const& targetPosition, Vec2 const& targetDimensions, float duration)
{
    m_startWindowPosition = m_windowPosition;
    m_targetWindowPosition = targetPosition;
    m_startWindowDimensions = m_windowDimensions;
    m_targetWindowDimensions = targetDimensions;
    m_animationDuration = duration;
    m_animationTimer = 0.0f;
    m_isAnimatingSize = true;
    m_isAnimatingPosition = true;
}

void Window::UpdateAnimations(float deltaSeconds)
{
    if (!IsAnimating()) return;

    m_animationTimer += deltaSeconds;
    float t = m_animationTimer / m_animationDuration;

    if (t >= 1.0f)
    {
        // 動畫完成
        t = 1.0f;
        m_isAnimatingSize = false;
        m_isAnimatingPosition = false;
    }

    // 使用 SmoothStep3 來創造平滑的動畫效果
    float easedT = SmoothStep5(t);

    if (m_isAnimatingSize)
    {
        Vec2 currentDimensions = Interpolate(m_startWindowDimensions, m_targetWindowDimensions, easedT);
        SetWindowDimensions(currentDimensions);
    }

    if (m_isAnimatingPosition)
    {
        Vec2 currentPosition = Interpolate(m_startWindowPosition, m_targetWindowPosition, easedT);
        SetWindowPosition(currentPosition);
    }
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
