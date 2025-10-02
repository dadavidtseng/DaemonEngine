//----------------------------------------------------------------------------------------------------
// OnScreenOutputDevice.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once

#include <mutex>
#include <vector>

#include "Engine/Core/ILogOutputDevice.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/StringUtils.hpp"

//----------------------------------------------------------------------------------------------------
// On-screen output device
//----------------------------------------------------------------------------------------------------
class OnScreenOutputDevice : public ILogOutputDevice
{
private:
	struct OnScreenMessage
	{
		String message;
		float  displayTime;
		float  remainingTime;
		Rgba8  color;
		int    uniqueId;
	};

	std::vector<OnScreenMessage> m_messages;
	mutable std::mutex           m_messagesMutex;
	int                          m_nextUniqueId = 0;

public:
	void WriteLog(const LogEntry& entry) override;
	void Update(float deltaTime);
	void RenderMessages(); // 需要與 Renderer 整合
	void AddMessage(const String& message, float displayTime, const Rgba8& color, int uniqueId = -1);
	void ClearMessages();
};
