//----------------------------------------------------------------------------------------------------
// EventRecipient.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Core/EventRecipient.hpp"

#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/EventSystem.hpp"

//----------------------------------------------------------------------------------------------------
EventRecipient::~EventRecipient()
{
    if (g_eventSystem)
    {
        g_eventSystem->UnsubscribeAllEventCallbacksForObject(this);
    }
}
