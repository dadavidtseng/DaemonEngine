//----------------------------------------------------------------------------------------------------
// Time.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Core/Time.hpp"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

//----------------------------------------------------------------------------------------------------
double InitializeTime(LARGE_INTEGER& out_initialTime)
{
    LARGE_INTEGER countsPerSecond;

    QueryPerformanceFrequency(&countsPerSecond);
    QueryPerformanceCounter(&out_initialTime);

    return 1.0 / static_cast<double>(countsPerSecond.QuadPart);
}

//----------------------------------------------------------------------------------------------------
double GetCurrentTimeSeconds()
{
    static LARGE_INTEGER initialTime;
    static double        secondsPerCount = InitializeTime(initialTime);

    LARGE_INTEGER currentCount;
    QueryPerformanceCounter(&currentCount);

    LONGLONG const elapsedCountsSinceInitialTime = currentCount.QuadPart - initialTime.QuadPart;
    double const   currentSeconds                = static_cast<double>(elapsedCountsSinceInitialTime) * secondsPerCount;

    return currentSeconds;
}
