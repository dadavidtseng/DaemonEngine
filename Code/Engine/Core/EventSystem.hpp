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
    int priority = 0;
};

//----------------------------------------------------------------------------------------------------
struct EventSystemConfig
{
};

//----------------------------------------------------------------------------------------------------
struct EventSubscriptionComparator
{
    bool operator()(EventSubscription const& lhs, EventSubscription const& rhs) const
    {
        return lhs.priority > rhs.priority;     // higher priority will be executed first
    }
};

//----------------------------------------------------------------------------------------------------
typedef std::vector<EventSubscription> SubscriptionList;

//----------------------------------------------------------------------------------------------------
class EventSystem
{
public:
    explicit EventSystem(EventSystemConfig const& config);
    ~EventSystem();

    void Startup();
    void Shutdown();
    void BeginFrame();
    void EndFrame();

    void SubscribeEventCallbackFunction(String const& eventName, EventCallbackFunction functionPtr, int priority = 0);
    void UnsubscribeEventCallbackFunction(String const& eventName, EventCallbackFunction functionPtr);
    void FireEvent(String const& eventName, EventArgs& args);
    void FireEvent(String const& eventName);

    Strings GetAllRegisteredEventNames() const;

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
