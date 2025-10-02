//----------------------------------------------------------------------------------------------------
// ConsoleOutputDevice.cpp
//----------------------------------------------------------------------------------------------------

#include "Engine/Core/LogSubsystem.hpp"
#include "Engine/Core/ConsoleOutputDevice.hpp"

#include <cstdio>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

//----------------------------------------------------------------------------------------------------
// External global variables
//----------------------------------------------------------------------------------------------------
extern HANDLE g_consoleHandle;

//----------------------------------------------------------------------------------------------------
// ConsoleOutputDevice implementation
//----------------------------------------------------------------------------------------------------

void ConsoleOutputDevice::WriteLog(const LogEntry& entry)
{
	// 使用現有的 Window 控制台控制代碼 (Windows)
#ifdef _WIN32
	HANDLE hConsole = g_consoleHandle ? g_consoleHandle : GetStdHandle(STD_OUTPUT_HANDLE);

	// 檢查控制台是否可用
	if (hConsole && hConsole != INVALID_HANDLE_VALUE)
	{
		WORD                       originalAttributes = 0;
		CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
		if (GetConsoleScreenBufferInfo(hConsole, &consoleInfo))
		{
			originalAttributes = consoleInfo.wAttributes;
		}

		// 根據詳細程度設定顏色
		WORD color = originalAttributes;
		switch (entry.verbosity)
		{
		case eLogVerbosity::Fatal:
			color = BACKGROUND_RED | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
			break;
		case eLogVerbosity::Error:
			color = FOREGROUND_RED | FOREGROUND_INTENSITY;
			break;
		case eLogVerbosity::Warning:
			color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
			break;
		case eLogVerbosity::Display:
			color = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
			break;
		case eLogVerbosity::Log:
			color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE; // 白色
			break;
		case eLogVerbosity::Verbose:
			color = FOREGROUND_BLUE | FOREGROUND_GREEN;
			break;
		case eLogVerbosity::VeryVerbose:
			color = FOREGROUND_BLUE;
			break;
		default:
			color = originalAttributes;
			break;
		}

		SetConsoleTextAttribute(hConsole, color);

		// 格式化輸出（與您原始的 printf 風格一致）
		printf("[%s] [%s] %s\n",
			   entry.timestamp.c_str(),
			   entry.category.c_str(),
			   entry.message.c_str());

		SetConsoleTextAttribute(hConsole, originalAttributes);
	}
	else
	{
		// 如果沒有控制台控制代碼，使用標準輸出
		printf("[%s] [%s] %s\n",
			   entry.timestamp.c_str(),
			   entry.category.c_str(),
			   entry.message.c_str());
	}
#else
	// Unix/Linux 系統使用 ANSI 顏色碼
	const char* colorCode = "\033[0m"; // 預設白色
	switch (entry.verbosity)
	{
	case eLogVerbosity::Fatal: colorCode = "\033[41;37;1m";
		break; // 紅底白字
	case eLogVerbosity::Error: colorCode = "\033[31;1m";
		break;    // 亮紅色
	case eLogVerbosity::Warning: colorCode = "\033[33;1m";
		break;    // 亮黃色
	case eLogVerbosity::Display: colorCode = "\033[32;1m";
		break;    // 亮綠色
	case eLogVerbosity::Log: colorCode = "\033[37m";
		break;      // 白色
	case eLogVerbosity::Verbose: colorCode = "\033[36m";
		break;      // 青色
	case eLogVerbosity::VeryVerbose: colorCode = "\033[34m";
		break;     // 藍色
	}

	printf("%s[%s] [%s] %s\033[0m\n",
		   colorCode,
		   entry.timestamp.c_str(),
		   entry.category.c_str(),
		   entry.message.c_str());
#endif
}

Rgba8 ConsoleOutputDevice::GetVerbosityColor(eLogVerbosity verbosity) const
{
	switch (verbosity)
	{
	case eLogVerbosity::Fatal: return Rgba8::RED;
	case eLogVerbosity::Error: return Rgba8(255, 100, 100, 255);
	case eLogVerbosity::Warning: return Rgba8::YELLOW;
	case eLogVerbosity::Display: return Rgba8::GREEN;
	case eLogVerbosity::Log: return Rgba8::WHITE;
	case eLogVerbosity::Verbose: return Rgba8(200, 200, 200, 255);
	case eLogVerbosity::VeryVerbose: return Rgba8(150, 150, 150, 255);
	default: return Rgba8::WHITE;
	}
}
