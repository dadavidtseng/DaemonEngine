//----------------------------------------------------------------------------------------------------
// EventSystem.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Core/EventSystem.hpp"

#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/LogSubsystem.hpp"

//----------------------------------------------------------------------------------------------------
EventSystem* g_eventSystem = nullptr;

//----------------------------------------------------------------------------------------------------
EventSystem::EventSystem(sEventSystemConfig const& config)
    : m_config(config)
{
}

//----------------------------------------------------------------------------------------------------
void EventSystem::Startup()
{
    DAEMON_LOG(LogEvent, eLogVerbosity::Log, "EventSystem::Startup()");
}

//----------------------------------------------------------------------------------------------------
void EventSystem::Shutdown()
{
    std::lock_guard<std::mutex> lock(m_subscriptionsMutex);
    m_subscriptionsByEventName.clear();
}

//----------------------------------------------------------------------------------------------------
void EventSystem::BeginFrame()
{
}

//----------------------------------------------------------------------------------------------------
void EventSystem::EndFrame()
{
}

//----------------------------------------------------------------------------------------------------
void EventSystem::SubscribeEventCallbackFunction(String const& eventName, EventCallbackFunction const functionPtr)
{
    std::lock_guard<std::mutex> lock(m_subscriptionsMutex);

    sEventSubscription newSubscription;
    newSubscription.callbackFunction = functionPtr;

    HashedCaseInsensitiveString hcisName(eventName);
    SubscriptionList& subscriptions = m_subscriptionsByEventName[hcisName];

    subscriptions.push_back(newSubscription);
}

//----------------------------------------------------------------------------------------------------
void EventSystem::UnsubscribeEventCallbackFunction(String const& eventName, EventCallbackFunction const functionPtr)
{
    std::lock_guard<std::mutex> lock(m_subscriptionsMutex);

    HashedCaseInsensitiveString hcisName(eventName);
    auto const iter = m_subscriptionsByEventName.find(hcisName);

    if (iter != m_subscriptionsByEventName.end())
    {
        SubscriptionList& subscriptions = iter->second;

        subscriptions.erase(
            std::remove_if(subscriptions.begin(), subscriptions.end(),
                           [functionPtr](sEventSubscription const& subscription)
                           {
                               return subscription.callbackFunction == functionPtr;
                           }),
            subscriptions.end()
        );

        // If the list is empty, remove the entry from the map
        if (subscriptions.empty())
        {
            m_subscriptionsByEventName.erase(iter);
        }
    }
}

//----------------------------------------------------------------------------------------------------
void EventSystem::FireEvent(String const& eventName, EventArgs& args)
{
    // Make a local copy of the subscription list to avoid holding mutex during callback execution
    SubscriptionList subscriptionsCopy;
    {
        std::lock_guard<std::mutex> lock(m_subscriptionsMutex);
        HashedCaseInsensitiveString hcisName(eventName);
        auto const iter = m_subscriptionsByEventName.find(hcisName);
        if (iter != m_subscriptionsByEventName.end())
        {
            subscriptionsCopy = iter->second;  // Copy the subscription list
        }
    }

    // Execute callbacks without holding the mutex (callbacks might subscribe/unsubscribe)
    for (sEventSubscription const& subscription : subscriptionsCopy)
    {
        bool consumed = false;

        if (subscription.callbackFunction)
        {
            consumed = subscription.callbackFunction(args);
        }
        else if (subscription.memberCallback)
        {
            consumed = subscription.memberCallback(args);
        }

        if (consumed)
        {
            break; // Event consumed; stop notifying further subscribers
        }
    }
}

//----------------------------------------------------------------------------------------------------
void EventSystem::FireEvent(String const& eventName)
{
    EventArgs emptyArgs;
    FireEvent(eventName, emptyArgs);
}

//----------------------------------------------------------------------------------------------------
StringList EventSystem::GetAllRegisteredEventNames() const
{
    std::lock_guard<std::mutex> lock(m_subscriptionsMutex);

    std::vector<String> eventNames;
    eventNames.reserve(m_subscriptionsByEventName.size());

    for (auto const& pair : m_subscriptionsByEventName)
    {
        eventNames.push_back(pair.first.GetOriginalString());
    }

    return eventNames;
}

//----------------------------------------------------------------------------------------------------
void SubscribeEventCallbackFunction(String const& eventName, EventCallbackFunction const functionPtr)
{
    if (!g_eventSystem)
    {
        DAEMON_LOG(LogEvent, eLogVerbosity::Error, "EventSystem::SubscribeEventCallbackFunction()");
        return;
    }
    g_eventSystem->SubscribeEventCallbackFunction(eventName, functionPtr);
}

//----------------------------------------------------------------------------------------------------
void UnsubscribeEventCallbackFunction(String const& eventName, EventCallbackFunction const functionPtr)
{
    if (g_eventSystem)
    {
        g_eventSystem->UnsubscribeEventCallbackFunction(eventName, functionPtr);
    }
}

//----------------------------------------------------------------------------------------------------
void FireEvent(String const& eventName, EventArgs& args)
{
    if (g_eventSystem != nullptr)
    {
        g_eventSystem->FireEvent(eventName, args);
    }
}

//----------------------------------------------------------------------------------------------------
void FireEvent(String const& eventName)
{
    if (g_eventSystem != nullptr)
    {
        g_eventSystem->FireEvent(eventName);
    }
}
