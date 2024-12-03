//----------------------------------------------------------------------------------------------------
// EventSystem.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Core/EventSystem.hpp"

#include "Engine/Core/EngineCommon.hpp"

//----------------------------------------------------------------------------------------------------
// Global Pointer
EventSystem* g_theEventSystem = nullptr;

//----------------------------------------------------------------------------------------------------
EventSystem::EventSystem(EventSystemConfig const& config)
    : m_config(config)
{
}

//----------------------------------------------------------------------------------------------------
EventSystem::~EventSystem()
{
    Shutdown();
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
    EventSubscription const subscription = { functionPtr };

    m_subscriptionsByEventName[eventName].push_back(subscription);
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
    auto const iter = m_subscriptionsByEventName.find(eventName);
    if (iter != m_subscriptionsByEventName.end())
    {
        SubscriptionList const& subscriptions = iter->second;

        for (auto const& subscription : subscriptions)
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
    if (g_theEventSystem)
    {
        g_theEventSystem->FireEvent(eventName, args);
    }
}

//----------------------------------------------------------------------------------------------------
void FireEvent(String const& eventName)
{
    if (g_theEventSystem)
    {
        g_theEventSystem->FireEvent(eventName);
    }
}
