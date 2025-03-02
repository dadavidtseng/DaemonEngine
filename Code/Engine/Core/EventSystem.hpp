//----------------------------------------------------------------------------------------------------
// EventSystem.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include <map>

#include "Engine/Core/StringUtils.hpp"

//-Forward-Declaration--------------------------------------------------------------------------------
class NamedStrings;

//----------------------------------------------------------------------------------------------------
typedef NamedStrings EventArgs;
typedef bool (*EventCallbackFunction)(EventArgs& args);

//----------------------------------------------------------------------------------------------------
struct EventSubscription
{
    EventCallbackFunction callbackFunction;
};

//----------------------------------------------------------------------------------------------------
struct EventSystemConfig
{
};

//----------------------------------------------------------------------------------------------------
typedef std::vector<EventSubscription> SubscriptionList;

//----------------------------------------------------------------------------------------------------
class EventSystem
{
public:
    explicit EventSystem(EventSystemConfig const& config);
    ~EventSystem() = default;

    void Startup();
    void Shutdown();
    void BeginFrame();
    void EndFrame();

    void SubscribeEventCallbackFunction(String const& eventName, EventCallbackFunction functionPtr);
    void UnsubscribeEventCallbackFunction(String const& eventName, EventCallbackFunction functionPtr);
    void FireEvent(String const& eventName, EventArgs& args);
    void FireEvent(String const& eventName);

    StringList GetAllRegisteredEventNames() const;

protected:
    EventSystemConfig m_config;
    std::map<String, SubscriptionList> m_subscriptionsByEventName;
};

//----------------------------------------------------------------------------------------------------
// Standalone global-namespace helper functions; these forward to "the" event system, if it exists
//
void SubscribeEventCallbackFunction(String const& eventName, EventCallbackFunction functionPtr);
void UnsubscribeEventCallbackFunction(String const& eventName, EventCallbackFunction functionPtr);
void FireEvent(String const& eventName, EventArgs& args);
void FireEvent(String const& eventName);
