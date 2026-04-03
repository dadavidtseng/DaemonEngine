//----------------------------------------------------------------------------------------------------
// EventSystem.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
//----------------------------------------------------------------------------------------------------
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/HashedCaseInsensitiveString.hpp"
#include "Engine/Core/NamedProperties.hpp"
#include "Engine/Core/StringUtils.hpp"
//----------------------------------------------------------------------------------------------------
#include <functional>
#include <map>
#include <mutex>


//----------------------------------------------------------------------------------------------------
using EventArgs             = NamedProperties;
using EventCallbackFunction = bool (*)(EventArgs& args);

//----------------------------------------------------------------------------------------------------
struct sEventSubscription
{
    EventCallbackFunction              callbackFunction = nullptr;
    void*                              objectInstance   = nullptr;
    std::function<bool(EventArgs&)>    memberCallback;
};

//----------------------------------------------------------------------------------------------------
struct sEventSystemConfig
{
};

//----------------------------------------------------------------------------------------------------
using SubscriptionList = std::vector<sEventSubscription>;

//----------------------------------------------------------------------------------------------------
class EventSystem
{
public:
    explicit EventSystem(sEventSystemConfig const& config);
    ~EventSystem() = default;

    void Startup();
    void Shutdown();
    void BeginFrame();
    void EndFrame();

    void SubscribeEventCallbackFunction(String const& eventName, EventCallbackFunction functionPtr);
    void UnsubscribeEventCallbackFunction(String const& eventName, EventCallbackFunction functionPtr);
    void FireEvent(String const& eventName, EventArgs& args);
    void FireEvent(String const& eventName);

    // Member function subscription
    template <typename ObjectType>
    void SubscribeEventCallbackObjectMethod(String const& eventName, ObjectType* object, bool (ObjectType::*method)(EventArgs&));

    template <typename ObjectType>
    void UnsubscribeEventCallbackObjectMethod(String const& eventName, ObjectType* object, bool (ObjectType::*method)(EventArgs&));

    void UnsubscribeAllEventCallbacksForObject(void* object);

    StringList GetAllRegisteredEventNames() const;

protected:
    sEventSystemConfig                 m_config;
    std::map<HashedCaseInsensitiveString, SubscriptionList> m_subscriptionsByEventName;
    mutable std::mutex                 m_subscriptionsMutex;  // Thread-safe access to m_subscriptionsByEventName
};

//----------------------------------------------------------------------------------------------------
// Standalone global-namespace helper functions; these forward to "the" event system, if it exists
//
void SubscribeEventCallbackFunction(String const& eventName, EventCallbackFunction functionPtr);
void UnsubscribeEventCallbackFunction(String const& eventName, EventCallbackFunction functionPtr);
void FireEvent(String const& eventName, EventArgs& args);
void FireEvent(String const& eventName);

void UnsubscribeAllEventCallbacksForObject(void* object);

//----------------------------------------------------------------------------------------------------
// Template implementations
//----------------------------------------------------------------------------------------------------
template <typename ObjectType>
void EventSystem::SubscribeEventCallbackObjectMethod(String const& eventName, ObjectType* object, bool (ObjectType::*method)(EventArgs&))
{
    std::lock_guard<std::mutex> lock(m_subscriptionsMutex);

    sEventSubscription newSubscription;
    newSubscription.objectInstance = static_cast<void*>(object);
    newSubscription.memberCallback = [object, method](EventArgs& args) -> bool
    {
        return (object->*method)(args);
    };

    HashedCaseInsensitiveString hcisName(eventName);
    m_subscriptionsByEventName[hcisName].push_back(newSubscription);
}

//----------------------------------------------------------------------------------------------------
template <typename ObjectType>
void EventSystem::UnsubscribeEventCallbackObjectMethod(String const& eventName, ObjectType* object, bool (ObjectType::*method)(EventArgs&))
{
    std::lock_guard<std::mutex> lock(m_subscriptionsMutex);

    HashedCaseInsensitiveString hcisName(eventName);
    auto iter = m_subscriptionsByEventName.find(hcisName);

    if (iter != m_subscriptionsByEventName.end())
    {
        SubscriptionList& subscriptions = iter->second;

        subscriptions.erase(
            std::remove_if(subscriptions.begin(), subscriptions.end(),
                           [object](sEventSubscription const& sub)
                           {
                               return sub.objectInstance == static_cast<void*>(object)
                                   && sub.callbackFunction == nullptr;
                           }),
            subscriptions.end()
        );

        if (subscriptions.empty())
        {
            m_subscriptionsByEventName.erase(iter);
        }
    }
}

//----------------------------------------------------------------------------------------------------
// Standalone template helpers (g_eventSystem declared in EngineCommon.hpp)
//----------------------------------------------------------------------------------------------------

template <typename ObjectType>
void SubscribeEventCallbackObjectMethod(String const& eventName, ObjectType* object, bool (ObjectType::*method)(EventArgs&))
{
    if (g_eventSystem)
    {
        g_eventSystem->SubscribeEventCallbackObjectMethod(eventName, object, method);
    }
}

//----------------------------------------------------------------------------------------------------
template <typename ObjectType>
void UnsubscribeEventCallbackObjectMethod(String const& eventName, ObjectType* object, bool (ObjectType::*method)(EventArgs&))
{
    if (g_eventSystem)
    {
        g_eventSystem->UnsubscribeEventCallbackObjectMethod(eventName, object, method);
    }
}
