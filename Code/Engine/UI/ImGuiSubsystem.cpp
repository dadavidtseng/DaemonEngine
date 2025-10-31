//----------------------------------------------------------------------------------------------------
// ImGuiSubsystem.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/UI/ImGuiSubsystem.hpp"
//----------------------------------------------------------------------------------------------------
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Platform/Window.hpp"
#include "Engine/Renderer/Renderer.hpp"
//----------------------------------------------------------------------------------------------------
#include "ThirdParty/imgui/imgui.h"
#include "ThirdParty/imgui/backends/imgui_impl_dx11.h"
#include "ThirdParty/imgui/backends/imgui_impl_win32.h"
//----------------------------------------------------------------------------------------------------
#include <Windows.h>

//----------------------------------------------------------------------------------------------------
// External DevConsole pointer
//----------------------------------------------------------------------------------------------------
extern DevConsole* g_devConsole;

//----------------------------------------------------------------------------------------------------
ImGuiSubsystem::ImGuiSubsystem(sImGuiSubsystemConfig const& config)
    : m_config(config)
{
    GUARANTEE_OR_DIE(config.m_renderer != nullptr, "ImGuiSubsystem: Renderer cannot be null")
    GUARANTEE_OR_DIE(config.m_window != nullptr, "ImGuiSubsystem: Window cannot be null")
}

//----------------------------------------------------------------------------------------------------
ImGuiSubsystem::~ImGuiSubsystem()
{
    if (m_isInitialized)
    {
        Shutdown();
    }
}

//----------------------------------------------------------------------------------------------------
void ImGuiSubsystem::Startup()
{
    if (m_isInitialized)
    {
        return;
    }

    // Get DirectX 11 device and context from Renderer
    m_device        = m_config.m_renderer->m_device;
    m_deviceContext = m_config.m_renderer->m_deviceContext;
    m_windowHandle  = m_config.m_window->GetWindowHandle();

    GUARANTEE_OR_DIE(m_device != nullptr, "ImGuiSubsystem: D3D11 Device is null")
    GUARANTEE_OR_DIE(m_deviceContext != nullptr, "ImGuiSubsystem: D3D11 DeviceContext is null")
    GUARANTEE_OR_DIE(m_windowHandle != nullptr, "ImGuiSubsystem: Window handle is null")

    InitializeContext();
    SetupBackends();

    m_isInitialized = true;

    DebuggerPrintf("ImGuiSubsystem: Initialized successfully\n");
}

//----------------------------------------------------------------------------------------------------
void ImGuiSubsystem::Shutdown()
{
    if (!m_isInitialized)
    {
        return;
    }

    CleanupBackends();

    if (ImGui::GetCurrentContext() != nullptr)
    {
        ImGui::DestroyContext();
    }

    m_isInitialized = false;
    m_device        = nullptr;
    m_deviceContext = nullptr;
    m_windowHandle  = nullptr;

    DebuggerPrintf("ImGuiSubsystem: Shutdown complete\n");
}

//----------------------------------------------------------------------------------------------------
void ImGuiSubsystem::BeginFrame()
{
    if (!m_isInitialized)
    {
        return;
    }

    // Start ImGui frame
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

//----------------------------------------------------------------------------------------------------
void ImGuiSubsystem::EndFrame()
{
    if (!m_isInitialized)
    {
        return;
    }

    // ImGui::EndFrame() is called automatically by ImGui::Render()
}

//----------------------------------------------------------------------------------------------------
void ImGuiSubsystem::Update()
{
    if (!m_isInitialized)
    {
        return;
    }

    // Alternative to BeginFrame() - does the same thing
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

//----------------------------------------------------------------------------------------------------
void ImGuiSubsystem::Render()
{
    if (!m_isInitialized)
    {
        return;
    }

    // Finalize ImGui frame and render
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

//----------------------------------------------------------------------------------------------------
void ImGuiSubsystem::InitializeContext()
{
    // Create ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    (void)io;

    // Enable keyboard and gamepad navigation
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    // Enable docking (optional, but useful for complex UIs)
    // Note: ImGuiConfigFlags_DockingEnable not available in this ImGui version
    // io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // Disable .ini file for now (can enable later if we want persistent window layouts)
    io.IniFilename = nullptr;

    // Set default dark theme
    // SetDarkTheme();
    SetLightTheme();
}

//----------------------------------------------------------------------------------------------------
void ImGuiSubsystem::SetupBackends()
{
    // Initialize Win32 platform backend
    bool win32InitSuccess = ImGui_ImplWin32_Init(m_windowHandle);
    GUARANTEE_OR_DIE(win32InitSuccess, "ImGuiSubsystem: Failed to initialize Win32 backend")

    // Initialize DirectX 11 renderer backend
    bool dx11InitSuccess = ImGui_ImplDX11_Init(m_device, m_deviceContext);
    GUARANTEE_OR_DIE(dx11InitSuccess, "ImGuiSubsystem: Failed to initialize DX11 backend")
}

//----------------------------------------------------------------------------------------------------
void ImGuiSubsystem::CleanupBackends()
{
    // CRITICAL: Flush pending commands and unbind all resources before shutdown
    // This prevents DirectX memory leaks by ensuring ImGui's buffers are properly unbound
    // before ImGui_ImplDX11_Shutdown() attempts to release them
    if (m_deviceContext != nullptr)
    {
        m_deviceContext->Flush();        // Execute all pending GPU commands
        m_deviceContext->ClearState();   // Unbind all vertex/index buffers, shaders, render targets
    }

    // Now safely shutdown ImGui backends
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
}

//----------------------------------------------------------------------------------------------------
void ImGuiSubsystem::SetDarkTheme()
{
    ImGui::StyleColorsDark();
}

//----------------------------------------------------------------------------------------------------
void ImGuiSubsystem::SetLightTheme()
{
    ImGui::StyleColorsLight();
}

//----------------------------------------------------------------------------------------------------
bool ImGuiSubsystem::WantCaptureMouse() const
{
    if (!m_isInitialized)
    {
        return false;
    }

    ImGuiIO& io = ImGui::GetIO();
    return io.WantCaptureMouse;
}

//----------------------------------------------------------------------------------------------------
bool ImGuiSubsystem::WantCaptureKeyboard() const
{
    if (!m_isInitialized)
    {
        return false;
    }

    ImGuiIO& io = ImGui::GetIO();
    return io.WantCaptureKeyboard;
}

//----------------------------------------------------------------------------------------------------
bool ImGuiSubsystem::ProcessWin32Message(void*            hwnd,
                                         unsigned int     msg,
                                         unsigned __int64 wParam,
                                         __int64          lParam)
{
    if (!m_isInitialized)
    {
        return false;
    }

    // Forward message to ImGui Win32 handler first
    extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    if (ImGui_ImplWin32_WndProcHandler(static_cast<HWND>(hwnd), msg, static_cast<WPARAM>(wParam), static_cast<LPARAM>(lParam)))
    {
        return true;
    }

    // Simple ImGui input capture logic - only when DevConsole is open
    if (g_devConsole && g_devConsole->IsOpen())
    {
        ImGuiIO& io = ImGui::GetIO();

        // Helper lambdas to identify event types
        auto IsMouseEvent = [](UINT eventMsg) -> bool
        {
            return eventMsg == WM_LBUTTONDOWN || eventMsg == WM_LBUTTONUP ||
                eventMsg == WM_RBUTTONDOWN || eventMsg == WM_RBUTTONUP ||
                eventMsg == WM_MBUTTONDOWN || eventMsg == WM_MBUTTONUP ||
                eventMsg == WM_MOUSEMOVE || eventMsg == WM_MOUSEWHEEL;
        };

        auto IsKeyEvent = [](UINT eventMsg) -> bool
        {
            return eventMsg == WM_KEYDOWN || eventMsg == WM_KEYUP ||
                eventMsg == WM_SYSKEYDOWN || eventMsg == WM_SYSKEYUP ||
                eventMsg == WM_CHAR;
        };

        // Block mouse events if ImGui wants to capture mouse
        if (io.WantCaptureMouse && IsMouseEvent(msg))
        {
            return true;
        }

        // Block keyboard events if ImGui wants to capture keyboard
        if (io.WantCaptureKeyboard && IsKeyEvent(msg))
        {
            return true;
        }
    }

    return false;
}
