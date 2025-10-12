//----------------------------------------------------------------------------------------------------
// EngineCommon.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Core/EngineCommon.hpp"

//----------------------------------------------------------------------------------------------------
NamedStrings g_gameConfigBlackboard;

//----------------------------------------------------------------------------------------------------
AudioSystem*           g_audio             = nullptr;
JobSystem*             g_jobSystem         = nullptr;
Renderer*              g_renderer          = nullptr;
RandomNumberGenerator* g_rng               = nullptr;
Window*                g_window            = nullptr;
ResourceSubsystem*     g_resourceSubsystem = nullptr;
ScriptSubsystem*       g_scriptSubsystem   = nullptr;
KADIWebSocketSubsystem* g_kadiSubsystem    = nullptr;


// //----------------------------------------------------------------------------------------------------
// namespace Engine
// {
//     namespace Audio
//     {
//         AudioSystem* g_audio = nullptr;
//     }
//
//     //------------------------------------------------------------------------------------------------
//     namespace Core
//     {
//         // extern DevConsole*   g_devConsole;
//         // extern EventSystem*  g_eventSystem;
//         JobSystem* g_jobSystem = nullptr;
//         // extern LogSubsystem* g_logSubsystem;
//     }
//
//     //------------------------------------------------------------------------------------------------
//     namespace Input
//     {
//         // extern InputSystem* g_input;
//     }
//
//     //------------------------------------------------------------------------------------------------
//     namespace Graphic
//     {
//         Renderer* g_renderer = nullptr;
//     }
//
//     //------------------------------------------------------------------------------------------------
//     namespace Math
//     {
//         RandomNumberGenerator* g_rng = nullptr;
//     }
//
//     //------------------------------------------------------------------------------------------------
//     namespace Platform
//     {
//         Window* g_window = nullptr;
//     }
//
//     //------------------------------------------------------------------------------------------------
//     namespace Resource
//     {
//         ResourceSubsystem* g_resourceSubsystem = nullptr;
//     }
//
//     //------------------------------------------------------------------------------------------------
//     namespace Script
//     {
//         ScriptSubsystem* g_scriptSubsystem = nullptr;
//     }
// }
