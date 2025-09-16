//----------------------------------------------------------------------------------------------------
// Window.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once

#include <random>

#include "Engine/Core/StringUtils.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Platform/WindowCommon.hpp"
#include "Engine/Renderer/Renderer.hpp"

//-Forward-Declaration--------------------------------------------------------------------------------
class InputSystem;
struct IDXGISwapChain1;
struct ID3D11RenderTargetView;

//----------------------------------------------------------------------------------------------------
struct sWindowConfig
{
    InputSystem*   m_inputSystem = nullptr;
    eWindowType    m_windowType  = eWindowType::INVALID;
    float          m_aspectRatio = 2.f;
    String         m_consoleTitle[11];
    String         m_windowTitle            = "DEFAULT";
    wchar_t const* m_iconFilePath           = nullptr;
    bool           m_supportMultipleWindows = false;
};

struct WindowRect
{
    long left   = 0;
    long top    = 0;
    long right  = 0;
    long bottom = 0;
};

//----------------------------------------------------------------------------------------------------
class Window
{
    friend class Renderer;

public:
    explicit Window(sWindowConfig config);
    ~Window() = default;
    void Startup();
    void Shutdown();

    void BeginFrame();
    void EndFrame();

    sWindowConfig const& GetConfig() const;
    void*                GetDisplayContext() const;
    void*                GetWindowHandle() const;
    Vec2                 GetNormalizedMouseUV() const;
    Vec2                 GetCursorPositionOnScreen() const;
    void                 EnableGlobalInputCapture();
    void                 DisableGlobalInputCapture();


    static Window* s_mainWindow; // fancy way of advertising global variables (advertisement)

    // Window type management
    void SetWindowType(eWindowType newType);
    void SetWindowHandle(void* newWindowHandle);
    void SetDisplayContext(void* newDisplayContext);
    void ReconfigureWindow();

    // Render information getters (useful for letterbox/crop modes)
    Vec2 GetClientDimensions() const;
    Vec2 GetClientPosition() const;
    void SetClientDimensions(Vec2 const& newDimensions);
    void SetClientPosition(Vec2 const& newPosition);
    Vec2 GetWindowPosition() const;
    Vec2 GetWindowDimensions() const;
    void SetWindowDimensions(Vec2 const& newDimensions);
    void SetWindowPosition(Vec2 const& newPosition);
    Vec2 EngineToWindowsCoords(Vec2 const& engineCoords) const;
    Vec2 WindowsToEngineCoords(Vec2 const& windowsCoords) const;
    Vec2 GetViewportDimensions() const;
    Vec2 GetViewportOffset() const;
    Vec2 GetScreenDimensions() const;
    Vec2 GetBorderOffset();

    // Utility functions
    float GetViewportAspectRatio() const;

    // WindowEx
    void UpdatePosition();
    void UpdateDimension();

    WindowRect lastRect{};
    bool       m_shouldUpdatePosition  = false;
    bool       m_shouldUpdateDimension = false;
    bool       m_useGlobalCapture;

private:
    void                    CreateOSWindow();
    void                    CreateConsole();
    void                    RunMessagePump() const;
    static LRESULT CALLBACK GlobalMouseProc(int nCode, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK GlobalKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
    sWindowConfig           m_config;
    void*                   m_windowHandle     = nullptr;          // Actually a Windows HWND (Handle of Window) on the Windows platform
    void*                   m_displayContext   = nullptr;          // Actually a Windows HDC (Handle to Device Context) on the Windows platform
    IDXGISwapChain1*        m_swapChain        = nullptr;
    ID3D11RenderTargetView* m_renderTargetView = nullptr;

    Vec2 m_screenDimensions   = Vec2(GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
    Vec2 m_windowDimensions   = Vec2::ZERO;         // the dimension of your window, will need conversion if m_windowType is `eWindowType::WINDOWED`
    Vec2 m_windowPosition     = Vec2::ZERO;         // the position of your window, will need conversion if m_windowType is `eWindowType::WINDOWED`
    Vec2 m_clientDimensions   = Vec2::ZERO;         // the dimension of your client dimension
    Vec2 m_clientPosition     = Vec2::ZERO;         // the position of your client position
    Vec2 m_viewportDimensions = Vec2::ZERO;         // the dimension of your viewport, which is used by `Renderer` and `Camera`
    Vec2 m_viewportPosition   = Vec2::ZERO;         // the position of your viewport, which is used by `Renderer` and `Camera`
    Vec2 m_viewportOffset     = Vec2::ZERO;         // For letterbox/crop modes - offset of render area

    HHOOK m_globalMouseHook    = nullptr;
    HHOOK m_globalKeyboardHook = nullptr;
};
