//----------------------------------------------------------------------------------------------------
// FrameEventQueueScriptInterface.cpp
// JavaScript Interface for FrameEventQueue Implementation
//----------------------------------------------------------------------------------------------------

#include "Engine/Core/FrameEventQueueScriptInterface.hpp"

#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/FrameEventQueue.hpp"

#include "ThirdParty/json/json.hpp"

//----------------------------------------------------------------------------------------------------
FrameEventQueueScriptInterface::FrameEventQueueScriptInterface(FrameEventQueue* frameEventQueue)
    : m_frameEventQueue(frameEventQueue)
{
    if (!m_frameEventQueue)
    {
        ERROR_AND_DIE("FrameEventQueueScriptInterface: FrameEventQueue pointer cannot be null");
    }

    FrameEventQueueScriptInterface::InitializeMethodRegistry();
}

//----------------------------------------------------------------------------------------------------
void FrameEventQueueScriptInterface::InitializeMethodRegistry()
{
    m_methodRegistry["drainAll"] = [this](ScriptArgs const& args) { return ExecuteDrainAll(args); };
}

//----------------------------------------------------------------------------------------------------
std::vector<ScriptMethodInfo> FrameEventQueueScriptInterface::GetAvailableMethods() const
{
    return {
        ScriptMethodInfo("drainAll",
                         "Drain all frame events and return as JSON array",
                         {},
                         "array")
    };
}

//----------------------------------------------------------------------------------------------------
StringList FrameEventQueueScriptInterface::GetAvailableProperties() const
{
    return {};
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult FrameEventQueueScriptInterface::CallMethod(String const&     methodName,
                                                               ScriptArgs const& args)
{
    try
    {
        auto it = m_methodRegistry.find(methodName);
        if (it != m_methodRegistry.end())
        {
            return it->second(args);
        }

        return ScriptMethodResult::Error("Unknown method: " + methodName);
    }
    catch (std::exception const& e)
    {
        return ScriptMethodResult::Error("Method execution exception: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
std::any FrameEventQueueScriptInterface::GetProperty(const String& propertyName) const
{
    UNUSED(propertyName)
    return std::any{};
}

//----------------------------------------------------------------------------------------------------
bool FrameEventQueueScriptInterface::SetProperty(const String& propertyName, const std::any& value)
{
    UNUSED(propertyName)
    UNUSED(value)
    return false;
}

//----------------------------------------------------------------------------------------------------
// ExecuteDrainAll
//
// Drains all events from the FrameEventQueue and returns them as a JSON array.
// Each event is a JSON object with a "type" field and type-specific data.
//
// Returns:
//   JSON array string, e.g.:
//   [{"type":"keyDown","keyCode":65},{"type":"cursorUpdate","x":400,"y":300,"dx":2,"dy":-1}]
//----------------------------------------------------------------------------------------------------
ScriptMethodResult FrameEventQueueScriptInterface::ExecuteDrainAll(ScriptArgs const& args)
{
    if (!args.empty())
    {
        return ScriptMethodResult::Error("drainAll() requires no arguments");
    }

    nlohmann::json eventsArray = nlohmann::json::array();

    m_frameEventQueue->ConsumeAll([&](FrameEvent const& evt)
    {
        nlohmann::json eventJson;

        switch (evt.type)
        {
        case eFrameEventType::KeyDown:
            eventJson["type"]    = "keyDown";
            eventJson["keyCode"] = evt.key.keyCode;
            break;

        case eFrameEventType::KeyUp:
            eventJson["type"]    = "keyUp";
            eventJson["keyCode"] = evt.key.keyCode;
            break;

        case eFrameEventType::MouseButtonDown:
            eventJson["type"]    = "mouseButtonDown";
            eventJson["keyCode"] = evt.key.keyCode;
            break;

        case eFrameEventType::MouseButtonUp:
            eventJson["type"]    = "mouseButtonUp";
            eventJson["keyCode"] = evt.key.keyCode;
            break;

        case eFrameEventType::CursorUpdate:
            eventJson["type"] = "cursorUpdate";
            eventJson["x"]    = evt.cursor.x;
            eventJson["y"]    = evt.cursor.y;
            eventJson["dx"]   = evt.cursor.dx;
            eventJson["dy"]   = evt.cursor.dy;
            break;
        }

        eventsArray.push_back(eventJson);
    });

    String eventsJson = eventsArray.dump();
    if (eventsJson.empty())
    {
        eventsJson = "[]";
    }

    return ScriptMethodResult::Success(eventsJson);
}

