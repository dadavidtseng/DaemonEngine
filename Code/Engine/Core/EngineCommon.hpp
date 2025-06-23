//----------------------------------------------------------------------------------------------------
// EngineCommon.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once

#include "Engine/Core/NamedStrings.hpp"

//----------------------------------------------------------------------------------------------------
class DevConsole;
class EventSystem;
class InputSystem;

//----------------------------------------------------------------------------------------------------
extern NamedStrings g_gameConfigBlackboard; // declared in EngineCommon.hpp, defined in EngineCommon.cpp
extern EventSystem* g_theEventSystem;
extern DevConsole*  g_theDevConsole;
extern InputSystem* g_theInput;


//----------------------------------------------------------------------------------------------------
#define UNUSED(x) (void)(x);
#define STATIC

//----------------------------------------------------------------------------------------------------
template <typename T>
void ENGINE_SAFE_RELEASE(T*& pointer)
{
    if (pointer != nullptr)
    {
        delete pointer;
        pointer = nullptr;
    }
}
