//----------------------------------------------------------------------------------------------------
// Clock.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Core/Clock.hpp"

Clock::Clock()
{
}

Clock::Clock(Clock& parent)
{
}

Clock::~Clock()
{
}

Clock::Clock(Clock const& copy)
{
}

void Clock::Reset()
{
}

bool Clock::IsPaused() const
{
    return false;
}

void Clock::Pause()
{
}

void Clock::Unpause()
{
}

void Clock::TogglePause()
{
}

void Clock::StepSingleFrame()
{
}

void Clock::SetTimeScale(float timeScale)
{
}

float Clock::GetTimeScale() const
{
    return 0.f;
}

float Clock::GetDeltaSeconds() const
{
    return 0.f;
}

float Clock::GetTotalSeconds() const
{
    return 0.f;
}

int Clock::GetFrameCount() const
{
    return 0;
}

Clock& Clock::GetSystemClock()
{
    static Clock systemClock;

    return systemClock;
}

void Clock::TickSystemClock()
{
}

void Clock::Tick()
{
}

void Clock::Advance(double deltaTimeSeconds)
{
}

void Clock::AddChild(Clock* childClock)
{
}

void Clock::RemoveChild(Clock* childClock)
{
}
