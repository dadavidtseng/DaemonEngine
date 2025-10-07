//----------------------------------------------------------------------------------------------------
// OnScreenOutputDevice.cpp
//----------------------------------------------------------------------------------------------------

#include "Engine/Core/LogSubsystem.hpp"
#include "Engine/Core/OnScreenOutputDevice.hpp"

#include <algorithm>
#include <mutex>

//----------------------------------------------------------------------------------------------------
// OnScreenOutputDevice implementation
//----------------------------------------------------------------------------------------------------

void OnScreenOutputDevice::WriteLog(const LogEntry& entry)
{
	Rgba8 color;
	switch (entry.m_verbosity)
	{
	case eLogVerbosity::Fatal: color = Rgba8::RED;
		break;
	case eLogVerbosity::Error: color = Rgba8(255, 100, 100, 255);
		break;
	case eLogVerbosity::Warning: color = Rgba8::YELLOW;
		break;
	case eLogVerbosity::Display: color = Rgba8::GREEN;
		break;
	default: color = Rgba8::WHITE;
		break;
	}

	AddMessage("[" + entry.m_category + "] " + entry.m_message, 5.0f, color);
}

void OnScreenOutputDevice::Update(float deltaTime)
{
	std::lock_guard<std::mutex> lock(m_messagesMutex);

	for (auto it = m_messages.begin(); it != m_messages.end();)
	{
		it->remainingTime -= deltaTime;
		if (it->remainingTime <= 0.0f)
		{
			it = m_messages.erase(it);
		}
		else
		{
			++it;
		}
	}
}

void OnScreenOutputDevice::AddMessage(const String& message, float displayTime, const Rgba8& color, int uniqueId)
{
	std::lock_guard<std::mutex> lock(m_messagesMutex);

	// 如果指定了唯一 ID，先移除舊的相同 ID 訊息
	if (uniqueId >= 0)
	{
		m_messages.erase(
			std::remove_if(m_messages.begin(), m_messages.end(),
						   [uniqueId](const OnScreenMessage& msg) { return msg.uniqueId == uniqueId; }),
			m_messages.end());
	}

	OnScreenMessage newMessage;
	newMessage.message       = message;
	newMessage.displayTime   = displayTime;
	newMessage.remainingTime = displayTime;
	newMessage.color         = color;
	newMessage.uniqueId      = uniqueId >= 0 ? uniqueId : m_nextUniqueId++;

	m_messages.push_back(newMessage);
}

void OnScreenOutputDevice::ClearMessages()
{
	std::lock_guard<std::mutex> lock(m_messagesMutex);
	m_messages.clear();
}

void OnScreenOutputDevice::RenderMessages()
{
	// 這個方法需要與 DaemonEngine 的 Renderer 整合
	// 在實際使用時，需要呼叫 g_theRenderer 來渲染文字
	// 這裡只是提供介面
}
