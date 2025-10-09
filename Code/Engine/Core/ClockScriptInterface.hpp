//----------------------------------------------------------------------------------------------------
// ClockScriptInterface.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
//----------------------------------------------------------------------------------------------------
#include "Engine/Script/IScriptableObject.hpp"

//----------------------------------------------------------------------------------------------------

//-Forward-Declaration--------------------------------------------------------------------------------
class Clock;

//----------------------------------------------------------------------------------------------------
/// @brief JavaScript interface for Clock integration providing comprehensive time management control
///
/// @remark Exposes Clock functionality to JavaScript including pause control, time scaling,
///         frame stepping, and time query operations for gameplay programming.
///
/// @remark Implements method registry pattern for efficient JavaScript method dispatch and
///         provides type-safe parameter validation for all clock operations.
///
/// @see Clock for underlying hierarchical time management implementation
/// @see IScriptableObject for JavaScript integration framework
//----------------------------------------------------------------------------------------------------
class ClockScriptInterface : public IScriptableObject
{
public:
    /// @brief Construct ClockScriptInterface for clock creation and management
    ///
    /// @remark Automatically initializes method registry for efficient JavaScript dispatch.
    /// @remark Manages JavaScript-created clocks internally with automatic cleanup.
    ClockScriptInterface();

    /// @brief Destructor - cleans up all JavaScript-created clocks
    ~ClockScriptInterface() override;

    std::vector<ScriptMethodInfo> GetAvailableMethods() const override;
    StringList                    GetAvailableProperties() const override;

    ScriptMethodResult CallMethod(String const& methodName, ScriptArgs const& args) override;
    std::any           GetProperty(String const& propertyName) const override;
    bool               SetProperty(String const& propertyName, std::any const& value) override;

private:
    // === METHOD REGISTRY FOR EFFICIENT DISPATCH ===
    void InitializeMethodRegistry() override;

    // === CLOCK CREATION AND DESTRUCTION ===
    ScriptMethodResult ExecuteCreateClock(ScriptArgs const& args);
    ScriptMethodResult ExecuteDestroyClock(ScriptArgs const& args);

    // === PAUSE CONTROL METHODS ===
    ScriptMethodResult ExecutePause(ScriptArgs const& args);
    ScriptMethodResult ExecuteUnpause(ScriptArgs const& args);
    ScriptMethodResult ExecuteTogglePause(ScriptArgs const& args);
    ScriptMethodResult ExecuteIsPaused(ScriptArgs const& args);

    // === TIME CONTROL METHODS ===
    ScriptMethodResult ExecuteStepSingleFrame(ScriptArgs const& args);
    ScriptMethodResult ExecuteSetTimeScale(ScriptArgs const& args);
    ScriptMethodResult ExecuteReset(ScriptArgs const& args);

    // === TIME QUERY METHODS ===
    ScriptMethodResult ExecuteGetTimeScale(ScriptArgs const& args);
    ScriptMethodResult ExecuteGetDeltaSeconds(ScriptArgs const& args);
    ScriptMethodResult ExecuteGetTotalSeconds(ScriptArgs const& args);
    ScriptMethodResult ExecuteGetFrameCount(ScriptArgs const& args);

    // === VALIDATION ===
    bool ValidateTimeScale(float timeScale) const;

    // === HELPER METHODS ===
    Clock* ExtractClockFromHandle(double handle) const;

    // Clock storage for JavaScript-created clocks
    std::vector<Clock*> m_createdClocks;
};
