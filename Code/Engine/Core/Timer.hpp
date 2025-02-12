//----------------------------------------------------------------------------------------------------
// Timer.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include "Engine/Core/Clock.hpp"

//----------------------------------------------------------------------------------------------------
// Timer class that can be attached to any clock in a hierarchy and correctly handles duration
// regardless of update frequency.
//
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

    // The clock to use for getting the current time.
    Clock const* m_clock     = nullptr;

    // Clock time at which the timer was started. This is incremented by one period each
    // time we decrement a period, however, so it is not an absolute start time. It is
    // the start time of all periods that have not yet been decremented.
    double       m_startTime = 0.f;

    // The time interval it takes for a period to elapse.
    double       m_period    = 0.f;
};
