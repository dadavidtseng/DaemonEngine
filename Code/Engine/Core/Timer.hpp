//----------------------------------------------------------------------------------------------------
// Timer.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include "Engine/Core/Clock.hpp"

//----------------------------------------------------------------------------------------------------
// Timer class that can be attached to any clock in a hierarchy and correctly handles duration
// regardless of update frequency.
class Timer
{
    explicit Timer(float period, Clock const* = nullptr);
    void     Start();
    void     Stop();
    float    GetElapsedTime() const;
    float    GetElapsedFraction() const;
    bool     IsStopped() const;
    bool     HasPeriodElapsed() const;
    bool     DecrementPeriodIfElapsed();

    Clock const* m_clock     = nullptr;
    double       m_startTime = 0.f;
    double       m_period    = 0.f;
};
