//----------------------------------------------------------------------------------------------------
// ImGuiSubsystem.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once

//----------------------------------------------------------------------------------------------------
// Forward Declarations
//----------------------------------------------------------------------------------------------------
class Renderer;
class Window;
struct ID3D11Device;
struct ID3D11DeviceContext;

//----------------------------------------------------------------------------------------------------
// ImGuiSystem Configuration
//----------------------------------------------------------------------------------------------------
struct sImGuiSubsystemConfig
{
    Renderer* m_renderer = nullptr;
    Window*   m_window   = nullptr;
};

//----------------------------------------------------------------------------------------------------
/// @brief ImGui subsystem for debug UI rendering
///
/// @details Manages Dear ImGui integration with DirectX 11 and Win32.
///          Handles initialization, frame lifecycle, and cleanup of ImGui contexts.
///          Must be constructed after Renderer and Window subsystems.
///
/// @remark ImGui is single-threaded and should only be called from main thread
/// @remark All ImGui rendering happens after game rendering, before EndFrame()
//----------------------------------------------------------------------------------------------------
class ImGuiSubsystem
{
public:
    explicit ImGuiSubsystem(sImGuiSubsystemConfig const& config);
    ~ImGuiSubsystem();

    void Startup();
    void Shutdown();

    void BeginFrame();
    void EndFrame();
    void Update();
    void Render();

    // Win32 message processing with DevConsole-aware input capture
    bool ProcessWin32Message(void* hwnd, unsigned int msg, unsigned __int64 wParam, __int64 lParam);

    // Configuration
    void SetDarkTheme();
    void SetLightTheme();

    // State queries
    bool IsInitialized() const { return m_isInitialized; }
    bool WantCaptureMouse() const;
    bool WantCaptureKeyboard() const;

private:
    sImGuiSubsystemConfig m_config;
    bool                  m_isInitialized = false;

    ID3D11Device*        m_device        = nullptr;
    ID3D11DeviceContext* m_deviceContext = nullptr;
    void*                m_windowHandle  = nullptr;

    void InitializeContext();
    void SetupBackends();
    void CleanupBackends();
};
