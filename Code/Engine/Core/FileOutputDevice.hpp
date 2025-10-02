//----------------------------------------------------------------------------------------------------
// FileOutputDevice.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once

#include <fstream>
#include <mutex>
#include <string>

#include "Engine/Core/ILogOutputDevice.hpp"
#include "Engine/Core/StringUtils.hpp"

//----------------------------------------------------------------------------------------------------
// File output device (fallback when SmartFileOutputDevice fails)
//----------------------------------------------------------------------------------------------------
class FileOutputDevice : public ILogOutputDevice
{
private:
	std::ofstream      m_logFile;
	String             m_filePath;
	mutable std::mutex m_fileMutex;

public:
	explicit FileOutputDevice(const String& filePath);
	~FileOutputDevice();

	void WriteLog(const LogEntry& entry) override;
	void Flush() override;
	bool IsAvailable() const override;
};
