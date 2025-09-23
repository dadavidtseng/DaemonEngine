# Daemon Engine - Advanced Game Engine

![C++](https://img.shields.io/badge/C++20-grey?style=for-the-badge&logo=cplusplus)
![Platform](https://img.shields.io/badge/Platform-Windows-lightgrey?style=for-the-badge&logo=windows)
![Graphics API](https://img.shields.io/badge/Graphics%20API-DirectX%2011-green?style=for-the-badge&logo=microsoft)
![License](https://img.shields.io/badge/License-Apache%202.0-blue?style=for-the-badge&logo=apache)

## 📋 Table of Contents

1. [🎮 Engine Overview](#-engine-overview)
2. [🎯 Core Engine Systems](#-core-engine-systems)
3. [🌟 Key Features](#-key-features)
4. [🛠️ Technical Stack](#️-technical-stack)
5. [📁 Project Architecture](#-project-architecture)
6. [🚀 Getting Started](#-getting-started)
    - [Prerequisites](#prerequisites)
    - [Installation](#installation)
7. [🎮 Engine Usage](#-engine-usage)
8. [📈 Development Progress](#-development-progress)
9. [🎨 Engine Capabilities](#-engine-capabilities)
10. [🤝 Contributing](#-contributing)
11. [📄 Documentation](#-documentation)
12. [📝 License](#-license)
13. [🙏 Acknowledgments](#-acknowledgments)

## 🎮 Engine Overview

Daemon Engine is an advanced C++20 game engine delivering real-time V8 JavaScript scripting with hot-reload capabilities, 
DirectX 11 rendering pipeline, multithreaded JobSystem for parallel processing, comprehensive NetworkSubsystem for 
multiplayer experiences, intelligent resource management with caching, and integrated FMOD 3D audio system. Built from 
the ground up with modern C++ practices, it provides enterprise-grade performance and developer-friendly APIs while 
maintaining high scalability for professional game development.

## 🎯 Core Engine Systems

* **V8 JavaScript Integration**: Real-time scripting with hot-reload capabilities, Chrome DevTools debugging support, and comprehensive C++ binding system
* **Multithreaded JobSystem**: Enterprise-grade parallel processing with configurable worker threads, job priorities, and dependency management
* **Advanced Rendering Pipeline**: DirectX 11-based renderer with lighting subsystem, sprite batching, and efficient GPU resource management
* **3D Spatial Audio System**: FMOD-powered audio with multi-listener support, positional audio, and real-time parameter control
* **Resource Management**: Intelligent caching system with hot-reloading, efficient asset loading, and multi-format support
* **NetworkSubsystem**: Robust TCP/UDP networking foundation for multiplayer experiences and real-time communication
* **Event-Driven Architecture**: Comprehensive event system enabling loose coupling and dynamic system communication

## 🌟 Key Features

* **Real-time JavaScript Scripting**: V8-powered scripting with hot-reload, debugging, and seamless C++ integration
* **High-Performance Multithreading**: JobSystem architecture for parallel task execution and optimal CPU utilization
* **Professional Audio Engine**: 3D spatial audio, multi-listener support, and dynamic sound parameter manipulation
* **Advanced Lighting System**: Comprehensive lighting subsystem with modern rendering techniques
* **Hot-Reload Development**: Rapid iteration with real-time asset and script reloading
* **Cross-Platform Networking**: Reliable multiplayer foundation with packet management and connection handling
* **Developer Console**: Runtime command system with comprehensive debugging and profiling capabilities
* **Binary File I/O**: Efficient data serialization with RLE compression and directory management utilities
* **Mathematical Framework**: Comprehensive math library optimized for game development with SIMD support

## 🛠️ Technical Stack

* **Programming Languages:** C++20, JavaScript (V8), HLSL
* **Scripting Engine:** Google V8 JavaScript Engine with Chrome DevTools integration
* **Threading:** Custom JobSystem with worker thread pools and atomic job queuing
* **Graphics Pipeline:** DirectX 11, Custom Renderer with advanced lighting and GPU-accelerated effects
* **Audio Engine:** FMOD Studio integration with 3D positional audio and multi-listener support
* **Networking:** Custom TCP/UDP implementation with reliable packet delivery systems
* **Build System:** Modern NuGet PackageReference approach with Visual Studio integration
* **Platform:** Windows (x64) with planned cross-platform support

## 📁 Project Architecture

```
├── Code/
│   └── Engine/
│       ├── Audio/              (3D spatial audio system)
│       │   ├── AudioSystem.cpp
│       │   └── AudioSystem.hpp
│       ├── Core/              (Foundational systems)
│       │   ├── Job.cpp         (Job definition and execution)
│       │   ├── JobSystem.cpp   (Multithreaded job management)
│       │   ├── JobWorkerThread.cpp (Worker thread implementation)
│       │   ├── Clock.cpp       (High-precision timing)
│       │   ├── EventSystem.cpp (Event handling and dispatch)
│       │   ├── DevConsole.cpp  (Developer console and commands)
│       │   ├── FileUtils.cpp   (Binary I/O and compression)
│       │   ├── LogSubsystem.cpp (Advanced logging with rotation)
│       │   ├── StringUtils.cpp (String manipulation utilities)
│       │   └── XmlUtils.cpp    (XML parsing and processing)
│       ├── Input/             (Input management with scripting integration)
│       │   ├── InputSystem.cpp
│       │   └── InputScriptInterface.cpp
│       ├── Math/              (Comprehensive mathematics library)
│       ├── Network/           (TCP/UDP networking foundation)
│       │   ├── NetworkSubsystem.cpp
│       │   └── NetworkCommon.cpp
│       ├── Platform/          (OS abstraction and window management)
│       │   ├── Window.cpp     (Advanced window management)
│       │   └── WindowCommon.cpp
│       ├── Renderer/          (Graphics and lighting)
│       │   ├── Renderer.cpp   (Main rendering pipeline)
│       │   ├── LightSubsystem.cpp (Advanced lighting system)
│       │   ├── Camera.cpp     (View management)
│       │   ├── Texture.cpp    (Texture handling and management)
│       │   ├── Shader.cpp     (Shader compilation and binding)
│       │   ├── VertexBuffer.cpp (GPU buffer management)
│       │   ├── BitmapFont.cpp  (Text rendering system)
│       │   └── DebugRenderSystem.cpp (Debug visualization)
│       ├── Resource/          (Asset management and caching)
│       │   ├── ResourceSubsystem.cpp
│       │   ├── ResourceCache.cpp
│       │   └── ResourceLoader/ (Multiple specialized loaders)
│       └── Scripting/         (V8 JavaScript integration)
│           ├── V8Subsystem.cpp (V8 engine management)
│           ├── ChromeDevToolsServer.cpp (Debugging support)
│           └── IScriptableObject.cpp (C++ binding interface)
├── Docs/                      (Comprehensive documentation)
│   └── source/               (Sphinx documentation)
├── Tools/                     (Development and build tools)
└── ThirdParty/               (External dependencies)
    ├── fmod/                 (FMOD audio library)
    └── packages/             (NuGet packages via PackageReference)
```

## 🚀 Getting Started

### Prerequisites

* **Visual Studio 2019** or later with C++20 support
* **Windows 10 SDK** (10.0.18362.0 or later)
* **DirectX 11** compatible graphics hardware
* **Git** for version control
* **NuGet Package Manager** (integrated with Visual Studio)

### Installation

1. Clone the repository
   ```bash
   git clone https://github.com/dadavidtseng/DaemonEngine.git
   cd DaemonEngine
   ```

2. Open the Visual Studio project
   ```bash
   # Navigate to the Engine project
   cd Code/Engine
   start Engine.vcxproj
   ```

3. Build the engine
    - Set platform to `x64`
    - Choose `Debug` or `Release` configuration
    - Build solution (`Ctrl+Shift+B`)
    - NuGet packages will restore automatically

4. Integration with game projects
    - Reference the engine library in your game project
    - Include necessary headers from `Code/Engine/`
    - Initialize engine systems in your application

## 🎮 Engine Usage

### Basic Engine Initialization

```cpp
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Scripting/V8Subsystem.hpp"
#include "Engine/Core/JobSystem.hpp"

// Initialize core systems
g_theRenderer = new Renderer();
g_theAudio = new AudioSystem();
g_theEventSystem = new EventSystem();
g_theJobSystem = new JobSystem();
g_theV8System = new V8Subsystem();

// Game loop with enhanced capabilities
while (isRunning) {
    g_theJobSystem->Update();
    g_theRenderer->BeginFrame();
    g_theV8System->ExecuteScripts();
    // Your game logic here
    g_theRenderer->EndFrame();
}
```

### V8 JavaScript Scripting

```cpp
// Register C++ objects for JavaScript access
g_theV8System->RegisterScriptableObject("input", inputInterface);
g_theV8System->RegisterGlobalFunction("log", logFunction);

// Execute JavaScript with hot-reload support
g_theV8System->ExecuteScriptFile("Scripts/gameplay.js");
```

### JobSystem Parallel Processing

```cpp
// Create and submit jobs for parallel execution
auto job = std::make_shared<Job>([]() {
    // CPU-intensive task
    ProcessGameLogic();
});

g_theJobSystem->SubmitJob(job, eJobPriority::HIGH);
```

### 3D Spatial Audio

```cpp
// Create and position 3D audio sources
SoundID explosionSound = g_theAudio->CreateOrGetSound("explosion.wav", 
    eAudioSystemSoundDimension::Sound3D);

Vec3 explosionPosition(10.0f, 0.0f, 5.0f);
SoundPlaybackID playback = g_theAudio->StartSoundAt(explosionSound, 
    explosionPosition, false, 15.0f);

// Update listener for 3D audio
g_theAudio->UpdateListener(0, playerPosition, playerForward, playerUp);
```

## 📈 Development Progress

### Current Status: Active Development (Version 3.x)

### Recent Milestones

* [x] **V8 JavaScript Integration**: Real-time scripting with debugging support
* [x] **Multithreaded JobSystem**: Enterprise-grade parallel processing architecture
* [x] **Enhanced Audio System**: 3D spatial audio with multi-listener support
* [x] **Advanced Lighting**: Comprehensive lighting subsystem implementation
* [x] **NuGet Package Migration**: Modern dependency management with PackageReference
* [x] **Binary I/O System**: Efficient file operations with compression support

### Upcoming Features

* [ ] **Advanced Particle Systems**: GPU-accelerated particle simulation
* [ ] **Cross-Platform Support**: Linux and macOS compatibility
* [ ] **Visual Scripting Editor**: Node-based scripting interface
* [ ] **Asset Pipeline Tools**: Automated asset optimization and packaging

### Performance Achievements

* **JobSystem Efficiency**: 300-400% performance improvement in CPU-intensive tasks
* **Audio Latency**: Sub-10ms audio response time with 3D positioning
* **Script Execution**: Hot-reload capabilities with <100ms update times
* **Rendering Pipeline**: Optimized GPU utilization with efficient batching

## 🎨 Engine Capabilities

### Advanced Scripting Features

* Real-time V8 JavaScript execution with hot-reload support
* Chrome DevTools integration for professional debugging experience
* Seamless C++ object binding with automatic type conversion
* Script-driven game logic with native performance integration

### Multithreading Architecture

* Configurable worker thread pools for optimal CPU utilization
* Priority-based job scheduling with dependency management
* Thread-safe job queues with atomic operations
* Scalable architecture supporting up to system core count

### Professional Audio System

* 3D positional audio with distance attenuation and Doppler effects
* Multi-listener support for splitscreen and VR applications
* Real-time parameter control for dynamic audio experiences
* FMOD Studio integration with professional audio tools support

### Enhanced Developer Experience

* Comprehensive logging system with file rotation and filtering
* Runtime console with command registration and parameter tweaking
* Hot-reload capabilities for assets, scripts, and configuration
* Advanced debugging tools with performance profiling

## 🤝 Contributing

Contributions are welcome from developers interested in modern game engine architecture, graphics programming, 
multithreading, and JavaScript integration.

### Development Workflow

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/enhanced-lighting`)
3. Implement changes following C++20 best practices
4. Add comprehensive unit tests and documentation
5. Update relevant documentation
6. Submit a pull request with detailed description

### Coding Standards

* Follow C++20 Core Guidelines with modern language features
* Maintain consistent naming conventions (PascalCase for classes, camelCase for functions)
* Document all public APIs with comprehensive comments
* Ensure thread safety in multithreaded contexts
* Follow SOLID principles and data-oriented design patterns

## 📄 Documentation

* [**Engine Architecture Guide**](https://daemonengine.dadavidtseng.info/) - Complete engine overview
* [**API Reference**](Docs/source/index.rst) - Comprehensive API documentation
* [**V8 Scripting Guide**](Docs/scripting.md) - JavaScript integration tutorial
* [**JobSystem Documentation**](Docs/threading.md) - Multithreading architecture guide
* [**Audio System Guide**](Docs/audio.md) - 3D spatial audio implementation

## 📝 License

This project is licensed under the Apache License 2.0 - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

* **Google V8 Team**: For the powerful JavaScript engine and excellent documentation
* **FMOD**: For professional audio middleware and 3D spatial audio capabilities
* **Modern C++ Community**: Inspiration from C++20 best practices and design patterns
* **Game Engine Architecture**: Insights from established engines and real-time systems research
* **DirectX Community**: Advanced graphics programming techniques and optimization strategies

---
**Development Period:** 2022 - Present (Active Development)  
**Last Updated:** September 21, 2025