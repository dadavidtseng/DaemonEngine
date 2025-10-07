//----------------------------------------------------------------------------------------------------
// FileOutputDevice.cpp
//----------------------------------------------------------------------------------------------------

#include "Engine/Core/LogSubsystem.hpp"
#include "Engine/Core/FileOutputDevice.hpp"

#include <fstream>
#include <mutex>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

//----------------------------------------------------------------------------------------------------
// FileOutputDevice implementation
//----------------------------------------------------------------------------------------------------

FileOutputDevice::FileOutputDevice(const String& filePath)
	: m_filePath(filePath)
{
	// 嘗試直接開啟檔案
	m_logFile.open(filePath, std::ios::out | std::ios::app);

	if (!m_logFile.is_open())
	{
		// 如果是 Logs/ 開頭的路徑，嘗試先建立目錄
		if (filePath.find("Logs/") == 0 || filePath.find("Logs\\") == 0)
		{
#ifdef _WIN32
			CreateDirectoryA("Logs", nullptr);
#else
			system("mkdir -p Logs");
#endif
			// 再次嘗試開啟原始路徑
			m_logFile.open(filePath, std::ios::out | std::ios::app);
		}

		// 如果還是失敗，在當前目錄建立
		if (!m_logFile.is_open())
		{
			m_logFile.open("DaemonEngine.log", std::ios::out | std::ios::app);
		}
	}
}

FileOutputDevice::~FileOutputDevice()
{
	if (m_logFile.is_open())
	{
		m_logFile.close();
	}
}

void FileOutputDevice::WriteLog(const LogEntry& entry)
{
	std::lock_guard lock(m_fileMutex);

	if (m_logFile.is_open())
	{
		m_logFile << "[" << entry.m_timestamp << "] "
		<< "[" << entry.m_threadId << "] "
		<< "[" << entry.m_category << "] ";

		// 轉換詳細程度為字串
		const char* verbosityStr = "Unknown";
		switch (entry.m_verbosity)
		{
		case eLogVerbosity::Fatal: verbosityStr = "Fatal";
			break;
		case eLogVerbosity::Error: verbosityStr = "Error";
			break;
		case eLogVerbosity::Warning: verbosityStr = "Warning";
			break;
		case eLogVerbosity::Display: verbosityStr = "Display";
			break;
		case eLogVerbosity::Log: verbosityStr = "Log";
			break;
		case eLogVerbosity::Verbose: verbosityStr = "Verbose";
			break;
		case eLogVerbosity::VeryVerbose: verbosityStr = "VeryVerbose";
			break;
		}

		m_logFile << "[" << verbosityStr << "] " << entry.m_message;

		// 如果有檔案和行號資訊，也記錄下來
		if (!entry.m_fileName.empty() && entry.m_lineNum > 0)
		{
			m_logFile << " (" << entry.m_fileName << ":" << entry.m_lineNum << ")";
		}

		m_logFile << std::endl;
	}
}

void FileOutputDevice::Flush()
{
	std::lock_guard<std::mutex> lock(m_fileMutex);
	if (m_logFile.is_open())
	{
		m_logFile.flush();
	}
}

bool FileOutputDevice::IsAvailable() const
{
	return m_logFile.is_open();
}
