//----------------------------------------------------------------------------------------------------
// Window.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once

#include "Engine/Core/StringUtils.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/Vec2.hpp"

//-Forward-Declaration--------------------------------------------------------------------------------
class InputSystem;

//----------------------------------------------------------------------------------------------------
struct WindowConfig
{
    InputSystem* m_inputSystem = nullptr;
    float        m_aspectRatio = 16.f / 9.f;
    String       m_consoleTitle[11];
    String       m_windowTitle = "Unnamed Application";
};

//----------------------------------------------------------------------------------------------------
class Window
{
public:
    friend class Renderer;
    explicit Window(WindowConfig const& config);
    ~Window() = default;
    void Startup();
    void Shutdown();

    void BeginFrame();
    void EndFrame();

    WindowConfig const& GetConfig() const;
    void*               GetDisplayContext() const;
    void*               GetWindowHandle() const;
    Vec2                GetNormalizedMouseUV() const;
    IntVec2             GetClientDimensions() const;

    static Window* s_mainWindow; // fancy way of advertising global variables (advertisement)

private:
    void CreateOSWindow();
    void CreateConsole();
    void RunMessagePump();

    WindowConfig m_config;
    void*        m_windowHandle     = nullptr;          // Actually a Windows HWND (Handle of Window) on the Windows platform
    void*        m_displayContext   = nullptr;          // Actually a Windows HDC (Handle to Device Context) on the Windows platform
    IntVec2      m_clientDimensions = IntVec2::ZERO;
};
