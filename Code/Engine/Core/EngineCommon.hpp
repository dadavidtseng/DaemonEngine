//----------------------------------------------------------------------------------------------------
// EngineCommon.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once

#include "Engine/Core/NamedStrings.hpp"

//-Forward-Declaration--------------------------------------------------------------------------------
class DevConsole;
class EventSystem;
class InputSystem;
class V8Subsystem;

//----------------------------------------------------------------------------------------------------
/// @brief declared in EngineCommon.hpp, defined in EngineCommon.cpp
extern NamedStrings g_gameConfigBlackboard;
extern EventSystem* g_theEventSystem;
extern DevConsole*  g_theDevConsole;
extern InputSystem* g_theInput;
extern V8Subsystem* g_theV8Subsystem;

//----------------------------------------------------------------------------------------------------
/// @brief
/// Marks a variable as intentionally unused to prevent compiler warnings.
/// @param x
/// The variable to mark as unused.
#define UNUSED(x) (void)(x);

//----------------------------------------------------------------------------------------------------
/// @brief
/// Macro to mark variables or functions in header files as static.
/// Currently expands to nothing; serves only as a semantic marker.
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
