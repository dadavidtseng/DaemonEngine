//----------------------------------------------------------------------------------------------------
// Timer.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Core/Timer.hpp"

//----------------------------------------------------------------------------------------------------
// Create a clock with a period and the specified clock. If the clock
// is null, use the system clock.
//
Timer::Timer(float period, Clock const*)
{
}

//----------------------------------------------------------------------------------------------------
// Set the start time to the clock's current total time.
//
void Timer::Start()
{
}

//----------------------------------------------------------------------------------------------------
// Sets the start time back to zero.
//
void Timer::Stop()
{
}

//----------------------------------------------------------------------------------------------------
// Returns zero if stopped, otherwise returns the time elapsed between the clock's current
// time and our start time.
//
float Timer::GetElapsedTime() const
{
    return 0.f;
}

//----------------------------------------------------------------------------------------------------
// Return the elapsed time as a percentage of our period. This can be greater than 1.
//
float Timer::GetElapsedFraction() const
{
    return 0.f;
}

//----------------------------------------------------------------------------------------------------
// Returns true if our start time is zero.
//
bool Timer::IsStopped() const
{
    return false;
}

//----------------------------------------------------------------------------------------------------
// Returns true if our elapsed time is greater than our period and we are not stopped.
//
bool Timer::HasPeriodElapsed() const
{
    return false;
}

//----------------------------------------------------------------------------------------------------
// If a period has elapsed, and we are not stopped, decrements a period by adding a
// period to the start time and returns true. Generally called within a loop until it
// returns false so the caller can process each elapsed period.
//
bool Timer::DecrementPeriodIfElapsed()
{
    return false;
}
