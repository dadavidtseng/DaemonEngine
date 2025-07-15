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
    float          m_aspectRatio = 4.f / 3.f;
    String         m_consoleTitle[11];
    String         m_windowTitle  = "DEFAULT";
    wchar_t const* m_iconFilePath = nullptr;
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
    explicit Window(sWindowConfig const& config);
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

    static Window* s_mainWindow; // fancy way of advertising global variables (advertisement)

    // Window type management
    void SetWindowType(eWindowType newType);
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
    Vec2 GetViewportDimensions() const;
    Vec2 GetViewportOffset() const;
    Vec2 GetScreenDimensions() const;
    bool IsFullscreen() const;

    // Utility functions
    float                   GetViewportAspectRatio() const;
    void*                   m_windowHandle     = nullptr;          // Actually a Windows HWND (Handle of Window) on the Windows platform
    void*                   m_displayContext   = nullptr;          // Actually a Windows HDC (Handle to Device Context) on the Windows platform
    IDXGISwapChain1*        m_swapChain        = nullptr;
    ID3D11RenderTargetView* m_renderTargetView = nullptr;


    // WindowEx
    void UpdatePosition(Vec2 const& newPosition);
    void UpdatePosition();
    void UpdateDimension();

    WindowRect lastRect{};
    bool       m_shouldUpdatePosition  = false;
    bool       m_shouldUpdateDimension = false;

private:
    void CreateOSWindow();
    void CreateConsole();
    void RunMessagePump();

    sWindowConfig m_config;

    Vec2 m_screenDimensions   = Vec2(GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
    Vec2 m_windowPosition     = Vec2::ZERO;
    Vec2 m_windowDimensions   = Vec2::ZERO;
    Vec2 m_clientPosition     = Vec2::ZERO;
    Vec2 m_clientDimensions   = Vec2::ZERO;
    Vec2 m_viewportPosition   = Vec2::ZERO;
    Vec2 m_viewportDimensions = Vec2::ZERO;      // For letterbox/crop modes - actual render area size
    Vec2 m_renderOffset       = Vec2::ZERO;      // For letterbox/crop modes - offset of render area
};
