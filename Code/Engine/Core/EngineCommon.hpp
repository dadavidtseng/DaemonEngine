//----------------------------------------------------------------------------------------------------
// EngineCommon.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once

#include "Engine/Core/NamedStrings.hpp"


class DevConsole;
class EventSystem;

//----------------------------------------------------------------------------------------------------
extern NamedStrings g_gameConfigBlackboard; // declared in EngineCommon.hpp, defined in EngineCommon.cpp
extern EventSystem* g_theEventSystem;
extern DevConsole*  g_theDevConsole;
//----------------------------------------------------------------------------------------------------
#define UNUSED(x) (void)(x);
#define STATIC
