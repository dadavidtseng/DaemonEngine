//----------------------------------------------------------------------------------------------------
// DebugOutputDevice.cpp
//----------------------------------------------------------------------------------------------------

#include "Engine/Core/LogSubsystem.hpp"
#include "Engine/Core/DebugOutputDevice.hpp"

#include "Engine/Core/StringUtils.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

//----------------------------------------------------------------------------------------------------
// DebugOutputDevice implementation
//----------------------------------------------------------------------------------------------------

void DebugOutputDevice::WriteLog(const LogEntry& entry)
{
#ifdef _WIN32
	String outputString = "[" + entry.category + "] " + entry.message + "\n";
	OutputDebugStringA(outputString.c_str());
#endif
}

bool DebugOutputDevice::IsAvailable() const
{
#ifdef _WIN32
	return IsDebuggerPresent();
#else
	return false;
#endif
}
