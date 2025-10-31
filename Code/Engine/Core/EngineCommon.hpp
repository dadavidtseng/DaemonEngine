//----------------------------------------------------------------------------------------------------
// EngineCommon.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
//----------------------------------------------------------------------------------------------------
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Core/NamedStrings.hpp"
#include "Engine/Script/ScriptSubsystem.hpp"

struct sAudioSystemConfig;
//-Forward-Declaration--------------------------------------------------------------------------------
class Window;
class ScriptSubsystem;
class ResourceSubsystem;
class Renderer;
class RandomNumberGenerator;
class AudioSystem;
class DevConsole;
class EventSystem;
class HotReloadSubsystem;
class InputSystem;
class JobSystem;
class LogSubsystem;
class ImGuiSubsystem;

//----------------------------------------------------------------------------------------------------
/// @brief declared in EngineCommon.hpp, defined in EngineCommon.cpp
extern NamedStrings g_gameConfigBlackboard;

//----------------------------------------------------------------------------------------------------
extern AudioSystem*           g_audio;
extern DevConsole*            g_devConsole;
extern EventSystem*           g_eventSystem;
extern JobSystem*             g_jobSystem;
extern LogSubsystem*          g_logSubsystem;
extern InputSystem*           g_input;
extern Renderer*              g_renderer;
extern RandomNumberGenerator* g_rng;
extern Window*                g_window;
extern ResourceSubsystem*     g_resourceSubsystem;
extern ScriptSubsystem*       g_scriptSubsystem;
extern ImGuiSubsystem*        g_imgui;

// Forward declaration for KADI subsystem
class KADIWebSocketSubsystem;
extern KADIWebSocketSubsystem* g_kadiSubsystem;

// //----------------------------------------------------------------------------------------------------
// struct sEngineConfig
// {
//     sAudioSystemConfig     m_audioConfig;
//     sScriptSubsystemConfig m_scriptConfig;
// };

// //----------------------------------------------------------------------------------------------------
// namespace Engine
// {
//     namespace Audio
//     {
//         extern AudioSystem* g_audio;
//     }
//
//     //------------------------------------------------------------------------------------------------
//     namespace Core
//     {
//         extern DevConsole*   g_devConsole;
//         extern EventSystem*  g_eventSystem;
//         extern JobSystem*    g_jobSystem;
//         extern LogSubsystem* g_logSubsystem;
//     }
//
//     //------------------------------------------------------------------------------------------------
//     namespace Input
//     {
//         extern InputSystem* g_input;
//     }
//
//     //------------------------------------------------------------------------------------------------
//     namespace Graphic
//     {
//         extern Renderer* g_renderer;
//     }
//
//     //------------------------------------------------------------------------------------------------
//     namespace Math
//     {
//         extern RandomNumberGenerator* g_rng;
//     }
//
//     //------------------------------------------------------------------------------------------------
//     namespace Platform
//     {
//         extern Window* g_window;
//     }
//
//     //------------------------------------------------------------------------------------------------
//     namespace Resource
//     {
//         extern ResourceSubsystem* g_resourceSubsystem;
//     }
//
//     //------------------------------------------------------------------------------------------------
//     namespace Script
//     {
//         extern ScriptSubsystem* g_scriptSubsystem;
//     }
// }


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
