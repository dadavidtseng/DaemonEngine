//----------------------------------------------------------------------------------------------------
// Timer.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Core/Timer.hpp"

Timer::Timer(float period, Clock const*)
{
}

void Timer::Start()
{
}

void Timer::Stop()
{
}

float Timer::GetElapsedTime() const
{
    return 0.f;
}

float Timer::GetElapsedFraction() const
{
    return 0.f;
}

bool Timer::IsStopped() const
{
    return false;
}

bool Timer::HasPeriodElapsed() const
{
    return false;
}

bool Timer::DecrementPeriodIfElapsed()
{
    return false;
}
