//----------------------------------------------------------------------------------------------------
// ConsoleOutputDevice.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once

#include "Engine/Core/ILogOutputDevice.hpp"
#include "Engine/Core/Rgba8.hpp"

//----------------------------------------------------------------------------------------------------
// Console output device
//----------------------------------------------------------------------------------------------------
class ConsoleOutputDevice : public ILogOutputDevice
{
public:
    void  WriteLog(const LogEntry& entry) override;
    Rgba8 GetVerbosityColor(eLogVerbosity verbosity) const;
};