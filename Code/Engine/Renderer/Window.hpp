//----------------------------------------------------------------------------------------------------
// Window.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include <string>

#include "Engine/Math/Vec2.hpp"

//----------------------------------------------------------------------------------------------------
class InputSystem;

//-----------------------------------------------------------------------------------------------
struct WindowConfig
{
	InputSystem *m_inputSystem = nullptr;
	float        m_aspectRatio = 16.f / 9.f;
	std::string  m_consoleTitle[11];
	std::string  m_windowTitle = "Unnamed Application";
};

//-----------------------------------------------------------------------------------------------
class Window
{
public:
	friend class Renderer;
	explicit Window(WindowConfig const& config);
	~Window();

	void Startup();
	void Shutdown();

	void BeginFrame();
	void EndFrame();

	WindowConfig const& GetConfig() const;
	void*               GetDisplayContext() const;
	void*               GetWindowHandle() const;
	Vec2                GetNormalizedMouseUV() const;

	static Window *s_mainWindow; // fancy way of advertising global variables (advertisement)

private:
	void CreateOSWindow();
	void CreateConsole();
	void RunMessagePump();

	WindowConfig m_config;
	void *       m_windowHandle   = nullptr; // Actually a Windows HWND on the Windows platform
	void *       m_displayContext = nullptr; // Actually a Windows HDC on the Windows platform
};
