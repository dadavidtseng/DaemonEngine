//----------------------------------------------------------------------------------------------------
// Clock.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include <vector>

//----------------------------------------------------------------------------------------------------
// Hierarchical clock that inherits timescale. Parent clocks pass scaled delta seconds down to
// child clocks to be used as their base delta seconds. Child clocks in turn scale that time and
// pass that down to their children. There is one system clock at the root of the hierarchy.
//
class Clock
{
public:
    Clock();
    explicit Clock(Clock& parent);
    ~Clock();
    Clock(Clock const& copy) = delete;

    void Reset();
    bool IsPaused() const;
    void Pause();
    void Unpause();
    void TogglePause();

    void StepSingleFrame();
    void SetTimeScale(float timeScale);

    float  GetTimeScale() const;
    double GetDeltaSeconds() const;
    double GetTotalSeconds() const;
    int    GetFrameCount() const;

    static Clock& GetSystemClock();
    static void   TickSystemClock();
    static Clock  s_systemClock;

protected:
    void Tick();
    void Advance(double deltaTimeSeconds);
    void AddChild(Clock* childClock);
    void RemoveChild(Clock const* childClock);

    Clock*              m_parent = nullptr;                 // Parent clock. Will be nullptr for the root clock(system clock).
    std::vector<Clock*> m_children;                         // All children of this clock.
    double              m_lastUpdateTimeInSeconds = 0.0;    // Bookkeeping variables.
    double              m_totalSeconds            = 0.0;    // Bookkeeping variables.
    double              m_deltaSeconds            = 0.0;    // Bookkeeping variables.
    int                 m_frameCount              = 0;      // Bookkeeping variables.
    float               m_timeScale               = 1.f;    // Timescale for this clock.
    bool                m_isPaused                = false;  // Pauses the clock completely.
    bool                m_stepSingleFrame         = false;  // For single stepping frames.
    double              m_maxDeltaSeconds         = 0.1;    // Max delta time. Useful for preventing large time steps when stepping in a debugger.
};
