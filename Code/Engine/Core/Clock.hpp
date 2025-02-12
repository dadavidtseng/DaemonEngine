//----------------------------------------------------------------------------------------------------
// Clock.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include <vector>

//----------------------------------------------------------------------------------------------------
// Hierarchical clock that inherits time scale. Parent clocks pass scaled delta seconds down to
// child clocks to be used as their base delta seconds. Child clocks in turn scale that time and
// pass that down to their children. There is one system clock at the root of the hierarchy.
//
class Clock
{
public:
    Clock();
    explicit Clock(Clock& parent);
    ~Clock();
    Clock(Clock const& copy);

    void Reset();
    bool IsPaused() const;
    void Pause();
    void Unpause();
    void TogglePause();

    void  StepSingleFrame();
    void  SetTimeScale(float timeScale);
    float GetTimeScale() const;

    float GetDeltaSeconds() const;
    float GetTotalSeconds() const;
    int   GetFrameCount() const;

    static Clock& GetSystemClock();
    static void   TickSystemClock();

protected:
    void Tick();
    void Advance(double deltaTimeSeconds);
    void AddChild(Clock* childClock);
    void RemoveChild(Clock* childClock);

    // Parent clock. Will be nullptr for the root clock.
    Clock*              m_parent = nullptr;

    // All children of this clock.
    std::vector<Clock*> m_children;

    // Book keeping variables.
    double m_lastUpdateTimeInSeconds = 0.f;
    double m_totalSeconds = 0.f;
    double m_deltaSeconds = 0.f;
    int m_frameCount = 0;

    // Time scale for this clock.
    double m_timeScale = 1.f;

    // Pauses the clock completely.
    bool m_isPaused = false;

    // For single stepping frames.
    bool m_stepSingleFrame = false;

    // Max delta time. Useful for preventing large time steps when stepping in a debugger.
    double m_maxDeltaSeconds = 0.1f;
};
