//----------------------------------------------------------------------------------------------------
// EventSystem.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Core/EventSystem.hpp"

#include "Engine/Core/NamedStrings.hpp"

//----------------------------------------------------------------------------------------------------
EventSystem* g_theEventSystem = nullptr;

//----------------------------------------------------------------------------------------------------
EventSystem::EventSystem(sEventSystemConfig const& config)
    : m_config(config)
{
}

//----------------------------------------------------------------------------------------------------
void EventSystem::Startup()
{
}

//----------------------------------------------------------------------------------------------------
void EventSystem::Shutdown()
{
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
    EventSubscription const newSubscription = {functionPtr};

    SubscriptionList& subscriptions = m_subscriptionsByEventName[eventName];

    subscriptions.push_back(newSubscription);
}

//----------------------------------------------------------------------------------------------------
void EventSystem::UnsubscribeEventCallbackFunction(String const& eventName, EventCallbackFunction const functionPtr)
{
    auto const iter = m_subscriptionsByEventName.find(eventName);

    if (iter != m_subscriptionsByEventName.end())
    {
        SubscriptionList& subscriptions = iter->second;

        subscriptions.erase(
            std::remove_if(subscriptions.begin(), subscriptions.end(),
                           [functionPtr](EventSubscription const& subscription)
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
    std::map<std::string, std::vector<EventSubscription>>::iterator const iter = m_subscriptionsByEventName.find(eventName);

    if (iter != m_subscriptionsByEventName.end())
    {
        SubscriptionList const& subscriptions = iter->second;

        for (EventSubscription const& subscription : subscriptions)
        {
             if (subscription.callbackFunction(args))
             {
                 break; // Event consumed; stop notifying further subscribers
             }
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
    std::vector<String> eventNames;
    eventNames.reserve(m_subscriptionsByEventName.size());

    for (std::pair<String const, SubscriptionList> const& pair : m_subscriptionsByEventName)
    {
        eventNames.push_back(pair.first);   // first for key, second for value in std::map
    }

    return eventNames;
}

//----------------------------------------------------------------------------------------------------
void SubscribeEventCallbackFunction(String const& eventName, EventCallbackFunction const functionPtr)
{
    if (g_theEventSystem)
    {
        g_theEventSystem->SubscribeEventCallbackFunction(eventName, functionPtr);
    }
}

//----------------------------------------------------------------------------------------------------
void UnsubscribeEventCallbackFunction(String const& eventName, EventCallbackFunction const functionPtr)
{
    if (g_theEventSystem)
    {
        g_theEventSystem->UnsubscribeEventCallbackFunction(eventName, functionPtr);
    }
}

//----------------------------------------------------------------------------------------------------
void FireEvent(String const& eventName, EventArgs& args)
{
    if (g_theEventSystem != nullptr)
    {
        g_theEventSystem->FireEvent(eventName, args);
    }
}

//----------------------------------------------------------------------------------------------------
void FireEvent(String const& eventName)
{
    if (g_theEventSystem != nullptr)
    {
        g_theEventSystem->FireEvent(eventName);
    }
}
