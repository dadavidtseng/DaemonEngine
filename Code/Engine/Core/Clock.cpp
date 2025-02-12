//----------------------------------------------------------------------------------------------------
// Clock.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Core/Clock.hpp"

#include "EngineCommon.hpp"

//----------------------------------------------------------------------------------------------------
// Default constructor, uses the system clock as the parent of the new clock.
//
Clock::Clock()
{
}

//----------------------------------------------------------------------------------------------------
// Constructor to specify a parent clock for the new clock.
//
Clock::Clock(Clock& parent)
{
}

//----------------------------------------------------------------------------------------------------
// Destructor, unparents ourselves and our children to avoid crashes but does not otherwise try
// to fix up the clock hierarchy. That is the responsibility of the user of this class.
//
Clock::~Clock()
{
}

//----------------------------------------------------------------------------------------------------
Clock::Clock(Clock const& copy)
{
}

//----------------------------------------------------------------------------------------------------
// Reset all book keeping variables values back to zero and then setr the last updated time
// to be the current system time.
//
void Clock::Reset()
{
}

//----------------------------------------------------------------------------------------------------
bool Clock::IsPaused() const
{
    return false;
}

//----------------------------------------------------------------------------------------------------
void Clock::Pause()
{
}

//----------------------------------------------------------------------------------------------------
void Clock::Unpause()
{
}

//----------------------------------------------------------------------------------------------------
void Clock::TogglePause()
{
}

//----------------------------------------------------------------------------------------------------
// Unpause for frame then pause again the next frame.
//
void Clock::StepSingleFrame()
{
}

//----------------------------------------------------------------------------------------------------
// Set and get the value by which this clock scales delta seconds.
//
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

//----------------------------------------------------------------------------------------------------
// Returns a reference to a static system clock that by default will be the parent of all
// other clocks if a parent is not specified.
//
STATIC Clock& Clock::GetSystemClock()
{
    static Clock systemClock;

    return systemClock;
}

//----------------------------------------------------------------------------------------------------
// Called in BeginFrame to Tick the system clock, which will in turn Advance the system
// clock, which will in turn Advance all of its children, thus updating the entire hierarchy.
//
void Clock::TickSystemClock()
{
}

//----------------------------------------------------------------------------------------------------
// Calculates the current delta seconds and clamps it to the max delta time, sets the last
// updated time, then calls Advance, passing down the delta seconds.
//
void Clock::Tick()
{
}

//----------------------------------------------------------------------------------------------------
// Calculates delta seconds based on pausing and time scale, updates all remaining book
// keeping variables, calls Advance on all child clocks and passes down our delta seconds,
// and handles pausing after frames for stepping single frames.
//
void Clock::Advance(double deltaTimeSeconds)
{
}

//----------------------------------------------------------------------------------------------------
// Add a child clock as one of our children. Does not handle cases where the child clock
// already has a parent.
//
void Clock::AddChild(Clock* childClock)
{
}

//----------------------------------------------------------------------------------------------------
// Removes a child clock from our children if it is a child, otherwise does nothing.
//
void Clock::RemoveChild(Clock* childClock)
{
}
