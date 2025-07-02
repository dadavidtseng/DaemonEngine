//----------------------------------------------------------------------------------------------------
// Window.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once

#include <random>

#include "Engine/Core/StringUtils.hpp"
#include "Engine/Math/IntVec2.hpp"
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

struct WindowDriftParams
{
    float velocityX      = 0;            // X方向速度 (像素/秒)
    float velocityY      = 0;            // Y方向速度 (像素/秒)
    float acceleration   = 50.0f;         // 加速度係數
    float drag           = 0.98f;                 // 阻力係數 (0.95-0.99)
    float bounceEnergy   = 0.8f;         // 反彈能量保留係數 (0.7-0.9)
    float wanderStrength = 2000.0f;       // 隨機漂移強度
    float targetVelocity = 100.0f;       // 目標速度
    bool  enableGravity  = true;        // 是否啟用重力
    bool  enableWander   = true;         // 是否啟用隨機漂移
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
    IntVec2              GetClientDimensions() const;

    static Window* s_mainWindow; // fancy way of advertising global variables (advertisement)

    // Window type management
    void SetWindowType(eWindowType newType);
    void ReconfigureWindow();

    // Render information getters (useful for letterbox/crop modes)
    IntVec2 GetRenderDimensions() const;
    IntVec2 GetRenderOffset() const;
    bool    IsFullscreen() const;

    // Utility functions
    float                   GetRenderAspectRatio() const { return static_cast<float>(GetRenderDimensions().x) / static_cast<float>(GetRenderDimensions().y); }
    void*                   m_windowHandle     = nullptr;          // Actually a Windows HWND (Handle of Window) on the Windows platform
    void*                   m_displayContext   = nullptr;          // Actually a Windows HDC (Handle to Device Context) on the Windows platform
    IDXGISwapChain1*        m_swapChain        = nullptr;
    ID3D11RenderTargetView* m_renderTargetView = nullptr;


    // WindowEx
    void       UpdateWindowDrift(float deltaSeconds);
    void       UpdateWindowPosition(Vec2 const& newPosition);
    void       UpdateWindowPosition();
    int        x         = 0, y         = 0, width         = 0, height         = 0;
    float      viewportX = 0, viewportY = 0, viewportWidth = 0, viewportHeight = 0;
    WindowRect lastRect{};
    bool       needsUpdate = true;
    bool       needsResize = false;
    int        virtualScreenWidth;
    int        virtualScreenHeight;

    // 漂移相關
    WindowDriftParams                     drift;
    std::mt19937                          rng;
    std::uniform_real_distribution<float> wanderDist{-1.0f, 1.0f};
    bool                                  isDragging = false;
    // POINT                                 dragOffset{};

private:
    void CreateOSWindow();
    void CreateConsole();
    void RunMessagePump();

    sWindowConfig m_config;

    IntVec2 m_clientDimensions = IntVec2(GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
    IntVec2 m_renderDimensions = IntVec2::ZERO;      // For letterbox/crop modes - actual render area size
    IntVec2 m_renderOffset     = IntVec2::ZERO;      // For letterbox/crop modes - offset of render area
};
