//----------------------------------------------------------------------------------------------------
// Clock.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Core/Clock.hpp"

#include "ErrorWarningAssert.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Math/MathUtils.hpp"

//----------------------------------------------------------------------------------------------------
STATIC Clock Clock::s_systemClock;

//----------------------------------------------------------------------------------------------------
// Default constructor, uses the system clock as the parent of the new clock.
//
Clock::Clock()
{
    m_parent = &GetSystemClock();
}

//----------------------------------------------------------------------------------------------------
// Constructor to specify a parent clock for the new clock.
//
Clock::Clock(Clock& parent)
{
    m_parent = &parent;
    m_parent->AddChild(this);
}

//----------------------------------------------------------------------------------------------------
// Destructor, un-parents ourselves and our children to avoid crashes but does not otherwise try
// to fix up the clock hierarchy. That is the responsibility of the user of this class.
//
Clock::~Clock()
{
    if (m_parent != nullptr)
    {
        m_parent->RemoveChild(this);
    }

    for (int i = 0, n = static_cast<int>(m_children.size()); i < n; ++i)
    {
        m_children[i]->m_parent = nullptr;
    }

    m_children.clear();
}

//----------------------------------------------------------------------------------------------------
// Reset all book keeping variables values back to zero and then set the last updated time
// to be the current system time.
//
void Clock::Reset()
{
    m_totalSeconds            = 0.0;
    m_deltaSeconds            = 0.0;
    m_frameCount              = 0;
    m_lastUpdateTimeInSeconds = GetCurrentTimeSeconds();
}

//----------------------------------------------------------------------------------------------------
bool Clock::IsPaused() const
{
    return m_isPaused;
}

//----------------------------------------------------------------------------------------------------
void Clock::Pause()
{
    m_isPaused = true;
}

//----------------------------------------------------------------------------------------------------
void Clock::Unpause()
{
    m_isPaused = false;
}

//----------------------------------------------------------------------------------------------------
void Clock::TogglePause()
{
    m_isPaused = !m_isPaused;
}

//----------------------------------------------------------------------------------------------------
// Unpause for frame then pause again the next frame.
//
void Clock::StepSingleFrame()
{
    m_stepSingleFrame = true;

    if (m_isPaused == false)
    {
        Pause();
    }

    Unpause();
}

//----------------------------------------------------------------------------------------------------
// Set and get the value by which this clock scales delta seconds.
//
void Clock::SetTimeScale(float const timeScale)
{
    m_timeScale = timeScale;
}

//----------------------------------------------------------------------------------------------------
float Clock::GetTimeScale() const
{
    return m_timeScale;
}

//----------------------------------------------------------------------------------------------------
double Clock::GetDeltaSeconds() const
{
    return m_deltaSeconds;
}

//----------------------------------------------------------------------------------------------------
double Clock::GetTotalSeconds() const
{
    return m_totalSeconds;
}

//----------------------------------------------------------------------------------------------------
int Clock::GetFrameCount() const
{
    return m_frameCount;
}

//----------------------------------------------------------------------------------------------------
// Returns a reference to a static system clock that by default will be the parent of all
// other clocks if a parent is not specified.
//
STATIC Clock& Clock::GetSystemClock()
{
    return s_systemClock;
}

//----------------------------------------------------------------------------------------------------
// Called in BeginFrame to Tick the system clock, which will in turn Advance the system
// clock, which will in turn Advance all of its children, thus updating the entire hierarchy.
//
void Clock::TickSystemClock()
{
    GetSystemClock().Tick();
}

//----------------------------------------------------------------------------------------------------
// Calculates the current delta seconds and clamps it to the max delta time, sets the last
// updated time, then calls Advance, passing down the delta seconds.
//
void Clock::Tick()
{
    double const currentSeconds    = GetCurrentTimeSeconds();
    double       deltaDeltaSeconds = currentSeconds - m_lastUpdateTimeInSeconds;
    m_lastUpdateTimeInSeconds      = currentSeconds;

    deltaDeltaSeconds = GetClamped(deltaDeltaSeconds, 0.0, m_maxDeltaSeconds);

    Advance(deltaDeltaSeconds);
}

//----------------------------------------------------------------------------------------------------
// Calculates delta seconds based on pausing and timescale, updates all remaining book
// keeping variables, calls Advance on all child clocks and passes down our delta seconds,
// and handles pausing after frames for stepping single frames.
//
void Clock::Advance(double const deltaTimeSeconds)
{
    if (m_isPaused == true)
    {
        m_timeScale    = 0.f;
    }

    m_deltaSeconds = deltaTimeSeconds * m_timeScale;
    m_totalSeconds += m_deltaSeconds;
    ++m_frameCount;

    if (!m_children.empty())
    {
        for (int i = 0; i < static_cast<int>(m_children.size()); ++i)
        {
            m_children[i]->Advance(m_deltaSeconds);
        }
    }

    if (m_stepSingleFrame == true)
    {
        m_stepSingleFrame = false;
        Pause();
    }
}

//----------------------------------------------------------------------------------------------------
// Add a child clock as one of our children. Does not handle cases where the child clock
// already has a parent.
//
void Clock::AddChild(Clock* childClock)
{
    if (childClock != nullptr)
    {
        m_children.push_back(childClock);
    }
}

//----------------------------------------------------------------------------------------------------
// Removes a child clock from our children if it is a child, otherwise does nothing.
//
void Clock::RemoveChild(Clock const* childClock)
{
    std::vector<Clock*>::iterator const it = std::find(m_children.begin(), m_children.end(), childClock);

    if (it != m_children.end())
    {
        m_children.erase(it);
    }
}
