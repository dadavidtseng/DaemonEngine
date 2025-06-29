//----------------------------------------------------------------------------------------------------
// WindowEx.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once

#include <chrono>
#include <random>

#include "Engine/Core/StringUtils.hpp"
#include "Engine/Renderer/Renderer.hpp"

//-Forward-Declaration--------------------------------------------------------------------------------
class InputSystem;
// struct RECT;


//----------------------------------------------------------------------------------------------------
struct sWindowExConfig
{
    InputSystem*   m_inputSystem = nullptr;
    float          m_aspectRatio = 16.f / 9.f;
    String         m_consoleTitle[11];
    String         m_windowTitle  = "DEFAULT";
    wchar_t const* m_iconFilePath = nullptr;
};

struct WinRect
{
    long left   = 0;
    long top    = 0;
    long right  = 0;
    long bottom = 0;
};

struct Point
{
    long x = 0;
    long y = 0;
};

struct DriftParams
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

class WindowEx
{
public:
    WindowEx();
    void*   m_windowHandle   = nullptr;
    void*   m_displayContext = nullptr;
    int     x                = 0, y         = 0, width         = 0, height         = 0;
    float   viewportX        = 0, viewportY = 0, viewportWidth = 0, viewportHeight = 0;
    WinRect lastRect{};
    bool    needsUpdate = true;
    bool    needsResize = false;

    int virtualScreenWidth;
    int virtualScreenHeight;

    // 漂移相關
    DriftParams                           drift;
    std::mt19937                          rng;
    std::uniform_real_distribution<float> wanderDist{-1.0f, 1.0f};
    // std::chrono::steady_clock::time_point lastUpdateTime;
    bool  isDragging = false;          // 是否正在被拖拽
    POINT dragOffset{};         // 拖拽偏移
    void  BeginFrame();
    void  RunMessagePump();
    void  UpdateWindowDrift(float deltaSeconds);
    void  UpdateWindowPosition();

    IDXGISwapChain*         m_swapChain        = nullptr;
    ID3D11RenderTargetView* m_renderTargetView = nullptr;
};

LRESULT CALLBACK WindowsMessageHandlingProcedure(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
