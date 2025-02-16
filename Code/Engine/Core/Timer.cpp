//----------------------------------------------------------------------------------------------------
// Timer.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Core/Timer.hpp"

//----------------------------------------------------------------------------------------------------
// Create a clock with a period and the specified clock. If the clock
// is null, use the system clock.
//
Timer::Timer(float const period, Clock const* clock)
    : m_period(period),
      m_clock(clock ? clock : &Clock::GetSystemClock())

{
    if (clock == nullptr)
    {
        m_clock = &Clock::GetSystemClock();
    }
    else
    {
        m_clock = clock;
    }
}

//----------------------------------------------------------------------------------------------------
// Set the start time to the clock's current total time.
//
void Timer::Start()
{
    m_startTime = m_clock->GetTotalSeconds();
}

//----------------------------------------------------------------------------------------------------
// Sets the start time back to zero.
//
void Timer::Stop()
{
    m_startTime = 0.0;
}

//----------------------------------------------------------------------------------------------------
// Returns zero if stopped, otherwise returns the time elapsed between the clock's current
// time and our start time.
//
float Timer::GetElapsedTime() const
{
    if (IsStopped())
    {
        return 0.f;
    }

    return static_cast<float>(m_clock->GetTotalSeconds() - m_startTime);
}

//----------------------------------------------------------------------------------------------------
// Return the elapsed time as a percentage of our period. This can be greater than 1.
//
float Timer::GetElapsedFraction() const
{
    float const elapsedFraction = GetElapsedTime() / static_cast<float>(m_period);

    return elapsedFraction;
}

//----------------------------------------------------------------------------------------------------
// Returns true if our start time is zero.
//
bool Timer::IsStopped() const
{
    return m_startTime == 0.0;
}

//----------------------------------------------------------------------------------------------------
// Returns true if our elapsed time is greater than our period, and we are not stopped.
//
bool Timer::HasPeriodElapsed() const
{
    bool const periodElapsed = !IsStopped() && GetElapsedTime() >= m_period;

    return periodElapsed;
}

//----------------------------------------------------------------------------------------------------
// If a period has elapsed, and we are not stopped, decrements a period by adding a
// period to the start time and returns true. Generally called within a loop until it
// returns false so the caller can process each elapsed period.
//
bool Timer::DecrementPeriodIfElapsed()
{
    bool hasElapsed = false;

    while (HasPeriodElapsed())
    {
        m_startTime += m_period;
        hasElapsed = true;
    }

    return hasElapsed;
}
