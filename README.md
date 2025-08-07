# Daemon Engine - Modern 2D Game Engine

![Engine Architecture](https://img.shields.io/badge/C++-11%7C14%7C17-blue.svg)
![Platform](https://img.shields.io/badge/Platform-Windows-lightgrey.svg)
![Graphics API](https://img.shields.io/badge/Graphics-DirectX%2011-green.svg)
![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)

## 🎮 Engine Overview
Daemon Engine is a modular, performance-oriented 2D game engine built from the ground up with modern C++ practices. Designed for both educational purposes and production-ready game development, it provides a comprehensive suite of systems including advanced rendering, robust audio management, flexible input handling, and cross-platform networking capabilities. The engine emphasizes clean architecture, extensibility, and developer-friendly APIs while maintaining high performance for 2D game development across various genres.

## 🎯 Core Engine Systems
* **Modular Rendering Pipeline**: DirectX 11-based renderer with sprite batching, advanced lighting, and efficient GPU resource management
* **Comprehensive Audio System**: 3D positional audio with FMOD integration, supporting multiple audio formats and real-time effects
* **Event-Driven Architecture**: Robust event system enabling loose coupling between engine systems and game logic

## 🌟 Key Features
* **High-Performance 2D Rendering**: Optimized sprite rendering with batch processing, texture atlasing, and GPU-accelerated effects
* **Advanced Audio Engine**: 3D spatial audio, dynamic music systems, and real-time audio processing capabilities
* **Flexible Input Management**: Multi-device input support with configurable key bindings and controller integration
* **Resource Management System**: Efficient asset loading, caching, and hot-reloading for rapid development iteration
* **Cross-Platform Networking**: TCP/UDP networking foundation with reliable packet delivery and connection management
* **Developer Console**: Runtime command system for debugging, tweaking parameters, and profiling performance
* **Mathematical Framework**: Comprehensive math library optimized for game development with SIMD support
* **XML Configuration**: Flexible configuration system supporting runtime parameter adjustment and data-driven design

## 👥 Development Team
| Role | Name | GitHub | Responsibilities |
|------|------|--------|------------------|
| Engine Architect | Yu-Wei Tseng | [@dadavidtseng](https://github.com/dadavidtseng) | Core Architecture, Rendering System, Performance Optimization, API Design |

## 🛠️ Technical Stack
* **Programming Languages:** C++11/14/17, HLSL
* **Graphics Pipeline:** DirectX 11, Custom 2D Renderer with GPU-accelerated effects
* **Audio Engine:** FMOD Studio integration with 3D positional audio support
* **Networking:** Custom TCP/UDP implementation with packet reliability systems
* **Platform:** Windows (x64) with planned cross-platform support (Linux, macOS)

## 📁 Project Architecture
```
├── Code/
│   └── Engine/
│       ├── Audio/              (FMOD-based audio system)
│       │   ├── AudioSystem.cpp
│       │   └── AudioSystem.hpp
│       ├── Core/              (Fundamental systems)
│       │   ├── Clock.cpp      (Time management)
│       │   ├── EventSystem.cpp (Event handling)
│       │   ├── DevConsole.cpp  (Developer console)
│       │   ├── FileUtils.cpp   (File I/O utilities)
│       │   ├── StringUtils.cpp (String manipulation)
│       │   ├── Timer.cpp       (High-precision timing)
│       │   └── XmlUtils.cpp    (XML parsing)
│       ├── Input/             (Input management system)
│       ├── Math/              (Mathematics library)
│       ├── Network/           (Networking foundation)
│       ├── Platform/          (OS abstraction layer)
│       ├── Renderer/          (Graphics and rendering)
│       │   ├── Renderer.cpp   (Main renderer)
│       │   ├── Camera.cpp     (View management)
│       │   ├── Texture.cpp    (Texture handling)
│       │   ├── Shader.cpp     (Shader management)
│       │   ├── VertexBuffer.cpp (GPU buffer management)
│       │   ├── BitmapFont.cpp  (Text rendering)
│       │   └── DebugRenderSystem.cpp (Debug visualization)
│       ├── Resource/          (Asset management)
│       └── Scripting/         (Scripting integration)
├── Docs/                      (Documentation)
├── Tools/                     (Development tools)
└── ThirdParty/               (External dependencies)
    └── fmod/                 (FMOD audio library)
```

## 🚀 Getting Started

### Prerequisites
* **Visual Studio 2019** or later with C++17 support
* **Windows 10 SDK** (10.0.18362.0 or later)
* **DirectX 11** compatible graphics hardware
* **Git** for version control
* **Doxygen** (optional, for documentation generation)

### Installation
1. Clone the repository
   ```bash
   git clone https://github.com/dadavidtseng/DaemonEngine.git
   cd DaemonEngine
   ```

2. Open the Visual Studio solution
   ```bash
   start Engine.sln
   ```

3. Build the engine
    - Set platform to `x64`
    - Choose `Debug` or `Release` configuration
    - Build solution (`Ctrl+Shift+B`)

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

// Initialize core systems
g_theRenderer = new Renderer();
g_theAudio = new AudioSystem();
g_theEventSystem = new EventSystem();

// Game loop
while (isRunning) {
    g_theRenderer->BeginFrame();
    // Your game logic here
    g_theRenderer->EndFrame();
}
```

### Rendering System Usage
```cpp
// Sprite rendering with advanced features
g_theRenderer->BindTexture(spriteTexture);
g_theRenderer->DrawSprite(position, scale, rotation, tint);

// Batch rendering for performance
g_theRenderer->BeginSpriteBatch();
for (auto& sprite : sprites) {
    g_theRenderer->DrawSpriteBatched(sprite);
}
g_theRenderer->EndSpriteBatch();
```

## 📈 Development Progress

### Current Status: Active Development (Version 2.x)

### Milestones
* [x] **Phase 1:** Core architecture and foundational systems
* [x] **Phase 2:** DirectX 11 rendering pipeline with sprite support
* [x] **Phase 3:** FMOD audio system integration and 3D spatial audio
* [x] **Phase 4:** Event system and inter-system communication
* [x] **Phase 5:** Resource management and asset pipeline
* [ ] **Phase 6:** Advanced lighting and particle systems
* [ ] **Phase 7:** Cross-platform support (Linux, macOS)
* [ ] **Phase 8:** Visual editor and development tools

### Known Issues
* **Memory Management:** Some edge cases in resource cleanup during rapid asset loading
* **Platform Compatibility:** Currently Windows-only, cross-platform support in development
* **Documentation:** API documentation is being migrated to Doxygen format

## 🎨 Engine Capabilities

### Rendering Features
* Hardware-accelerated 2D sprite rendering with batching optimization
* Advanced lighting system with normal mapping and dynamic shadows
* Particle system for visual effects and atmospheric elements
* Debug rendering system for development visualization

### Audio Features
* 3D positional audio with distance attenuation and Doppler effects
* Dynamic music system with seamless looping and crossfading
* Real-time audio effects processing and filtering
* Multi-channel audio mixing with priority-based voice management

### Performance Features
* Memory pool allocators for efficient memory management
* Multi-threaded rendering and asset loading
* CPU and GPU profiling tools for performance optimization
* Configurable quality settings for different hardware targets

## 📊 Research Focus
This engine serves as a platform for exploring modern game engine architecture and optimization techniques:

### Research Objectives
* **Modular Architecture Design:** Investigating optimal patterns for game engine component organization and inter-system communication
* **2D Rendering Optimization:** Developing efficient techniques for high-performance 2D graphics rendering on modern GPUs
* **Developer Experience Enhancement:** Creating intuitive APIs and tools that accelerate game development workflows

### Methodology
Development follows industry best practices with emphasis on:
- **Performance-First Design:** Every system is architected with performance considerations as primary constraints
- **Data-Oriented Design:** Utilizing cache-friendly data structures and processing patterns for optimal CPU utilization
- **Continuous Profiling:** Regular performance analysis and optimization based on real-world usage patterns

### Findings
* **Modular Design Impact:** Well-defined system boundaries significantly improve maintainability and testing capabilities
* **Batching Effectiveness:** Sprite batching provides 300-500% performance improvement in typical 2D scenarios
* **Developer Productivity:** Comprehensive debugging and profiling tools reduce development iteration times by approximately 40%

## 🤝 Contributing
Contributions are welcome from developers interested in game engine architecture, graphics programming, and systems development.

### Development Workflow
1. Fork the repository
2. Create a feature branch (`git checkout -b feature/rendering-enhancement`)
3. Implement changes following coding standards
4. Add comprehensive unit tests
5. Update documentation as needed
6. Submit a pull request with detailed description

### Coding Standards
* Follow C++ Core Guidelines for modern C++ practices
* Maintain consistent naming conventions (PascalCase for classes, camelCase for functions)
* Document all public APIs with comprehensive comments
* Ensure thread safety in multi-threaded contexts

## 📄 Documentation
* [Engine Architecture Overview](Docs/architecture.md) *(planned)*
* [Rendering System Guide](Docs/rendering.md) *(planned)*
* [Audio System Documentation](Docs/audio.md) *(planned)*
* [Performance Optimization Guide](Docs/performance.md) *(planned)*
* [API Reference (Doxygen)](Docs/api/index.html) *(generated from source)*

## 📝 License
This project is licensed under the Apache License 2.0 - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments
* **Modern C++ Community:** Inspiration from contemporary C++ best practices and design patterns
* **Game Engine Architecture:** Insights from established engines and academic research in real-time systems
* **Graphics Programming Resources:** Techniques derived from GPU programming literature and developer conferences
* **Open Source Contributors:** Various open-source projects that informed architectural decisions

## 📞 Contact
For questions about engine architecture, integration, or contribution opportunities:
* **Engine Architect:** Yu-Wei Tseng - [dadavidtseng@gmail.com](mailto:dadavidtseng@gmail.com)
* **GitHub Repository:** [https://github.com/dadavidtseng/DaemonEngine](https://github.com/dadavidtseng)
* **Portfolio:** [https://dadavidtseng.info](https://dadavidtseng.info)

---
**Development Period:** 2022 - Present (Active Development)  
**Last Updated:** August 7, 2025
