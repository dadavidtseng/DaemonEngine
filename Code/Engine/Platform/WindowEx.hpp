//----------------------------------------------------------------------------------------------------
// Window.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once

#include "Engine/Core/StringUtils.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Renderer/RendererEx.hpp"
// #include "Engine/Renderer/RendererEx.hpp"

//-Forward-Declaration--------------------------------------------------------------------------------
class InputSystem;

//----------------------------------------------------------------------------------------------------
struct sWindowExConfig
{
    InputSystem*   m_inputSystem = nullptr;
    float          m_aspectRatio = 16.f / 9.f;
    String         m_consoleTitle[11];
    String         m_windowTitle  = "DEFAULT";
    wchar_t const* m_iconFilePath = nullptr;
};

// 漂移參數結構
struct DriftParams
{
    float velocityX;            // X方向速度 (像素/秒)
    float velocityY;            // Y方向速度 (像素/秒)
    float acceleration;         // 加速度係數
    float drag;                 // 阻力係數 (0.95-0.99)
    float bounceEnergy;         // 反彈能量保留係數 (0.7-0.9)
    float wanderStrength;       // 隨機漂移強度
    float targetVelocity;       // 目標速度
    bool  enableGravity;        // 是否啟用重力
    bool  enableWander;         // 是否啟用隨機漂移

    DriftParams();
};

//----------------------------------------------------------------------------------------------------
class WindowEx
{
    friend class RendererEx;

public:
    explicit WindowEx(sWindowExConfig const& config);
    // WindowEx() = default;
    ~WindowEx() = default;
    void Startup();
    void Shutdown();

    void BeginFrame();
    void EndFrame();

    sWindowExConfig const& GetConfig() const;
    void*                  GetDisplayContext() const;
    void*                  GetWindowHandle() const;
    Vec2                   GetNormalizedMouseUV() const;
    IntVec2                GetClientDimensions() const;

    // void      UpdateWindow();
    void      UpdatePosition();
    bool      IsWindowClassRegistered();
    WindowEx* CreateChildWindow(wchar_t const* title, int x, int y, int width, int height);
    void      UpdateWindowPosition(int sceneWidth, int sceneHeight, int virtualScreenWidth, int virtualScreenHeight);

    // static WindowEx* s_mainWindowEx; // fancy way of advertising global variables (advertisement)

private:
    void CreateOSWindow();
    void CreateConsole();
    void RunMessagePump();

    sWindowExConfig m_config;
    void*           m_windowInstance   = nullptr;
    void*           m_windowHandle     = nullptr;          // Actually a Windows HWND (Handle of Window) on the Windows platform
    void*           m_displayContext   = nullptr;          // Actually a Windows HDC (Handle to Device Context) on the Windows platform
    IntVec2         m_clientDimensions = IntVec2::ZERO;
    int             x,         y,         width,         height;
    float           viewportX, viewportY, viewportWidth, viewportHeight;
    RECT            lastRect{};
    bool            needsUpdate = true;
};

LRESULT CALLBACK WindowsExMessageHandlingProcedure(HWND windowHandle, UINT wmMessageCode, WPARAM wParam, LPARAM lParam);
