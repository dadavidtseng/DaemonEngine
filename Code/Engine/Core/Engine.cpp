//----------------------------------------------------------------------------------------------------
// GEngine.cpp
// Global engine singleton implementation
//----------------------------------------------------------------------------------------------------
#include "Game/EngineBuildPreferences.hpp"
//----------------------------------------------------------------------------------------------------
#include "Engine/Core/Engine.hpp"

#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/JobSystem.hpp"
#include "Engine/Core/LogSubsystem.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#ifdef ENGINE_SCRIPTING_ENABLED
#include "Engine/Network/KADIWebSocketSubsystem.hpp"
#include "Engine/Script/ScriptSubsystem.hpp"
#endif // ENGINE_SCRIPTING_ENABLED
#include "Engine/Platform/Window.hpp"
#include "Engine/Platform/WindowCommon.hpp"
#include "Engine/Renderer/DebugRenderSystem.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Resource/ResourceSubsystem.hpp"
#include "Engine/UI/ImGuiSubsystem.hpp"
#include "Engine/Widget/WidgetSubsystem.hpp"

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
    bool           bHasEngineSubsystemConfig = false;

    try
    {
        std::ifstream subsystemConfigFile("Data/Config/EngineSubsystems.json");

        if (subsystemConfigFile.is_open())
        {
            subsystemConfigFile >> subsystemConfig;
            bHasEngineSubsystemConfig = true;
            DebuggerPrintf("(GEngine::Construct)EngineSubsystems.json exists. Loaded EngineSubsystems config from \"Data/Config/EngineSubsystems.json\"\n");
        }
        else
        {
            DebuggerPrintf("(GEngine::Construct)EngineSubsystems.json not found, using default configuration (all subsystems enabled)\n");
        }
    }
    catch (nlohmann::json::exception const& e)
    {
        DebuggerPrintf("(GEngine::Construct)JSON parsing error in EngineSubsystems.json: %s\n", e.what());
        DebuggerPrintf("(GEngine::Construct)Using default configuration (all subsystems enabled)\n");
    }

    //------------------------------------------------------------------------------------------------
#pragma region LogSubsystem
    // Check if LogSubsystem should be enabled from core subsystems list
    bool bEnableLogSubsystem = true;  // Default: enabled

    if (bHasEngineSubsystemConfig &&
        subsystemConfig.contains("core") &&
        subsystemConfig["core"].contains("subsystems"))
    {
        auto const& coreSubsystems = subsystemConfig["core"]["subsystems"];
        bEnableLogSubsystem        = std::ranges::find(coreSubsystems, "LogSubsystem") != coreSubsystems.end();
    }

    if (bEnableLogSubsystem)
    {
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
                DebuggerPrintf("(GEngine::Construct)Loaded LogSubsystem config from JSON\n");
            }
            else
            {
                // Fallback to hardcoded defaults if JSON file not found
                DebuggerPrintf("(GEngine::Construct)LogConfig.json not found, using default configuration\n");

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
            DebuggerPrintf("(GEngine::Construct)JSON parsing error in LogConfig.json: %s\n", e.what());

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
        DebuggerPrintf("(GEngine::Construct)LogSubsystem: ENABLED\n");
    }
    else
    {
        g_logSubsystem = nullptr;
        DebuggerPrintf("(GEngine::Construct)LogSubsystem: DISABLED (from config)\n");
    }
#pragma endregion
    //------------------------------------------------------------------------------------------------
#pragma region EventSystem
    // Check if EventSystem should be enabled from core subsystems list
    bool enableEventSystem = true;  // Default: enabled

    if (bHasEngineSubsystemConfig && subsystemConfig.contains("core") && subsystemConfig["core"].contains("subsystems"))
    {
        auto const& coreSubsystems = subsystemConfig["core"]["subsystems"];
        enableEventSystem          = std::ranges::find(coreSubsystems, "EventSystem") != coreSubsystems.end();
    }

    if (enableEventSystem)
    {
        sEventSystemConfig constexpr sEventSystemConfig;
        g_eventSystem = new EventSystem(sEventSystemConfig);
        DebuggerPrintf("(GEngine::Construct)EventSystem: ENABLED\n");
    }
    else
    {
        g_eventSystem = nullptr;
        DebuggerPrintf("(GEngine::Construct)EventSystem: DISABLED (from config)\n");
    }
#pragma endregion
    //------------------------------------------------------------------------------------------------
#pragma region JobSystem
    // Check if JobSystem should be enabled from core subsystems list
    bool bEnableJobSystem = true;  // Default: enabled

    if (bHasEngineSubsystemConfig &&
        subsystemConfig.contains("core") &&
        subsystemConfig["core"].contains("subsystems"))
    {
        auto const& coreSubsystems = subsystemConfig["core"]["subsystems"];
        bEnableJobSystem           = std::ranges::find(coreSubsystems, "JobSystem") != coreSubsystems.end();
    }

    if (bEnableJobSystem)
    {
        int totalCores = static_cast<int>(std::thread::hardware_concurrency());
        // if (totalCores < 3) totalCores = 3;  // Minimum: main thread + 1 IO + 1 generic

        int numIOThreads      = 1;                         // 1 dedicated thread for file I/O
        int numGenericThreads = totalCores - 2;       // N-2 threads for computation (terrain generation, etc.)
        numGenericThreads     = (std::max)(numGenericThreads, 1);  // Ensure at least 1 generic worker

        sJobSubsystemConfig sJobSubsystemConfig;
        sJobSubsystemConfig.m_genericThreadNum = numGenericThreads;
        sJobSubsystemConfig.m_ioThreadNum      = numIOThreads;
        JobSystem* jobSystem                   = new JobSystem(sJobSubsystemConfig);
        g_jobSystem                            = jobSystem;
        DebuggerPrintf("(GEngine::Construct)JobSystem: ENABLED\n");
    }
    else
    {
        g_jobSystem = nullptr;
        DebuggerPrintf("(GEngine::Construct)JobSystem: DISABLED (from config)\n");
    }
#pragma endregion
    //------------------------------------------------------------------------------------------------
#pragma region InputSystem
    // Check if InputSystem is enabled in config
    bool enableInput = true;  // Default: enabled

    if (bHasEngineSubsystemConfig && subsystemConfig.contains("subsystems") && subsystemConfig["subsystems"].contains("input"))
    {
        enableInput = subsystemConfig["subsystems"]["input"].value("enabled", true);
    }

    if (enableInput)
    {
        sInputSystemConfig inputConfig;

        // Read config from JSON if available
        if (bHasEngineSubsystemConfig && subsystemConfig.contains("subsystems") && subsystemConfig["subsystems"].contains("input") &&
            subsystemConfig["subsystems"]["input"].contains("config"))
        {
            // auto const& inputJsonConfig = subsystemConfig["subsystems"]["input"]["config"];
            // inputConfig.enableController = inputJsonConfig.value("enableController", true);
            // inputConfig.enableMouse = inputJsonConfig.value("enableMouse", true);
            // inputConfig.enableKeyboard = inputJsonConfig.value("enableKeyboard", true);
            // Note: sInputSystemConfig currently has no fields - reserved for future expansion
            DebuggerPrintf("(GEngine::Construct)InputSystem: JSON config available but struct has no fields yet\n");
        }
        else
        {
            // Fallback to hardcoded defaults (currently no config fields)
        }

        InputSystem* inputSystem = new InputSystem(inputConfig);
        g_input                  = inputSystem;
        DebuggerPrintf("(GEngine::Construct)InputSystem: ENABLED\n");
    }
    else
    {
        g_input = nullptr;
        DebuggerPrintf("(GEngine::Construct)InputSystem: DISABLED (from config)\n");
    }
#pragma endregion
    //------------------------------------------------------------------------------------------------
#pragma region Window
    // Check if Window should be enabled from core subsystems list
    bool enableWindow = true;  // Default: enabled

    if (bHasEngineSubsystemConfig &&
        subsystemConfig.contains("core") &&
        subsystemConfig["core"].contains("subsystems"))
    {
        auto const& coreSubsystems = subsystemConfig["core"]["subsystems"];
        enableWindow               = std::ranges::find(coreSubsystems, "Window") != coreSubsystems.end();
    }

    if (enableWindow)
    {
        sWindowConfig windowConfig;

        // Read config from JSON if available
        if (bHasEngineSubsystemConfig &&
            subsystemConfig.contains("subsystems") &&
            subsystemConfig["subsystems"].contains("platform") &&
            subsystemConfig["subsystems"]["platform"].contains("config"))
        {
            auto const& platformJsonConfig = subsystemConfig["subsystems"]["platform"]["config"];

            windowConfig.m_windowType = eWindowType::WINDOWED; // Default fallback

            // Read windowType from JSON (string to enum conversion)
            std::string windowTypeStr = platformJsonConfig.value("windowType", "WINDOWED");
            if (windowTypeStr == "WINDOWED") windowConfig.m_windowType = eWindowType::WINDOWED;
            else if (windowTypeStr == "BORDERLESS") windowConfig.m_windowType = eWindowType::BORDERLESS;
            else if (windowTypeStr == "FULLSCREEN_LETTERBOX") windowConfig.m_windowType = eWindowType::FULLSCREEN_LETTERBOX;
            else if (windowTypeStr == "FULLSCREEN_STRETCH") windowConfig.m_windowType = eWindowType::FULLSCREEN_STRETCH;
            else if (windowTypeStr == "FULLSCREEN_CROP") windowConfig.m_windowType = eWindowType::FULLSCREEN_CROP;
            else if (windowTypeStr == "MINIMIZED") windowConfig.m_windowType = eWindowType::MINIMIZED;
            else if (windowTypeStr == "HIDDEN") windowConfig.m_windowType = eWindowType::HIDDEN;

            windowConfig.m_aspectRatio            = platformJsonConfig.value("aspectRatio", 2.0f);
            windowConfig.m_windowTitle            = platformJsonConfig.value("windowTitle", "DEFAULT");
            windowConfig.m_supportMultipleWindows = platformJsonConfig.value("supportMultipleWindows", false);

            DebuggerPrintf("(GEngine::Construct)Window: Configured from JSON - Type: %s, AspectRatio: %.1f, Title: %s, MultiWindow: %s\n",
                           windowTypeStr.c_str(), windowConfig.m_aspectRatio, windowConfig.m_windowTitle.c_str(),
                           windowConfig.m_supportMultipleWindows ? "true" : "false");
        }
        else
        {
            // Fallback to hardcoded defaults
            windowConfig.m_windowType  = eWindowType::WINDOWED;
            windowConfig.m_aspectRatio = 2.f;
            windowConfig.m_windowTitle = "DEFAULT";
            DebuggerPrintf("(GEngine::Construct)Window: Using hardcoded defaults\n");
        }

        windowConfig.m_inputSystem = g_input;
        g_window                   = new Window(windowConfig);
        DebuggerPrintf("(GEngine::Construct)Window: ENABLED\n");
    }
    else
    {
        g_window = nullptr;
        DebuggerPrintf("(GEngine::Construct)Window: DISABLED (from config)\n");
    }
#pragma endregion
    //------------------------------------------------------------------------------------------------
#pragma region Renderer
    // Check if Renderer should be enabled from core subsystems list
    bool enableRenderer = true;  // Default: enabled
    if (bHasEngineSubsystemConfig && subsystemConfig.contains("core") && subsystemConfig["core"].contains("subsystems"))
    {
        auto const& coreSubsystems = subsystemConfig["core"]["subsystems"];
        enableRenderer             = std::ranges::find(coreSubsystems, "Renderer") != coreSubsystems.end();
    }

    if (enableRenderer)
    {
        sRendererConfig sRendererConfig;
        sRendererConfig.m_window = g_window;
        g_renderer               = new Renderer(sRendererConfig);
        DebuggerPrintf("(GEngine::Construct)Renderer: ENABLED\n");
    }
    else
    {
        g_renderer = nullptr;
        DebuggerPrintf("(GEngine::Construct)Renderer: DISABLED (from config)\n");
    }
#pragma endregion
    //------------------------------------------------------------------------------------------------
#pragma region ImGuiSubsystem
    // Check if ImGuiSubsystem should be enabled from core subsystems list
    bool enableImGui = true;  // Default: enabled
    if (bHasEngineSubsystemConfig && subsystemConfig.contains("core") && subsystemConfig["core"].contains("subsystems"))
    {
        auto const& coreSubsystems = subsystemConfig["core"]["subsystems"];
        enableImGui                = std::ranges::find(coreSubsystems, "ImGuiSubsystem") != coreSubsystems.end();
    }

    if (enableImGui && g_renderer && g_window)
    {
        sImGuiSubsystemConfig imguiConfig;
        imguiConfig.m_renderer = g_renderer;
        imguiConfig.m_window   = g_window;
        g_imgui                = new ImGuiSubsystem(imguiConfig);
        DebuggerPrintf("(GEngine::Construct)ImGuiSubsystem: ENABLED\n");
    }
    else
    {
        g_imgui = nullptr;
        if (!enableImGui)
        {
            DebuggerPrintf("(GEngine::Construct)ImGuiSubsystem: DISABLED (from config)\n");
        }
        else
        {
            DebuggerPrintf("(GEngine::Construct)ImGuiSubsystem: DISABLED (missing Renderer or Window)\n");
        }
    }
#pragma endregion
    //------------------------------------------------------------------------------------------------
#pragma region DevConsole
    // Check if DevConsole should be enabled from core subsystems list
    bool enableDevConsole = true;  // Default: enabled
    if (bHasEngineSubsystemConfig && subsystemConfig.contains("core") && subsystemConfig["core"].contains("subsystems"))
    {
        auto const& coreSubsystems = subsystemConfig["core"]["subsystems"];
        enableDevConsole           = std::find(coreSubsystems.begin(), coreSubsystems.end(), "DevConsole") != coreSubsystems.end();
    }

    if (enableDevConsole)
    {
        sDevConsoleConfig devConsoleConfig;
        devConsoleConfig.m_defaultRenderer = g_renderer;
        devConsoleConfig.m_defaultFontName = "DaemonFont";
        g_devConsole                       = new DevConsole(devConsoleConfig);
        DebuggerPrintf("(GEngine::Construct)DevConsole: ENABLED\n");
    }
    else
    {
        g_devConsole = nullptr;
        DebuggerPrintf("(GEngine::Construct)DevConsole: DISABLED (from config)\n");
    }
#pragma endregion
    //------------------------------------------------------------------------------------------------
#pragma region ResourceSubsystem
    // Check if ResourceSubsystem should be enabled from core subsystems list
    bool enableResourceSubsystem = true;  // Default: enabled
    if (bHasEngineSubsystemConfig && subsystemConfig.contains("core") && subsystemConfig["core"].contains("subsystems"))
    {
        auto const& coreSubsystems = subsystemConfig["core"]["subsystems"];
        enableResourceSubsystem    = std::find(coreSubsystems.begin(), coreSubsystems.end(), "ResourceSubsystem") != coreSubsystems.end();
    }

    if (enableResourceSubsystem)
    {
        sResourceSubsystemConfig resourceSubsystemConfig;
        resourceSubsystemConfig.m_renderer    = g_renderer;
        resourceSubsystemConfig.m_threadCount = 4;
        g_resourceSubsystem                   = new ResourceSubsystem(resourceSubsystemConfig);
        DebuggerPrintf("(GEngine::Construct)ResourceSubsystem: ENABLED\n");
    }
    else
    {
        g_resourceSubsystem = nullptr;
        DebuggerPrintf("(GEngine::Construct)ResourceSubsystem: DISABLED (from config)\n");
    }
#pragma endregion
    //------------------------------------------------------------------------------------------------
#pragma region WidgetSubsystem
    // Check if WidgetSubsystem should be enabled from core subsystems list
    bool enableWidgetSubsystem = true;  // Default: enabled
    if (bHasEngineSubsystemConfig && subsystemConfig.contains("core") && subsystemConfig["core"].contains("subsystems"))
    {
        auto const& coreSubsystems = subsystemConfig["core"]["subsystems"];
        enableWidgetSubsystem      = std::ranges::find(coreSubsystems, "WidgetSubsystem") != coreSubsystems.end();
    }

    if (enableWidgetSubsystem)
    {
        sWidgetSubsystemConfig widgetConfig;
        // Use default config values (initialWidgetCapacity=64, initialOwnerCapacity=32)
        g_widgetSubsystem = new WidgetSubsystem(widgetConfig);
        DebuggerPrintf("(GEngine::Construct)WidgetSubsystem: ENABLED\n");
    }
    else
    {
        g_widgetSubsystem = nullptr;
        DebuggerPrintf("(GEngine::Construct)WidgetSubsystem: DISABLED (from config)\n");
    }
#pragma endregion
    //------------------------------------------------------------------------------------------------
#pragma region AudioSystem
    // Check if AudioSystem is enabled in config
    bool enableAudio = true;  // Default: enabled
    if (bHasEngineSubsystemConfig && subsystemConfig.contains("subsystems") && subsystemConfig["subsystems"].contains("audio"))
    {
        enableAudio = subsystemConfig["subsystems"]["audio"].value("enabled", true);
    }

    if (enableAudio)
    {
        sAudioSystemConfig audioConfig;

        // Read config from JSON if available
        if (bHasEngineSubsystemConfig && subsystemConfig.contains("subsystems") && subsystemConfig["subsystems"].contains("audio") &&
            subsystemConfig["subsystems"]["audio"].contains("config"))
        {
            // auto const& audioJsonConfig = subsystemConfig["subsystems"]["audio"]["config"];
            // audioConfig.maxChannels = audioJsonConfig.value("maxChannels", 128);
            // Note: sAudioSystemConfig currently has no fields - reserved for future expansion
            DebuggerPrintf("(GEngine::Construct)AudioSystem: JSON config available but struct has no fields yet\n");
        }
        else
        {
            // Fallback to hardcoded defaults (currently no config fields)
        }

        AudioSystem* audioSystem = new AudioSystem(audioConfig);
        g_audio                  = audioSystem;
        DebuggerPrintf("(GEngine::Construct)AudioSystem: ENABLED\n");
    }
    else
    {
        g_audio = nullptr;
        DebuggerPrintf("(GEngine::Construct)AudioSystem: DISABLED (from config)\n");
    }
#pragma endregion
    //------------------------------------------------------------------------------------------------
#ifdef ENGINE_SCRIPTING_ENABLED
#pragma region ScriptSubsystem
    // Check if ScriptSubsystem is enabled in config
    bool enableScript = true;  // Default: enabled
    if (bHasEngineSubsystemConfig && subsystemConfig.contains("subsystems") && subsystemConfig["subsystems"].contains("script"))
    {
        enableScript = subsystemConfig["subsystems"]["script"].value("enabled", true);
    }

    if (enableScript)
    {
        sScriptSubsystemConfig scriptConfig;

        // Read config from JSON if available
        if (bHasEngineSubsystemConfig && subsystemConfig.contains("subsystems") && subsystemConfig["subsystems"].contains("script"))
        {
            auto const& scriptJsonConfig     = subsystemConfig["subsystems"]["script"]["config"];
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
        DebuggerPrintf("(GEngine::Construct)ScriptSubsystem: ENABLED\n");
    }
    else
    {
        g_scriptSubsystem = nullptr;
        DebuggerPrintf("(GEngine::Construct)ScriptSubsystem: DISABLED (from config)\n");
    }
#pragma endregion
#endif // ENGINE_SCRIPTING_ENABLED
    //------------------------------------------------------------------------------------------------
#pragma region Math (RandomNumberGenerator)
    // Check if Math subsystem (RNG) should be enabled
    bool enableMath = true;  // Default: enabled
    if (bHasEngineSubsystemConfig && subsystemConfig.contains("subsystems") && subsystemConfig["subsystems"].contains("math"))
    {
        enableMath = subsystemConfig["subsystems"]["math"].value("enabled", true);
    }

    if (enableMath)
    {
        // Read config from JSON if available
        unsigned int seed = 0;  // 0 = use time-based seed
        if (bHasEngineSubsystemConfig && subsystemConfig.contains("subsystems") && subsystemConfig["subsystems"].contains("math") &&
            subsystemConfig["subsystems"]["math"].contains("config"))
        {
            auto const& mathJsonConfig = subsystemConfig["subsystems"]["math"]["config"];
            if (mathJsonConfig.contains("defaultSeed") && !mathJsonConfig["defaultSeed"].is_null())
            {
                seed = mathJsonConfig.value("defaultSeed", 0u);
                DebuggerPrintf("(GEngine::Construct)Math (RandomNumberGenerator): Using custom seed %u from config\n", seed);
            }
        }

        if (seed == 0)
        {
            g_rng = new RandomNumberGenerator();  // Time-based seed
        }
        else
        {
            g_rng = new RandomNumberGenerator();  // Custom seed from JSON
        }
        DebuggerPrintf("(GEngine::Construct)Math (RandomNumberGenerator): ENABLED\n");
    }
    else
    {
        g_rng = nullptr;
        DebuggerPrintf("(GEngine::Construct)Math (RandomNumberGenerator): DISABLED (from config)\n");
    }
#pragma endregion
    //------------------------------------------------------------------------------------------------
#pragma region Network (NetworkTCPSubsystem)
    // NOTE: NetworkTCPSubsystem exists but is not yet integrated into global engine initialization
    // Implementation available in Engine/Network/NetworkTCPSubsystem.hpp
    // To integrate: Add g_networkSubsystem global pointer to EngineCommon.hpp and implement here

    // Check if Network subsystem should be enabled
    bool enableNetwork = false;  // Default: disabled (not yet integrated)
    if (bHasEngineSubsystemConfig && subsystemConfig.contains("subsystems") && subsystemConfig["subsystems"].contains("network"))
    {
        enableNetwork = subsystemConfig["subsystems"]["network"].value("enabled", false);
    }

    if (enableNetwork)
    {
        DebuggerPrintf("(GEngine::Construct)Network (NetworkTCPSubsystem): NOT YET IMPLEMENTED\n");
        DebuggerPrintf("(GEngine::Construct)  - NetworkTCPSubsystem class exists in Engine/Network/\n");
        DebuggerPrintf("(GEngine::Construct)  - Requires global g_networkSubsystem pointer integration\n");
    }
    else
    {
        DebuggerPrintf("(GEngine::Construct)Network (NetworkTCPSubsystem): DISABLED (from config or not integrated)\n");
    }
#pragma endregion
    //------------------------------------------------------------------------------------------------
#pragma region KADI (KADIWebSocketSubsystem)
#ifdef ENGINE_SCRIPTING_ENABLED
    // KADI broker integration for distributed agent communication
    // Implementation available in Engine/Network/KADIWebSocketSubsystem.hpp

    // Check if KADI subsystem should be enabled
    bool enableKADI = true;  // Default: enabled for Phase 1 testing
    if (bHasEngineSubsystemConfig && subsystemConfig.contains("subsystems") && subsystemConfig["subsystems"].contains("kadi"))
    {
        enableKADI = subsystemConfig["subsystems"]["kadi"].value("enabled", true);
    }

    if (enableKADI)
    {
        g_kadiSubsystem = new KADIWebSocketSubsystem();
        DebuggerPrintf("(GEngine::Construct)KADIWebSocketSubsystem: ENABLED\n");
    }
    else
    {
        g_kadiSubsystem = nullptr;
        DebuggerPrintf("(GEngine::Construct)KADIWebSocketSubsystem: DISABLED (from config)\n");
    }
#endif // ENGINE_SCRIPTING_ENABLED
#pragma endregion
    //------------------------------------------------------------------------------------------------
}

//----------------------------------------------------------------------------------------------------
void GEngine::Destruct()
{
#ifdef ENGINE_SCRIPTING_ENABLED
    ENGINE_SAFE_RELEASE(g_kadiSubsystem);
#endif // ENGINE_SCRIPTING_ENABLED
    ENGINE_SAFE_RELEASE(g_rng);
#ifdef ENGINE_SCRIPTING_ENABLED
    ENGINE_SAFE_RELEASE(g_scriptSubsystem);
#endif // ENGINE_SCRIPTING_ENABLED
    ENGINE_SAFE_RELEASE(g_audio);
    ENGINE_SAFE_RELEASE(g_input);
    ENGINE_SAFE_RELEASE(g_resourceSubsystem);
    ENGINE_SAFE_RELEASE(g_devConsole);
    ENGINE_SAFE_RELEASE(g_imgui);
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

    if (g_devConsole != nullptr)
    {
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
    }

    // Startup core subsystems (check for null before calling)
    if (g_logSubsystem != nullptr)
    {
        g_logSubsystem->Startup();
        DebuggerPrintf("(GEngine::Startup)LogSubsystem started\n");
    }

    if (g_jobSystem != nullptr)
    {
        g_jobSystem->Startup();
    }

    if (g_eventSystem != nullptr)
    {
        g_eventSystem->Startup();
        DebuggerPrintf("(GEngine::Startup)EventSystem started\n");
    }

    if (g_window != nullptr)
    {
        g_window->Startup();
        DebuggerPrintf("(GEngine::Startup)Window started\n");
    }

    if (g_renderer != nullptr)
    {
        g_renderer->Startup();
        DebuggerPrintf("(GEngine::Startup)Renderer started\n");
    }

    if (g_resourceSubsystem != nullptr)
    {
        g_resourceSubsystem->Startup();
        DebuggerPrintf("(GEngine::Startup)ResourceSubsystem started\n");
    }

    if (g_renderer != nullptr)
    {
        g_renderer->PostStartup();
        DebuggerPrintf("(GEngine::Startup)Renderer PostStartup completed\n");
    }

    // KADI initialization moved here (Option A: immediately after Renderer, before resource loading)
    // This ensures MCP tools are registered quickly (~50ms), before Claude Desktop typically connects
    // Previously: KADI initialized last (after 17 seconds of resource loading)
    // Now: KADI connects and registers tools before textures/shaders load
#ifdef ENGINE_SCRIPTING_ENABLED
    if (g_kadiSubsystem != nullptr)
    {
        g_kadiSubsystem->Startup();
        DebuggerPrintf("(GEngine::Startup)KADIWebSocketSubsystem started\n");
    }
#endif // ENGINE_SCRIPTING_ENABLED

    if (g_imgui != nullptr)
    {
        g_imgui->Startup();
        DebuggerPrintf("(GEngine::Startup)ImGuiSubsystem started\n");
    }


    if (g_devConsole != nullptr)
    {
        g_devConsole->StartUp();
        DebuggerPrintf("(GEngine::Startup)DevConsole started\n");
    }

    if (g_renderer != nullptr)
    {
        DebugRenderSystemStartup(sDebugRenderConfig);
        DebuggerPrintf("(GEngine::Startup)DebugRenderSystem started\n");
    }

    // Conditionally startup optional subsystems
    if (g_input != nullptr)
    {
        g_input->Startup();
        DebuggerPrintf("(GEngine::Startup)InputSystem started\n");
    }

    if (g_audio != nullptr)
    {
        g_audio->Startup();
        DebuggerPrintf("(GEngine::Startup)AudioSystem started\n");
    }

#ifdef ENGINE_SCRIPTING_ENABLED
    if (g_scriptSubsystem != nullptr)
    {
        g_scriptSubsystem->Startup();
        DebuggerPrintf("(GEngine::Startup)ScriptSubsystem started\n");
    }
#endif

    // Assignment 7: Start WidgetSubsystem
    if (g_widgetSubsystem != nullptr)
    {
        g_widgetSubsystem->StartUp();
        DebuggerPrintf("(GEngine::Startup)WidgetSubsystem started\n");
    }
}

//----------------------------------------------------------------------------------------------------
void GEngine::Shutdown()
{
    // Shutdown optional subsystems conditionally (reverse order)
    if (g_widgetSubsystem)
    {
        g_widgetSubsystem->ShutDown();
        DebuggerPrintf("(GEngine::Shutdown)WidgetSubsystem shutdown\n");
    }

#ifdef ENGINE_SCRIPTING_ENABLED
    if (g_scriptSubsystem)
    {
        g_scriptSubsystem->Shutdown();
        DebuggerPrintf("(GEngine::Shutdown)ScriptSubsystem shutdown\n");
    }
#endif

    if (g_audio)
    {
        g_audio->Shutdown();
        DebuggerPrintf("(GEngine::Shutdown)AudioSystem shutdown\n");
    }

    if (g_input)
    {
        g_input->Shutdown();
        DebuggerPrintf("(GEngine::Shutdown)InputSystem shutdown\n");
    }

    // Shutdown core subsystems (reverse order of startup, check for null)

    DebugRenderSystemShutdown();
    DebuggerPrintf("(GEngine::Shutdown)DebugRenderSystem shutdown\n");


    if (g_resourceSubsystem)
    {
        g_resourceSubsystem->Shutdown();
        DebuggerPrintf("(GEngine::Shutdown)ResourceSubsystem shutdown\n");
    }

    if (g_devConsole)
    {
        g_devConsole->Shutdown();
        DebuggerPrintf("(GEngine::Shutdown)DevConsole shutdown\n");
    }

    if (g_imgui)
    {
        g_imgui->Shutdown();
        DebuggerPrintf("(GEngine::Shutdown)ImGuiSubsystem shutdown\n");
    }

    // KADI shutdown moved here to match new startup order (after ImGui, before Renderer)
#ifdef ENGINE_SCRIPTING_ENABLED
    if (g_kadiSubsystem)
    {
        g_kadiSubsystem->Shutdown();
        DebuggerPrintf("(GEngine::Shutdown)KADIWebSocketSubsystem shutdown\n");
    }
#endif // ENGINE_SCRIPTING_ENABLED

    if (g_renderer)
    {
        g_renderer->Shutdown();
        DebuggerPrintf("(GEngine::Shutdown)Renderer shutdown\n");
    }

    if (g_window)
    {
        g_window->Shutdown();
        DebuggerPrintf("(GEngine::Shutdown)Window shutdown\n");
    }

    if (g_eventSystem)
    {
        g_eventSystem->Shutdown();
        DebuggerPrintf("(GEngine::Shutdown)EventSystem shutdown\n");
    }

    if (g_jobSystem)
    {
        g_jobSystem->Shutdown();
        DebuggerPrintf("(GEngine::Shutdown)JobSystem shutdown\n");
    }

    if (g_logSubsystem)
    {
        g_logSubsystem->Shutdown();
        DebuggerPrintf("(GEngine::Shutdown)LogSubsystem shutdown\n");
    }
}
