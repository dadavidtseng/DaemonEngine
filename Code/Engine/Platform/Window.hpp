//----------------------------------------------------------------------------------------------------
// Window.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once

#include "WindowCommon.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/Vec2.hpp"

//-Forward-Declaration--------------------------------------------------------------------------------
class InputSystem;

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
    IntVec2              GetClientDimensions() const;

    static Window* s_mainWindow; // fancy way of advertising global variables (advertisement)

    // Window type management
    void SetWindowType(eWindowType newType);
    void ReconfigureWindow();

    // Render information getters (useful for letterbox/crop modes)
    IntVec2 GetRenderDimensions() const;
    IntVec2 GetRenderOffset() const;
    bool IsFullscreen() const;

    // Utility functions
    float GetRenderAspectRatio() const { return static_cast<float>(GetRenderDimensions().x) / static_cast<float>(GetRenderDimensions().y); }

private:
    void CreateOSWindow();
    void CreateConsole();
    void RunMessagePump();

    sWindowConfig m_config;
    void*         m_windowHandle   = nullptr;          // Actually a Windows HWND (Handle of Window) on the Windows platform
    void*         m_displayContext = nullptr;          // Actually a Windows HDC (Handle to Device Context) on the Windows platform
    IntVec2       m_clientDimensions = IntVec2::ZERO;
    IntVec2 m_renderDimensions= IntVec2::ZERO;  // For letterbox/crop modes - actual render area size
    IntVec2 m_renderOffset= IntVec2::ZERO;      // For letterbox/crop modes - offset of render area
};
