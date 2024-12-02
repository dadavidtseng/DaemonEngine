//----------------------------------------------------------------------------------------------------
// EventSystem.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include <map>

#include "Engine/Core/StringUtils.hpp"

//----------------------------------------------------------------------------------------------------
typedef std::vector<EventSubscription> SubscriptionList;

//----------------------------------------------------------------------------------------------------
class EventSystem
{
public:
    EventSystem(EventSystemConfig const& config);
    ~EventSystem();
    void Startup();
    void Shutdown();
    void BeginFrame();
    void EndFrame();

    void SubscribeEventCallbackFunction(String const& eventName, EventCallbackFunction functionPtr);
    void UnsubscribeEventCallbackFunction(String const& eventName, EventCallbackFunction functionPrt);

protected:
    EventSystemConfig                  m_config;
    std::map<String, SubscriptionList> m_subscriptionsByEventName;
};

//----------------------------------------------------------------------------------------------------
//  Standalone global-namespace helper functions; these forward to "the" event system, if it exists
//
void SubscribeEventCallbackFunction(String const& eventName, EventCallbackFunction functionPtr);
void UnsubscribeEventCallbackFunction(String const& eventName, EventCallbackFunction functionPrt);
void FireEvent(String const& eventName, EventArgs& args);
void FireEvent(String const& eventName);