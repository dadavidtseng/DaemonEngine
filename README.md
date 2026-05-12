# Daemon Engine (Engine Module)

This README documents the Engine module (C++20) for Daemon Engine — a modular, performance-oriented game engine with V8 JavaScript integration, DirectX 11 rendering, FMOD audio, and a lock-free async architecture for dual-language game logic.

Changelog (highlights)
- 2025-12-07: Audio async pattern implementation — added Code/Engine/Audio/AudioState.hpp:
  - AudioState is a POD struct (~72 bytes) representing per-source playback state for use with the double-buffered AudioStateBuffer (soundId, soundPath, position, volume, isPlaying, isLooped, isLoaded, isActive). Used to safely communicate audio playback state between worker and main threads.
- 2025-11-09: Documented JobSystem shutdown pattern. Critical: stop JobSystem BEFORE deleting objects accessible to worker threads. See SimpleMiner's App.cpp for an example three-stage shutdown:
  1. g_jobSystem->Shutdown()
  2. Delete game objects (chunks, world, entities)
  3. GEngine::Get().Shutdown()
- 2025-10-27: M4-T8 Async Architecture refactor:
  - Added Entity module and async entity management (EntityAPI, EntityScriptInterface)
  - Introduced generic StateBuffer template for lock-free double-buffering (Core module)
  - Camera system refactor: CameraAPI + CameraStateBuffer
  - Introduced IJSGameLogicContext to invert dependency between Engine and Game
  - Thread-safe state synchronization between worker and main threads

Project Vision
- Modular, subsystem-based engine designed for education and production use.
- Emphasizes performance, data-oriented design, and an event-driven subsystem model.
- Provides DirectX11 renderer, FMOD audio, input systems, networking, resource management, and V8 scripting with async worker threads.

Technical Stack (engine module)
- Language: C++20
- Graphics: DirectX 11
- Audio: FMOD (3D audio)
- Scripting: V8 (async worker architecture)
- Platform: Windows x64
- Build: Visual Studio / MSBuild (Engine.sln)

Architecture Overview
- The engine is organized into independent modules that communicate via an EventSystem and well-defined interfaces.
- Async JavaScript integration uses lock-free double-buffering (StateBuffer) to synchronize state between the main thread (rendering) and worker threads (V8 JS logic).

Async Architecture Pattern (M4-T8)
Main Thread (Rendering) and Worker Thread (JavaScript) interact using front/back StateBuffers and a render command queue.

Main Thread (Rendering)           Worker Thread (JavaScript)
- BeginFrame()                     - V8 Isolate Lock
- Process RenderCommands           - JSEngine.update()
- SwapBuffers()                    - Write to Back Buffers (entities, camera, audio...)
  - EntityStateBuffer
  - CameraStateBuffer
  - AudioStateBuffer                 - Submit RenderCommands
- Render from Front Buffers        - V8 Isolate Unlock
- EndFrame()

Module Index (paths & key headers / entry points)
| Module | Path | Key headers / entry points |
|---|---:|---|
| Core | Code/Engine/Core/ | EngineCommon.hpp, EventSystem.hpp, StateBuffer.hpp, BufferParser.hpp, Engine.hpp (GEngine) |
| Entity | Code/Engine/Entity/ | EntityAPI.hpp, EntityScriptInterface.hpp, EntityID.hpp |
| Renderer | Code/Engine/Renderer/ | Renderer.hpp, Camera.hpp, CameraAPI.hpp |
| Audio | Code/Engine/Audio/ | AudioSystem.hpp, AudioState.hpp |
| Input | Code/Engine/Input/ | InputSystem.hpp, AnalogJoystick.hpp |
| Math | Code/Engine/Math/ | MathUtils.hpp, Vec2.hpp, Vec3.hpp, Mat44.hpp, AABB2.hpp |
| Script | Code/Engine/Script/ | V8Subsystem.hpp, IJSGameLogicContext.hpp |
| Resource | Code/Engine/Resource/ | ResourceSubsystem.hpp |
| Network | Code/Engine/Network/ | NetworkSubsystem.hpp, BaseWebSocketSubsystem.hpp |
| Platform | Code/Engine/Platform/ | Window.hpp |
| UI | Code/Engine/UI/ | (UI subsystem headers) |
| Widget | Code/Engine/Widget/ | (Widget subsystem headers) |

Key Engine Concepts and APIs

GEngine (global engine singleton)
- File: Code/Engine/Core/Engine.hpp (GEngine.hpp)
- Purpose: central singleton providing access to core subsystems and encapsulating lifecycle operations.
- Important methods:
  - static GEngine& Get();
  - void Construct();
  - void Destruct();
  - void Startup();
  - void Shutdown();
- Notes: Subsystems should be initialized before use. Some subsystems may be optional (e.g., AudioSystem, InputSystem).

IJSGameLogicContext (scripting integration)
- File: Code/Engine/Script/IJSGameLogicContext.hpp
- Purpose: abstract interface for game-specific JavaScript execution context. Engine's JSGameLogicJob depends on this interface so the Engine library can compile without concrete Game code.
- Responsibilities:
  - Provide worker-thread callbacks like UpdateJSWorkerThread(...)
  - Implementations must be thread-safe (typically using v8::Locker)
- Usage example:
  - class Game : public IJSGameLogicContext { void UpdateJSWorkerThread(float dt, ...) override; };

StateBuffer (lock-free double-buffering)
- Location: Code/Engine/Core/StateBuffer.hpp (referenced in CLAUDE.md)
- Purpose: generic template used for passing state (entities, camera, audio) between worker and main threads without locks.
- Notes: Used to implement front/back buffers such as EntityStateBuffer, CameraStateBuffer, and AudioStateBuffer. Module-specific POD structs (e.g., AudioState) are designed to be copyable and efficient for this pattern.

EntityID (entity identifier type)
- File: Code/Engine/Entity/EntityID.hpp
- Definition: using EntityID = uint64_t;
- Purpose: unique identifier type used across the engine (Entity, Renderer, Physics, networking, etc.).
- Notes:
  - 64-bit unsigned provides a very large space for incremental ID generation.
  - JavaScript compatibility is considered: JS safe integer range is up to 2^53-1 (9,007,199,254,740,991). When passing EntityID to JS, take care if IDs may exceed that range (common designs avoid exceeding 2^53).
  - Typical usage: EntityID id = 12345;

BufferParser (binary parsing helper)
- File: Code/Engine/Core/BufferParser.hpp
- Purpose: utility to parse binary data buffers for resource loading and network messages.
- Primary API (selected):
  - Primitives: ParseByte(), ParseChar(), ParseUshort(), ParseShort(), ParseUint32(), ParseInt32(), ParseUint64(), ParseInt64(), ParseFloat(), ParseDouble()
  - Strings: ParseZeroTerminatedString(std::string&), ParseLengthPrecededString(std::string&)
  - Engine semi-primitives: ParseVec2(), ParseVec3(), ParseIntVec2(), ParseRgba8(), ParseAABB2()
- Usage: construct with pointer+size or std::vector<uint8_t> and call parsers in order to decode binary formats.

JobSystem shutdown pattern (CRITICAL)
- Always stop worker threads before deleting objects that worker threads may access.
- Recommended shutdown sequence (example):
  1. g_jobSystem->Shutdown()  // stop all workers
  2. Delete game objects (chunks, world, entities, resources)
  3. GEngine::Get().Shutdown() // destroy engine systems
- Rationale: avoids race conditions and memory access-after-free when workers are still processing.

Common File Layout (select)
- Code/Engine/Core/
  - EngineCommon.hpp
  - EventSystem.hpp
  - StateBuffer.hpp
  - BufferParser.hpp
  - Engine.hpp (GEngine)
- Code/Engine/Entity/
  - EntityAPI.hpp
  - EntityScriptInterface.hpp
  - EntityID.hpp
- Code/Engine/Renderer/
  - Renderer.hpp
  - Camera.hpp
  - CameraAPI.hpp
- Code/Engine/Script/
  - V8Subsystem.hpp
  - IJSGameLogicContext.hpp
- Code/Engine/Audio/
  - AudioSystem.hpp
  - AudioState.hpp
- Code/Engine/Input/
  - InputSystem.hpp
  - AnalogJoystick.hpp
- Code/Engine/Math/
  - MathUtils.hpp
  - Vec2.hpp
  - Vec3.hpp
  - Mat44.hpp
  - AABB2.hpp
- Code/Engine/Network/
  - NetworkSubsystem.hpp
  - BaseWebSocketSubsystem.hpp
- Code/Engine/Platform/
  - Window.hpp

Build & Run

Prerequisites
- Windows 10 (x64)
- Visual Studio 2019 or later with C++20 support
- Windows 10 SDK (10.0.18362.0 or later)
- DirectX 11 capable GPU
- Git

Build instructions
1. Clone repository:
   git clone https://github.com/dadavidtseng/DaemonEngine.git
   cd DaemonEngine
2. Open Engine.sln in Visual Studio:
   start Engine.sln
3. Set solution configuration:
   - Platform: x64
   - Debug / Release as needed
4. Build solution (Ctrl+Shift+B)

Basic Engine Initialization (adapted from architecture docs)
```cpp
#include "Code/Engine/Core/EngineCommon.hpp"
#include "Code/Engine/Renderer/Renderer.hpp"
#include "Code/Engine/Audio/AudioSystem.hpp"
#include "Code/Engine/Core/EventSystem.hpp"

// Create and wire subsystems (example)
g_theRenderer = new Renderer();
g_theAudio = new AudioSystem();
g_eventSystem = new EventSystem();

// Initialize engine singleton
GEngine::Get().Construct();
GEngine::Get().Startup();

// Game loop skeleton
while (isRunning) {
    g_theRenderer->BeginFrame();
    // game logic, input processing, submit render commands
    g_theRenderer->EndFrame();
}

// Shutdown sequence must follow JobSystem shutdown pattern
```

Testing Strategy
- Current practices:
  - Manual testing via developer console
  - Visual debugging (DebugRenderSystem)
  - Memory tracking and leak detection
  - Thread-safety analysis for async architecture
- Recommended additions:
  - Integrate Google Test for unit tests
  - Automated regression tests and performance benchmarks
  - Async stress tests for StateBuffer/JobSystem interactions

Notes, References, and Examples
- See Code/Engine/Core/CLAUDE.md and Code/Engine/Renderer/CLAUDE.md for per-module architecture notes.
- Example project SimpleMiner demonstrates the recommended JobSystem shutdown pattern in App.cpp.
- IJSGameLogicContext and JSGameLogicJob are central to the lock-free async JavaScript integration — see Code/Engine/Script/ for implementation details.
- Module-specific POD structs (e.g., Code/Engine/Audio/AudioState.hpp) are used with StateBuffer double-buffering for efficient audio state synchronization.

Contributing
- Follow C++20 practices used throughout the codebase.
- Use PascalCase for classes and enums (e.g., AudioSystem).
- Keep subsystem boundaries clear and prefer dependency inversion for testability (e.g., IJSGameLogicContext).

If you want a deeper, per-file reference (public APIs, methods, and examples) or a generated header/API list, provide the repository or specific files and I will update this README with API signatures and code examples extracted directly from the source.