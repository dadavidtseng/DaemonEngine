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
ClockScriptInterface::ClockScriptInterface(Clock* clock)
    : m_clock(clock)
{
    if (!m_clock)
    {
        ERROR_AND_DIE("ClockScriptInterface: Clock pointer cannot be null")
    }

    // Initialize method registry for efficient dispatch
    ClockScriptInterface::InitializeMethodRegistry();
}

//----------------------------------------------------------------------------------------------------
void ClockScriptInterface::InitializeMethodRegistry()
{
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
        // === PAUSE CONTROL METHODS ===
        ScriptMethodInfo("pause",
                         "Pause the clock, stopping time progression",
                         {},
                         "void"),

        ScriptMethodInfo("unpause",
                         "Unpause the clock, resuming time progression",
                         {},
                         "void"),

        ScriptMethodInfo("togglePause",
                         "Toggle pause state of the clock",
                         {},
                         "void"),

        ScriptMethodInfo("isPaused",
                         "Check if the clock is currently paused",
                         {},
                         "bool"),

        // === TIME CONTROL METHODS ===
        ScriptMethodInfo("stepSingleFrame",
                         "Advance the clock by a single frame while paused",
                         {},
                         "void"),

        ScriptMethodInfo("setTimeScale",
                         "Set time scale multiplier (1.0 = normal, 0.5 = slow motion, 2.0 = fast forward)",
                         {"number"},
                         "void"),

        ScriptMethodInfo("reset",
                         "Reset the clock to initial state",
                         {},
                         "void"),

        // === TIME QUERY METHODS ===
        ScriptMethodInfo("getTimeScale",
                         "Get current time scale multiplier",
                         {},
                         "number"),

        ScriptMethodInfo("getDeltaSeconds",
                         "Get time elapsed since last frame in seconds",
                         {},
                         "number"),

        ScriptMethodInfo("getTotalSeconds",
                         "Get total accumulated time in seconds",
                         {},
                         "number"),

        ScriptMethodInfo("getFrameCount",
                         "Get total number of frames processed by this clock",
                         {},
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
// PAUSE CONTROL METHODS
//----------------------------------------------------------------------------------------------------

ScriptMethodResult ClockScriptInterface::ExecutePause(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 0, "pause");
    if (!result.success) return result;

    try
    {
        m_clock->Pause();
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
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 0, "unpause");
    if (!result.success) return result;

    try
    {
        m_clock->Unpause();
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
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 0, "togglePause");
    if (!result.success) return result;

    try
    {
        m_clock->TogglePause();
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
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 0, "isPaused");
    if (!result.success) return result;

    try
    {
        bool isPaused = m_clock->IsPaused();
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
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 0, "stepSingleFrame");
    if (!result.success) return result;

    try
    {
        m_clock->StepSingleFrame();
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
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 1, "setTimeScale");
    if (!result.success) return result;

    try
    {
        float timeScale = ScriptTypeExtractor::ExtractFloat(args[0]);

        if (!ValidateTimeScale(timeScale))
        {
            return ScriptMethodResult::Error("Time scale must be between 0.0 and 10.0");
        }

        m_clock->SetTimeScale(timeScale);
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
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 0, "reset");
    if (!result.success) return result;

    try
    {
        m_clock->Reset();
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
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 0, "getTimeScale");
    if (!result.success) return result;

    try
    {
        float timeScale = m_clock->GetTimeScale();
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
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 0, "getDeltaSeconds");
    if (!result.success) return result;

    try
    {
        double deltaSeconds = m_clock->GetDeltaSeconds();
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
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 0, "getTotalSeconds");
    if (!result.success) return result;

    try
    {
        double totalSeconds = m_clock->GetTotalSeconds();
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
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 0, "getFrameCount");
    if (!result.success) return result;

    try
    {
        int frameCount = m_clock->GetFrameCount();
        return ScriptMethodResult::Success(static_cast<double>(frameCount));
    }
    catch (const std::exception& e)
    {
        return ScriptMethodResult::Error("Failed to get frame count: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
// VALIDATION
//----------------------------------------------------------------------------------------------------

bool ClockScriptInterface::ValidateTimeScale(float timeScale) const
{
    return (timeScale >= 0.0f && timeScale <= 10.0f);
}
