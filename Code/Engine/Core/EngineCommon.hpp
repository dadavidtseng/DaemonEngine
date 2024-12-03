//----------------------------------------------------------------------------------------------------
// EngineCommon.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once

#include "Engine/Core/NamedStrings.hpp"


class EventSystem;

//----------------------------------------------------------------------------------------------------
extern NamedStrings g_gameConfigBlackboard; // declared in EngineCommon.hpp, defined in EngineCommon.cpp
extern EventSystem* g_theEventSystem;
//----------------------------------------------------------------------------------------------------
#define UNUSED(x) (void)(x);
// TODO: decide to add this or not
// #define STATIC;