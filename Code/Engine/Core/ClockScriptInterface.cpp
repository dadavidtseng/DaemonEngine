//----------------------------------------------------------------------------------------------------
// ClockScriptInterface.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Core/ClockScriptInterface.hpp"

//----------------------------------------------------------------------------------------------------
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/LogSubsystem.hpp"
#include "Engine/Script/ScriptTypeExtractor.hpp"

//----------------------------------------------------------------------------------------------------
ClockScriptInterface::ClockScriptInterface()
{
    // Initialize method registry for efficient dispatch
    ClockScriptInterface::InitializeMethodRegistry();
}

//----------------------------------------------------------------------------------------------------
ClockScriptInterface::~ClockScriptInterface()
{
    // Clean up all JavaScript-created clocks
    for (Clock* clock : m_createdClocks)
    {
        delete clock;
    }
    m_createdClocks.clear();
}

//----------------------------------------------------------------------------------------------------
void ClockScriptInterface::InitializeMethodRegistry()
{
    // === CLOCK CREATION AND DESTRUCTION ===
    m_methodRegistry["createClock"]  = [this](ScriptArgs const& args) { return ExecuteCreateClock(args); };
    m_methodRegistry["destroyClock"] = [this](ScriptArgs const& args) { return ExecuteDestroyClock(args); };

    // === PAUSE CONTROL METHODS ===
    m_methodRegistry["pause"]       = [this](ScriptArgs const& args) { return ExecutePause(args); };
    m_methodRegistry["unpause"]     = [this](ScriptArgs const& args) { return ExecuteUnpause(args); };
    m_methodRegistry["togglePause"] = [this](ScriptArgs const& args) { return ExecuteTogglePause(args); };
    m_methodRegistry["isPaused"]    = [this](ScriptArgs const& args) { return ExecuteIsPaused(args); };

    // === TIME CONTROL METHODS ===
    m_methodRegistry["stepSingleFrame"] = [this](ScriptArgs const& args) { return ExecuteStepSingleFrame(args); };
    m_methodRegistry["setTimeScale"]    = [this](ScriptArgs const& args) { return ExecuteSetTimeScale(args); };
    m_methodRegistry["reset"]           = [this](ScriptArgs const& args) { return ExecuteReset(args); };

    // === TIME QUERY METHODS ===
    m_methodRegistry["getTimeScale"]    = [this](ScriptArgs const& args) { return ExecuteGetTimeScale(args); };
    m_methodRegistry["getDeltaSeconds"] = [this](ScriptArgs const& args) { return ExecuteGetDeltaSeconds(args); };
    m_methodRegistry["getTotalSeconds"] = [this](ScriptArgs const& args) { return ExecuteGetTotalSeconds(args); };
    m_methodRegistry["getFrameCount"]   = [this](ScriptArgs const& args) { return ExecuteGetFrameCount(args); };
}

//----------------------------------------------------------------------------------------------------
std::vector<ScriptMethodInfo> ClockScriptInterface::GetAvailableMethods() const
{
    return {
        // === CLOCK CREATION AND DESTRUCTION ===
        ScriptMethodInfo("createClock",
                         "Create a new clock instance as child of system clock",
                         {},
                         "Clock*"),

        ScriptMethodInfo("destroyClock",
                         "Destroy a clock instance",
                         {"Clock*"},
                         "void"),

        // === PAUSE CONTROL METHODS ===
        ScriptMethodInfo("pause",
                         "Pause the specified clock, stopping time progression",
                         {"Clock*"},
                         "void"),

        ScriptMethodInfo("unpause",
                         "Unpause the specified clock, resuming time progression",
                         {"Clock*"},
                         "void"),

        ScriptMethodInfo("togglePause",
                         "Toggle pause state of the specified clock",
                         {"Clock*"},
                         "void"),

        ScriptMethodInfo("isPaused",
                         "Check if the specified clock is currently paused",
                         {"Clock*"},
                         "bool"),

        // === TIME CONTROL METHODS ===
        ScriptMethodInfo("stepSingleFrame",
                         "Advance the specified clock by a single frame while paused",
                         {"Clock*"},
                         "void"),

        ScriptMethodInfo("setTimeScale",
                         "Set time scale multiplier for the specified clock (1.0 = normal, 0.5 = slow motion, 2.0 = fast forward)",
                         {"Clock*", "number"},
                         "void"),

        ScriptMethodInfo("reset",
                         "Reset the specified clock to initial state",
                         {"Clock*"},
                         "void"),

        // === TIME QUERY METHODS ===
        ScriptMethodInfo("getTimeScale",
                         "Get current time scale multiplier of the specified clock",
                         {"Clock*"},
                         "number"),

        ScriptMethodInfo("getDeltaSeconds",
                         "Get time elapsed since last frame in seconds for the specified clock",
                         {"Clock*"},
                         "number"),

        ScriptMethodInfo("getTotalSeconds",
                         "Get total accumulated time in seconds for the specified clock",
                         {"Clock*"},
                         "number"),

        ScriptMethodInfo("getFrameCount",
                         "Get total number of frames processed by the specified clock",
                         {"Clock*"},
                         "number")
    };
}

//----------------------------------------------------------------------------------------------------
StringList ClockScriptInterface::GetAvailableProperties() const
{
    return {
        // Clock doesn't currently expose properties
        // Could add timeScale, isPaused as properties in future
    };
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult ClockScriptInterface::CallMethod(String const& methodName, ScriptArgs const& args)
{
    try
    {
        auto it = m_methodRegistry.find(methodName);
        if (it != m_methodRegistry.end())
        {
            return it->second(args);
        }

        return ScriptMethodResult::Error("Unknown clock method: " + methodName);
    }
    catch (std::exception const& e)
    {
        return ScriptMethodResult::Error("Clock method execution failed: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
std::any ClockScriptInterface::GetProperty(String const& propertyName) const
{
    // No properties currently implemented
    UNUSED(propertyName)
    return std::any{};
}

//----------------------------------------------------------------------------------------------------
bool ClockScriptInterface::SetProperty(String const& propertyName, std::any const& value)
{
    // No properties currently implemented
    UNUSED(propertyName)
    UNUSED(value)
    return false;
}

//----------------------------------------------------------------------------------------------------
// CLOCK CREATION AND DESTRUCTION
//----------------------------------------------------------------------------------------------------

ScriptMethodResult ClockScriptInterface::ExecuteCreateClock(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 0, "createClock");
    if (!result.success) return result;

    try
    {
        // Create new clock as child of system clock (matching C++ Game::Game() behavior)
        Clock* newClock = new Clock(Clock::GetSystemClock());
        m_createdClocks.push_back(newClock);

        // Return clock handle (pointer as number)
        double handle = static_cast<double>(reinterpret_cast<uintptr_t>(newClock));
        return ScriptMethodResult::Success(handle);
    }
    catch (const std::exception& e)
    {
        return ScriptMethodResult::Error("Failed to create clock: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult ClockScriptInterface::ExecuteDestroyClock(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 1, "destroyClock");
    if (!result.success) return result;

    try
    {
        double clockHandle = ScriptTypeExtractor::ExtractDouble(args[0]);
        Clock* clock = reinterpret_cast<Clock*>(static_cast<uintptr_t>(clockHandle));

        // Find and remove from created clocks vector
        auto it = std::find(m_createdClocks.begin(), m_createdClocks.end(), clock);
        if (it != m_createdClocks.end())
        {
            delete *it;
            m_createdClocks.erase(it);
            return ScriptMethodResult::Success("Clock destroyed successfully");
        }

        return ScriptMethodResult::Error("Clock not found in managed clocks");
    }
    catch (const std::exception& e)
    {
        return ScriptMethodResult::Error("Failed to destroy clock: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
// PAUSE CONTROL METHODS
//----------------------------------------------------------------------------------------------------

ScriptMethodResult ClockScriptInterface::ExecutePause(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 1, "pause");
    if (!result.success) return result;

    try
    {
        Clock* clock = ExtractClockFromHandle(ScriptTypeExtractor::ExtractDouble(args[0]));
        if (!clock)
        {
            return ScriptMethodResult::Error("Invalid clock handle");
        }

        clock->Pause();
        return ScriptMethodResult::Success("Clock paused successfully");
    }
    catch (const std::exception& e)
    {
        return ScriptMethodResult::Error("Failed to pause clock: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult ClockScriptInterface::ExecuteUnpause(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 1, "unpause");
    if (!result.success) return result;

    try
    {
        Clock* clock = ExtractClockFromHandle(ScriptTypeExtractor::ExtractDouble(args[0]));
        if (!clock)
        {
            return ScriptMethodResult::Error("Invalid clock handle");
        }

        clock->Unpause();
        return ScriptMethodResult::Success("Clock unpaused successfully");
    }
    catch (const std::exception& e)
    {
        return ScriptMethodResult::Error("Failed to unpause clock: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult ClockScriptInterface::ExecuteTogglePause(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 1, "togglePause");
    if (!result.success) return result;

    try
    {
        Clock* clock = ExtractClockFromHandle(ScriptTypeExtractor::ExtractDouble(args[0]));
        if (!clock)
        {
            return ScriptMethodResult::Error("Invalid clock handle");
        }

        clock->TogglePause();
        return ScriptMethodResult::Success("Clock pause toggled successfully");
    }
    catch (const std::exception& e)
    {
        return ScriptMethodResult::Error("Failed to toggle clock pause: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult ClockScriptInterface::ExecuteIsPaused(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 1, "isPaused");
    if (!result.success) return result;

    try
    {
        Clock* clock = ExtractClockFromHandle(ScriptTypeExtractor::ExtractDouble(args[0]));
        if (!clock)
        {
            return ScriptMethodResult::Error("Invalid clock handle");
        }

        bool isPaused = clock->IsPaused();
        return ScriptMethodResult::Success(isPaused);
    }
    catch (const std::exception& e)
    {
        return ScriptMethodResult::Error("Failed to check pause state: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
// TIME CONTROL METHODS
//----------------------------------------------------------------------------------------------------

ScriptMethodResult ClockScriptInterface::ExecuteStepSingleFrame(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 1, "stepSingleFrame");
    if (!result.success) return result;

    try
    {
        Clock* clock = ExtractClockFromHandle(ScriptTypeExtractor::ExtractDouble(args[0]));
        if (!clock)
        {
            return ScriptMethodResult::Error("Invalid clock handle");
        }

        clock->StepSingleFrame();
        return ScriptMethodResult::Success("Clock stepped single frame successfully");
    }
    catch (const std::exception& e)
    {
        return ScriptMethodResult::Error("Failed to step single frame: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult ClockScriptInterface::ExecuteSetTimeScale(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 2, "setTimeScale");
    if (!result.success) return result;

    try
    {
        Clock* clock = ExtractClockFromHandle(ScriptTypeExtractor::ExtractDouble(args[0]));
        if (!clock)
        {
            return ScriptMethodResult::Error("Invalid clock handle");
        }

        float timeScale = ScriptTypeExtractor::ExtractFloat(args[1]);

        if (!ValidateTimeScale(timeScale))
        {
            return ScriptMethodResult::Error("Time scale must be between 0.0 and 10.0");
        }

        clock->SetTimeScale(timeScale);
        return ScriptMethodResult::Success("Time scale set successfully");
    }
    catch (const std::exception& e)
    {
        return ScriptMethodResult::Error("Failed to set time scale: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult ClockScriptInterface::ExecuteReset(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 1, "reset");
    if (!result.success) return result;

    try
    {
        Clock* clock = ExtractClockFromHandle(ScriptTypeExtractor::ExtractDouble(args[0]));
        if (!clock)
        {
            return ScriptMethodResult::Error("Invalid clock handle");
        }

        clock->Reset();
        return ScriptMethodResult::Success("Clock reset successfully");
    }
    catch (const std::exception& e)
    {
        return ScriptMethodResult::Error("Failed to reset clock: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
// TIME QUERY METHODS
//----------------------------------------------------------------------------------------------------

ScriptMethodResult ClockScriptInterface::ExecuteGetTimeScale(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 1, "getTimeScale");
    if (!result.success) return result;

    try
    {
        Clock* clock = ExtractClockFromHandle(ScriptTypeExtractor::ExtractDouble(args[0]));
        if (!clock)
        {
            return ScriptMethodResult::Error("Invalid clock handle");
        }

        float timeScale = clock->GetTimeScale();
        return ScriptMethodResult::Success(static_cast<double>(timeScale));
    }
    catch (const std::exception& e)
    {
        return ScriptMethodResult::Error("Failed to get time scale: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult ClockScriptInterface::ExecuteGetDeltaSeconds(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 1, "getDeltaSeconds");
    if (!result.success) return result;

    try
    {
        Clock* clock = ExtractClockFromHandle(ScriptTypeExtractor::ExtractDouble(args[0]));
        if (!clock)
        {
            return ScriptMethodResult::Error("Invalid clock handle");
        }

        double deltaSeconds = clock->GetDeltaSeconds();
        return ScriptMethodResult::Success(deltaSeconds);
    }
    catch (const std::exception& e)
    {
        return ScriptMethodResult::Error("Failed to get delta seconds: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult ClockScriptInterface::ExecuteGetTotalSeconds(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 1, "getTotalSeconds");
    if (!result.success) return result;

    try
    {
        Clock* clock = ExtractClockFromHandle(ScriptTypeExtractor::ExtractDouble(args[0]));
        if (!clock)
        {
            return ScriptMethodResult::Error("Invalid clock handle");
        }

        double totalSeconds = clock->GetTotalSeconds();
        return ScriptMethodResult::Success(totalSeconds);
    }
    catch (const std::exception& e)
    {
        return ScriptMethodResult::Error("Failed to get total seconds: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult ClockScriptInterface::ExecuteGetFrameCount(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 1, "getFrameCount");
    if (!result.success) return result;

    try
    {
        Clock* clock = ExtractClockFromHandle(ScriptTypeExtractor::ExtractDouble(args[0]));
        if (!clock)
        {
            return ScriptMethodResult::Error("Invalid clock handle");
        }

        int frameCount = clock->GetFrameCount();
        return ScriptMethodResult::Success(static_cast<double>(frameCount));
    }
    catch (const std::exception& e)
    {
        return ScriptMethodResult::Error("Failed to get frame count: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
// VALIDATION AND HELPER METHODS
//----------------------------------------------------------------------------------------------------

bool ClockScriptInterface::ValidateTimeScale(float timeScale) const
{
    return (timeScale >= 0.0f && timeScale <= 10.0f);
}

//----------------------------------------------------------------------------------------------------
Clock* ClockScriptInterface::ExtractClockFromHandle(double handle) const
{
    return reinterpret_cast<Clock*>(static_cast<uintptr_t>(handle));
}
