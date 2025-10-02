//----------------------------------------------------------------------------------------------------
// DevConsoleOutputDevice.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once

#include "Engine/Core/ILogOutputDevice.hpp"

//----------------------------------------------------------------------------------------------------
// DaemonEngine developer console output device
//----------------------------------------------------------------------------------------------------
class DevConsoleOutputDevice : public ILogOutputDevice
{
public:
	void WriteLog(const LogEntry& entry) override;
	bool IsAvailable() const override;
};
