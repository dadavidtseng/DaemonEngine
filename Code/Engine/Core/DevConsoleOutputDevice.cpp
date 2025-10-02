//----------------------------------------------------------------------------------------------------
// DevConsoleOutputDevice.cpp
//----------------------------------------------------------------------------------------------------

#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/LogSubsystem.hpp"
#include "Engine/Core/DevConsoleOutputDevice.hpp"

#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/StringUtils.hpp"

//----------------------------------------------------------------------------------------------------
// DevConsoleOutputDevice implementation
//----------------------------------------------------------------------------------------------------

void DevConsoleOutputDevice::WriteLog(const LogEntry& entry)
{
	if (g_devConsole)
	{
		// 根據詳細程度選擇適當的顏色類型
		Rgba8 lineType = DevConsole::INFO_MINOR;
		switch (entry.verbosity)
		{
		case eLogVerbosity::Fatal:
		case eLogVerbosity::Error:
			lineType = DevConsole::ERROR;
			break;
		case eLogVerbosity::Warning:
			lineType = DevConsole::WARNING;
			break;
		case eLogVerbosity::Display:
			lineType = DevConsole::INFO_MAJOR;
			break;
		default:
			lineType = DevConsole::INFO_MINOR;
			break;
		}

		g_devConsole->AddLine(lineType, "[" + entry.category + "] " + entry.message);
	}
}

bool DevConsoleOutputDevice::IsAvailable() const
{
	return g_devConsole != nullptr;
}
