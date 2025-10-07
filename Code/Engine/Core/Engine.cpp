//----------------------------------------------------------------------------------------------------
// GEngine.cpp
// Global engine singleton implementation
//----------------------------------------------------------------------------------------------------

#include <algorithm>

#include "Engine/Core/Engine.hpp"

#include "DevConsole.hpp"
#include "EngineCommon.hpp"
#include "ErrorWarningAssert.hpp"
#include "LogSubsystem.hpp"
#include "Engine/Core/JobSystem.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Platform/Window.hpp"
#include "Engine/Platform/WindowCommon.hpp"
#include "Engine/Renderer/DebugRenderSystem.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Resource/ResourceSubsystem.hpp"
#include "Engine/Script/ScriptSubsystem.hpp"

// using namespace Engine;

//----------------------------------------------------------------------------------------------------
GEngine& GEngine::Get()
{
    static GEngine instance;

    return instance;
}

//----------------------------------------------------------------------------------------------------
void GEngine::Construct()
{
    //------------------------------------------------------------------------------------------------
    // Load engine subsystem configuration
    //------------------------------------------------------------------------------------------------
    nlohmann::json subsystemConfig;
    bool hasSubsystemConfig = false;

    try
    {
        std::ifstream subsystemConfigFile("Data/Config/EngineSubsystems.json");
        if (subsystemConfigFile.is_open())
        {
            subsystemConfigFile >> subsystemConfig;
            hasSubsystemConfig = true;
            DebuggerPrintf("Loaded EngineSubsystems config from JSON\n");
        }
        else
        {
            DebuggerPrintf("EngineSubsystems.json not found, using default configuration (all enabled)\n");
        }
    }
    catch (nlohmann::json::exception const& e)
    {
        DebuggerPrintf("JSON parsing error in EngineSubsystems.json: %s\n", e.what());
        DebuggerPrintf("Using default configuration (all subsystems enabled)\n");
    }

    //------------------------------------------------------------------------------------------------
#pragma region LogSubsystem
    // Load LogSubsystem configuration from JSON file
    sLogSubsystemConfig config;

    try
    {
        std::ifstream configFile("Data/Config/LogConfig.json");
        if (configFile.is_open())
        {
            nlohmann::json jsonConfig;
            configFile >> jsonConfig;
            config = sLogSubsystemConfig::FromJSON(jsonConfig);

            // Simple success message (we can't use LogSubsystem yet as it's not initialized)
            DebuggerPrintf("Loaded LogSubsystem config from JSON\n");
        }
        else
        {
            // Fallback to hardcoded defaults if JSON file not found
            DebuggerPrintf("LogConfig.json not found, using default configuration\n");

            config.logFilePath      = "Logs/latest.log";
            config.enableConsole    = true;
            config.enableFile       = true;
            config.enableDebugOut   = true;
            config.enableOnScreen   = true;
            config.enableDevConsole = true;
            config.asyncLogging     = true;
            config.maxLogEntries    = 50000;
            config.timestampEnabled = true;
            config.threadIdEnabled  = true;
            config.autoFlush        = false;

            // Enhanced smart rotation settings
            config.enableSmartRotation = true;
            config.rotationConfigPath  = "Data/Config/LogRotation.json";

            // Configure Minecraft-style rotation settings
            config.smartRotationConfig.maxFileSizeBytes = 100 * 1024 * 1024;
            config.smartRotationConfig.maxTimeInterval  = std::chrono::hours(2);
            config.smartRotationConfig.logDirectory     = "Logs";
            config.smartRotationConfig.currentLogName   = "latest.log";
            config.smartRotationConfig.sessionPrefix    = "session";
        }
    }
    catch (nlohmann::json::exception const& e)
    {
        DebuggerPrintf("JSON parsing error in LogConfig.json: %s\n", e.what());

        // Fallback to hardcoded defaults on error
        config.logFilePath                          = "Logs/latest.log";
        config.enableConsole                        = true;
        config.enableFile                           = true;
        config.enableDebugOut                       = true;
        config.enableOnScreen                       = true;
        config.enableDevConsole                     = true;
        config.asyncLogging                         = true;
        config.maxLogEntries                        = 50000;
        config.timestampEnabled                     = true;
        config.threadIdEnabled                      = true;
        config.autoFlush                            = false;
        config.enableSmartRotation                  = true;
        config.rotationConfigPath                   = "Data/Config/LogRotation.json";
        config.smartRotationConfig.maxFileSizeBytes = 100 * 1024 * 1024;
        config.smartRotationConfig.maxTimeInterval  = std::chrono::hours(2);
        config.smartRotationConfig.logDirectory     = "Logs";
        config.smartRotationConfig.currentLogName   = "latest.log";
        config.smartRotationConfig.sessionPrefix    = "session";
    }

    g_logSubsystem = new LogSubsystem(config);
#pragma endregion
    //------------------------------------------------------------------------------------------------
#pragma region EventSystem
    sEventSystemConfig constexpr sEventSystemConfig;
    g_eventSystem = new EventSystem(sEventSystemConfig);
#pragma endregion
    //------------------------------------------------------------------------------------------------
#pragma region JobSystem
    int totalCores = static_cast<int>(std::thread::hardware_concurrency());
    // if (totalCores < 3) totalCores = 3;  // Minimum: main thread + 1 IO + 1 generic

    int numIOThreads      = 1;                         // 1 dedicated thread for file I/O
    int numGenericThreads = totalCores - 2;       // N-2 threads for computation (terrain generation, etc.)
    numGenericThreads     = (std::max)(numGenericThreads, 1);  // Ensure at least 1 generic worker

    sJobSubsystemConfig sJobSubsystemConfig;
    sJobSubsystemConfig.m_genericThreadNum = numGenericThreads;
    sJobSubsystemConfig.m_ioThreadNum      = numIOThreads;
    JobSystem* jobSystem                   = new JobSystem(sJobSubsystemConfig);
    g_jobSystem                      = jobSystem;
#pragma endregion
    //------------------------------------------------------------------------------------------------
#pragma region InputSystem
    // Check if InputSystem is enabled in config
    bool enableInput = true;  // Default: enabled
    if (hasSubsystemConfig && subsystemConfig.contains("subsystems") && subsystemConfig["subsystems"].contains("input"))
    {
        enableInput = subsystemConfig["subsystems"]["input"].value("enabled", true);
    }

    if (enableInput)
    {
        sInputSystemConfig constexpr sInputSystemConfig;
        InputSystem*                 inputSystem = new InputSystem(sInputSystemConfig);
        g_input                                  = inputSystem;
        DebuggerPrintf("InputSystem: ENABLED\n");
    }
    else
    {
        g_input = nullptr;
        DebuggerPrintf("InputSystem: DISABLED (from config)\n");
    }
#pragma endregion
    //------------------------------------------------------------------------------------------------
#pragma region Window
    sWindowConfig sWindowConfig;
    sWindowConfig.m_windowType  = eWindowType::WINDOWED;
    sWindowConfig.m_aspectRatio = 2.f;
    sWindowConfig.m_inputSystem = g_input;
    sWindowConfig.m_windowTitle = "ScriptVisualTests";
    g_window                    = new Window(sWindowConfig);
#pragma endregion
    //------------------------------------------------------------------------------------------------
#pragma region Renderer
    sRendererConfig sRendererConfig;
    sRendererConfig.m_window = g_window;
    g_renderer               = new Renderer(sRendererConfig);
#pragma endregion
    //------------------------------------------------------------------------------------------------
#pragma region DevConsole
    sDevConsoleConfig devConsoleConfig;
    devConsoleConfig.m_defaultRenderer = g_renderer;
    devConsoleConfig.m_defaultFontName = "DaemonFont";
    g_devConsole                       = new DevConsole(devConsoleConfig);
#pragma endregion
    //------------------------------------------------------------------------------------------------
#pragma region ResourceSubsystem
    sResourceSubsystemConfig resourceSubsystemConfig;
    resourceSubsystemConfig.m_renderer    = g_renderer;
    resourceSubsystemConfig.m_threadCount = 4;
    g_resourceSubsystem                   = new ResourceSubsystem(resourceSubsystemConfig);
#pragma endregion
    //------------------------------------------------------------------------------------------------
#pragma region AudioSystem
    // Check if AudioSystem is enabled in config
    bool enableAudio = true;  // Default: enabled
    if (hasSubsystemConfig && subsystemConfig.contains("subsystems") && subsystemConfig["subsystems"].contains("audio"))
    {
        enableAudio = subsystemConfig["subsystems"]["audio"].value("enabled", true);
    }

    if (enableAudio)
    {
        sAudioSystemConfig constexpr sAudioSystemConfig;
        AudioSystem*                 audioSystem = new AudioSystem(sAudioSystemConfig);
        g_audio                                  = audioSystem;
        DebuggerPrintf("AudioSystem: ENABLED\n");
    }
    else
    {
        g_audio = nullptr;
        DebuggerPrintf("AudioSystem: DISABLED (from config)\n");
    }
#pragma endregion
    //------------------------------------------------------------------------------------------------
#pragma region ScriptSubsystem
    // Check if ScriptSubsystem is enabled in config
    bool enableScript = true;  // Default: enabled
    if (hasSubsystemConfig && subsystemConfig.contains("subsystems") && subsystemConfig["subsystems"].contains("script"))
    {
        enableScript = subsystemConfig["subsystems"]["script"].value("enabled", true);
    }

    if (enableScript)
    {
        sScriptSubsystemConfig scriptConfig;

        // Read config from JSON if available
        if (hasSubsystemConfig && subsystemConfig.contains("subsystems") && subsystemConfig["subsystems"].contains("script"))
        {
            auto const& scriptJsonConfig = subsystemConfig["subsystems"]["script"]["config"];
            scriptConfig.enableDebugging     = scriptJsonConfig.value("enableDebugging", true);
            scriptConfig.heapSizeLimit       = scriptJsonConfig.value("heapSizeLimit", 256);
            scriptConfig.enableConsoleOutput = scriptJsonConfig.value("enableConsoleOutput", true);
            scriptConfig.enableHotReload     = scriptJsonConfig.value("enableHotReload", true);
            scriptConfig.enableInspector     = scriptJsonConfig.value("enableInspector", true);
            scriptConfig.inspectorPort       = scriptJsonConfig.value("inspectorPort", 9229);
            scriptConfig.inspectorHost       = scriptJsonConfig.value("inspectorHost", "127.0.0.1");
            scriptConfig.waitForDebugger     = scriptJsonConfig.value("waitForDebugger", false);
        }
        else
        {
            // Fallback to hardcoded defaults
            scriptConfig.enableDebugging     = true;
            scriptConfig.heapSizeLimit       = 256;
            scriptConfig.enableConsoleOutput = true;
            scriptConfig.enableHotReload     = true;
            scriptConfig.enableInspector     = true;
            scriptConfig.inspectorPort       = 9229;
            scriptConfig.inspectorHost       = "127.0.0.1";
            scriptConfig.waitForDebugger     = false;
        }

        g_scriptSubsystem = new ScriptSubsystem(scriptConfig);
        DebuggerPrintf("ScriptSubsystem: ENABLED\n");
    }
    else
    {
        g_scriptSubsystem = nullptr;
        DebuggerPrintf("ScriptSubsystem: DISABLED (from config)\n");
    }
#pragma endregion
    //------------------------------------------------------------------------------------------------
    g_rng = new RandomNumberGenerator();
}

//----------------------------------------------------------------------------------------------------
void GEngine::Destruct()
{
    ENGINE_SAFE_RELEASE(g_rng);
    ENGINE_SAFE_RELEASE(g_scriptSubsystem);
    ENGINE_SAFE_RELEASE(g_audio);
    ENGINE_SAFE_RELEASE(g_input);
    ENGINE_SAFE_RELEASE(g_resourceSubsystem);
    ENGINE_SAFE_RELEASE(g_devConsole);
    ENGINE_SAFE_RELEASE(g_renderer);
    ENGINE_SAFE_RELEASE(g_window);
    ENGINE_SAFE_RELEASE(g_eventSystem);
    ENGINE_SAFE_RELEASE(g_jobSystem);
    ENGINE_SAFE_RELEASE(g_logSubsystem);
}

//----------------------------------------------------------------------------------------------------
void GEngine::Startup()
{
#pragma region DebugRenderSystem
    sDebugRenderConfig sDebugRenderConfig;
    sDebugRenderConfig.m_renderer = g_renderer;
    sDebugRenderConfig.m_fontName = "DaemonFont";
#pragma endregion

    g_devConsole->AddLine(DevConsole::INFO_MAJOR, "Controls");
    g_devConsole->AddLine(DevConsole::INFO_MINOR, "(Mouse) Aim");
    g_devConsole->AddLine(DevConsole::INFO_MINOR, "(W/A)   Move");
    g_devConsole->AddLine(DevConsole::INFO_MINOR, "(S/D)   Strafe");
    g_devConsole->AddLine(DevConsole::INFO_MINOR, "(Q/E)   Roll");
    g_devConsole->AddLine(DevConsole::INFO_MINOR, "(Z/C)   Elevate");
    g_devConsole->AddLine(DevConsole::INFO_MINOR, "(Shift) Sprint");
    g_devConsole->AddLine(DevConsole::INFO_MINOR, "(H)     Set Camera to Origin");
    g_devConsole->AddLine(DevConsole::INFO_MINOR, "(1)     Spawn Line");
    g_devConsole->AddLine(DevConsole::INFO_MINOR, "(2)     Spawn Point");
    g_devConsole->AddLine(DevConsole::INFO_MINOR, "(3)     Spawn Wireframe Sphere");
    g_devConsole->AddLine(DevConsole::INFO_MINOR, "(4)     Spawn Basis");
    g_devConsole->AddLine(DevConsole::INFO_MINOR, "(5)     Spawn Billboard Text");
    g_devConsole->AddLine(DevConsole::INFO_MINOR, "(6)     Spawn Wireframe Cylinder");
    g_devConsole->AddLine(DevConsole::INFO_MINOR, "(7)     Add Message");
    g_devConsole->AddLine(DevConsole::INFO_MINOR, "(~)     Toggle Dev Console");
    g_devConsole->AddLine(DevConsole::INFO_MINOR, "(ESC)   Exit Game");
    g_devConsole->AddLine(DevConsole::INFO_MINOR, "(SPACE) Start Game");

    g_logSubsystem->Startup();
    g_jobSystem->Startup();
    g_eventSystem->Startup();
    g_window->Startup();
    g_renderer->Startup();
    g_devConsole->StartUp();
    g_resourceSubsystem->Startup();
    DebugRenderSystemStartup(sDebugRenderConfig);

    // Conditionally startup optional subsystems
    if (g_input)
    {
        g_input->Startup();
        DebuggerPrintf("InputSystem started\n");
    }

    if (g_audio)
    {
        g_audio->Startup();
        DebuggerPrintf("AudioSystem started\n");
    }

    if (g_scriptSubsystem)
    {
        g_scriptSubsystem->Startup();
        DebuggerPrintf("ScriptSubsystem started\n");
    }
}

//----------------------------------------------------------------------------------------------------
void GEngine::Shutdown()
{
    // Shutdown optional subsystems conditionally (reverse order)
    if (g_scriptSubsystem)
    {
        g_scriptSubsystem->Shutdown();
        DebuggerPrintf("ScriptSubsystem shutdown\n");
    }

    if (g_audio)
    {
        g_audio->Shutdown();
        DebuggerPrintf("AudioSystem shutdown\n");
    }

    if (g_input)
    {
        g_input->Shutdown();
        DebuggerPrintf("InputSystem shutdown\n");
    }

    // Shutdown core subsystems (reverse order of startup)
    DebugRenderSystemShutdown();
    g_resourceSubsystem->Shutdown();
    g_devConsole->Shutdown();
    g_renderer->Shutdown();
    g_window->Shutdown();
    g_eventSystem->Shutdown();
    g_jobSystem->Shutdown();
    g_logSubsystem->Shutdown();
}
