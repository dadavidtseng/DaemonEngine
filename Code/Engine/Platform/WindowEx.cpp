//----------------------------------------------------------------------------------------------------
// WindowEx.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Platform/WindowEx.hpp"

#include <iostream>

// #define CONSOLE_HANDLER
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <Engine/Core/EngineCommon.hpp>

#include "Engine/Core/EventSystem.hpp"
#include "Engine/Input/InputSystem.hpp"


//----------------------------------------------------------------------------------------------------
// STATIC WindowEx* WindowEx::s_mainWindowEx = nullptr;


WindowEx::WindowEx()
{
    virtualScreenWidth  = GetSystemMetrics(SM_CXSCREEN);
    virtualScreenHeight = GetSystemMetrics(SM_CYSCREEN);
    // 初始化隨機數生成器
    rng.seed((unsigned int)std::chrono::steady_clock::now().time_since_epoch().count());
    // lastUpdateTime = std::chrono::steady_clock::now();

    // 隨機初始速度
    std::uniform_real_distribution<float> velDist(-50.0f, 50.0f);
    drift.velocityX = velDist(rng);
    drift.velocityY = velDist(rng);
}

void WindowEx::BeginFrame()
{
    RunMessagePump();
}

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

void WindowEx::UpdateWindowDrift(float const deltaSeconds)
{
    if (isDragging) return; // 拖拽時不漂移

    // auto  currentTime     = std::chrono::steady_clock::now();
    // float deltaTime       = std::chrono::duration<float>(currentTime - window.lastUpdateTime).count();
    // window.lastUpdateTime = currentTime;
    //
    // // if (deltaTime > 0.1f) deltaTime = 0.1f; // 限制最大 delta time
    // // 更嚴格的 delta time 控制
    // if (deltaTime > 0.016f) deltaTime = 0.016f; // 限制為 60fps
    // if (deltaTime < 0.001f) return; // 太小的變化直接忽略

    RECT windowRect;
    GetWindowRect((HWND)m_windowHandle, &windowRect);

    int currentX = windowRect.left;
    int currentY = windowRect.top;

    // 重力效果
    if (drift.enableGravity)
    {
        drift.velocityY += drift.acceleration * deltaSeconds;
    }

    // 隨機漂移
    if (drift.enableWander)
    {
        drift.velocityX += wanderDist(rng) * drift.wanderStrength * deltaSeconds;
        drift.velocityY += wanderDist(rng) * drift.wanderStrength * deltaSeconds;
    }

    // 速度限制
    float const currentSpeed = sqrt(drift.velocityX * drift.velocityX +
        drift.velocityY * drift.velocityY);
    if (currentSpeed > drift.targetVelocity)
    {
        float const scale = drift.targetVelocity / currentSpeed;
        drift.velocityX *= scale;
        drift.velocityY *= scale;
    }

    // 阻力
    drift.velocityX *= drift.drag;
    drift.velocityY *= drift.drag;

    // 計算新位置
    int newX = currentX + static_cast<int>(drift.velocityX * deltaSeconds);
    int newY = currentY + static_cast<int>(drift.velocityY * deltaSeconds);

    // 邊界碰撞檢測和反彈
    RECT clientRect;
    GetClientRect((HWND)m_windowHandle, &clientRect);
    int const windowWidth  = clientRect.right - clientRect.left;
    int const windowHeight = clientRect.bottom - clientRect.top;

    bool bounced = false;

    // 左右邊界
    if (newX < 0)
    {
        newX            = 0;
        drift.velocityX = -drift.velocityX * drift.bounceEnergy;
        bounced         = true;
    }
    else if (newX + windowWidth > virtualScreenWidth)
    {
        newX            = virtualScreenWidth - windowWidth;
        drift.velocityX = -drift.velocityX * drift.bounceEnergy;
        bounced         = true;
    }

    // 上下邊界
    if (newY < 0)
    {
        newY            = 0;
        drift.velocityY = -drift.velocityY * drift.bounceEnergy;
        bounced         = true;
    }
    else if (newY + windowHeight > virtualScreenHeight)
    {
        newY            = virtualScreenHeight - windowHeight;
        drift.velocityY = -drift.velocityY * drift.bounceEnergy;
        bounced         = true;
    }

    // 反彈時添加一些隨機性
    if (bounced)
    {
        std::uniform_real_distribution<float> bounceDist(-30.0f, 30.0f);
        drift.velocityX += bounceDist(rng);
        drift.velocityY += bounceDist(rng);
    }

    // 移動窗口
    if (newX != currentX || newY != currentY)
    {
        SetWindowPos((HWND)m_windowHandle, nullptr, newX, newY, 0, 0,
                     SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

void WindowEx::UpdateWindowPosition()
{
    RECT windowRect;
    GetWindowRect((HWND)m_windowHandle, &windowRect);

    // 检查窗口位置或大小是否改变
    if (memcmp(&windowRect, &lastRect, sizeof(RECT)) != 0)
    {
        lastRect.left   = windowRect.left;
        lastRect.top    = windowRect.top;
        lastRect.right  = windowRect.right;
        lastRect.bottom = windowRect.bottom;
        needsUpdate     = true;

        RECT clientRect;
        GetClientRect((HWND)m_windowHandle, &clientRect);
        int newWidth  = clientRect.right - clientRect.left;
        int newHeight = clientRect.bottom - clientRect.top;

        // 检查客户区大小是否改变（需要重新创建SwapChain）
        if (newWidth != width || newHeight != height)
        {
            width  = newWidth;
            height = newHeight;
            needsResize = true; // 添加这个标志
        }

        // 重新计算viewport参数
        viewportX      = (float)windowRect.left / (float)virtualScreenWidth;
        viewportY      = (float)windowRect.top / (float)virtualScreenHeight;
        viewportWidth  = (float)width / (float)virtualScreenWidth;
        viewportHeight = (float)height / (float)virtualScreenHeight;

float sceneWidth = 1920.f;
        float sceneHeight = 1080.f;

        // 确保座标对齐到像素边界
        float pixelAlignX = 1.0f / (float)sceneWidth;
        float pixelAlignY = 1.0f / (float)sceneHeight;

        viewportX      = floor(viewportX / pixelAlignX) * pixelAlignX;
        viewportY      = floor(viewportY / pixelAlignY) * pixelAlignY;
        viewportWidth  = ceil(viewportWidth / pixelAlignX) * pixelAlignX;
        viewportHeight = ceil(viewportHeight / pixelAlignY) * pixelAlignY;

        // 边界检查
        viewportX      = max(0.0f, min(1.0f,viewportX));
        viewportY      = max(0.0f, min(1.0f,viewportY));
        viewportWidth  = max(0.0f, min(1.0f -viewportX, viewportWidth));
        viewportHeight = max(0.0f, min(1.0f -viewportY, viewportHeight));
    }
}

// 窗口程序
LRESULT CALLBACK WindowsMessageHandlingProcedure(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{InputSystem* input = nullptr;
    switch (uMsg)
    {
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
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
