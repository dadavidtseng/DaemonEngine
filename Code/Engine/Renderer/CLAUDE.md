[Root Directory](../../../CLAUDE.md) > [Code](../../) > [Engine](../) > **Renderer**

# Renderer Module Documentation

## Module Responsibilities

The Renderer module provides a comprehensive DirectX 11-based graphics rendering system with advanced features including sprite batching, lighting, shader management, texture handling, and debug visualization. It serves as the primary interface for all graphics operations in the engine.

## Entry and Startup

### Primary Entry Point
- `Renderer.hpp` - Main renderer class and rendering pipeline
- `RenderCommon.hpp` - Shared rendering constants and enums
- `Camera.hpp` - View and projection management
- `CameraAPI.hpp` - High-level async camera management API
- `CameraScriptInterface.hpp` - JavaScript bindings for camera control
- `CameraStateBuffer.hpp` - Thread-safe camera state synchronization

### Initialization Pattern
```cpp
sRendererConfig config;
config.m_window = window;
Renderer* renderer = new Renderer(config);
renderer->Startup();

// Basic render loop
renderer->BeginFrame();
renderer->BeginCamera(camera);
// Render calls here
renderer->EndCamera(camera);
renderer->EndFrame();
```

## External Interfaces

### Core Rendering API
```cpp
class Renderer {
    // Frame management
    void BeginFrame();
    void EndFrame() const;
    void ClearScreen(Rgba8 const& clearColor) const;
    
    // Camera management
    void BeginCamera(Camera const& camera);
    void EndCamera(Camera const& camera);
    
    // Vertex rendering
    void DrawVertexArray(int numVertexes, Vertex_PCU const* vertexes);
    void DrawVertexArray(VertexList_PCU const& verts, IndexList const& indexes);
    
    // Textured rendering
    void DrawTexturedQuad(AABB2 const& bounds, Texture const* texture, 
                         Rgba8 const& tint, float uniformScaleXY, float rotationDegreesAboutZ);
};
```


### Camera Management API (M4-T8 Async Architecture)
```cpp
class CameraAPI {
    // Camera creation/destruction (async with callbacks)
    CallbackID CreateCamera(Vec3 const& position, EulerAngles const& orientation,
                           std::string const& type, ScriptCallback const& callback);
    CallbackID DestroyCamera(EntityID cameraId, ScriptCallback const& callback);
    
    // Camera updates (fire-and-forget)
    void UpdateCamera(EntityID cameraId, Vec3 const& position, 
                     EulerAngles const& orientation);  // RECOMMENDED: Atomic update
    void MoveCameraBy(EntityID cameraId, Vec3 const& delta);
    void LookAtCamera(EntityID cameraId, Vec3 const& target);
    
    // Camera state management
    CallbackID SetActiveCamera(EntityID cameraId, ScriptCallback const& callback);
    CallbackID UpdateCameraType(EntityID cameraId, std::string const& type,
                                ScriptCallback const& callback);
    uintptr_t GetCameraHandle(EntityID cameraId) const;
    
    // Callback execution (main thread)
    void ExecutePendingCallbacks();
    void NotifyCallbackReady(CallbackID callbackId, EntityID resultId);
};

// Usage from JavaScript (via CameraScriptInterface):
// camera.create({position: {x: -10, y: 0, z: 5}, orientation: {yaw: 0, pitch: 0, roll: 0},
//                type: 'world'}, (cameraId) => { console.log('Camera created:', cameraId); });
// camera.update(cameraId, {x: -5, y: 0, z: 3}, {yaw: 45, pitch: 0, roll: 0});
// camera.moveBy(cameraId, {dx: 1, dy: 0, dz: 0});
// camera.lookAt(cameraId, {x: 0, y: 0, z: 0});
```


```cpp
// Asset creation and caching
Image       CreateImageFromFile(char const* imageFilePath);
BitmapFont* CreateOrGetBitmapFontFromFile(char const* bitmapFontFilePathWithNoExtension);
Shader*     CreateOrGetShaderFromFile(char const* shaderFilePath, eVertexType vertexType);
Texture*    CreateOrGetTextureFromFile(char const* imageFilePath);
```

### GPU Resource Management
```cpp
// Buffer creation and management
ConstantBuffer* CreateConstantBuffer(unsigned int size) const;
IndexBuffer*    CreateIndexBuffer(unsigned int size, unsigned int stride) const;
VertexBuffer*   CreateVertexBuffer(unsigned int size, unsigned int stride) const;

// GPU data transfer
void CopyCPUToGPU(void const* data, unsigned int size, ConstantBuffer* cbo) const;
void CopyCPUToGPU(void const* data, unsigned int size, VertexBuffer* vbo) const;
```

## Key Dependencies and Configuration

### External Dependencies
- **DirectX 11**: Core graphics API (`d3d11.lib`, `dxgi.lib`)
- **D3DCompiler**: Shader compilation (`d3dcompiler.lib`)
- **STB Image**: Image loading library
- **Platform Module**: Window management integration

### Internal Dependencies
- Core module for utilities and color management
- Math module for matrices, vectors, and geometric operations
- Platform module for window system integration

### Configuration Structure
```cpp
struct sRendererConfig {
    Window* m_window = nullptr;  // Target window for rendering
};
```

### Rendering State Management
```cpp
enum class eBlendMode : int8_t { OPAQUE, ALPHA, ADDITIVE, COUNT };
enum class eDepthMode : int8_t { DISABLED, ENABLED, COUNT };
enum class eSamplerMode : int8_t { POINT_CLAMP, BILINEAR_WRAP, COUNT };
enum class eRasterizerMode : int8_t { SOLID_CULL_BACK, WIREFRAME, COUNT };
```

## Data Models

### Vertex Types
```cpp
// Primary vertex format for UI and 2D graphics
struct Vertex_PCU {
    Vec3  m_position;
    Rgba8 m_color;
    Vec2  m_uvTexCoords;
};

// Advanced vertex format for 3D graphics
struct Vertex_PCUTBN {
    Vec3  m_position;
    Rgba8 m_color;
    Vec2  m_uvTexCoords;
    Vec3  m_tangent;
    Vec3  m_binormal;
    Vec3  m_normal;
};
```

### Shader System
```cpp
class Shader {
    // Shader compilation and binding
    // Supports vertex, pixel, and geometry shaders
    // Automatic constant buffer binding
};

class ConstantBuffer {
    // GPU constant buffer wrapper
    // Automatic memory management
    // Efficient CPU to GPU data transfer
};
```

### Texture System
```cpp
class Texture {
    // DirectX 11 texture wrapper
    // Supports 2D textures, render targets
    // Automatic mipmap generation
    // Format conversion support
};

class Image {
    // CPU-side image representation
    // Format conversion utilities
    // File I/O integration
};
```

## Testing and Quality

### Debug Features
- **DebugRenderSystem**: Wireframe and debug primitive rendering
- **Performance Profiling**: Frame time and GPU usage tracking
- **State Validation**: DirectX state consistency checking
- **Memory Tracking**: GPU resource usage monitoring

### Quality Assurance
- Automatic DirectX error checking and reporting
- Shader compilation error handling
- Resource leak detection
- Performance warning system for inefficient operations

### Current Testing Approach
- Manual visual testing through debug renders
- Performance monitoring via frame rate analysis
- State validation through DirectX debug layer
- Memory usage tracking for GPU resources

### Recommended Testing Additions
- Automated visual regression tests
- Shader unit tests with known input/output
- Performance benchmarks for common operations
- Mock DirectX interfaces for unit testing

## FAQ

### Q: How do I optimize rendering performance?
A: Use vertex batching, minimize state changes, cache textures and shaders, and prefer indexed rendering over individual vertex arrays.

### Q: What coordinate system does the renderer use?
A: Right-handed coordinate system with Y-up for 3D, screen coordinates (0,0) at top-left for 2D UI.

### Q: How do I add custom shaders?
A: Create `.hlsl` files and use `CreateOrGetShaderFromFile()`. Shaders are automatically compiled and cached.

### Q: Can I render to multiple render targets?
A: Yes, the system supports custom render targets via `CreateRenderTexture()` and multiple render target binding.


### Q: How do I use the async Camera API?
A: Use CameraAPI::CreateCamera() with a callback for async creation. Update camera state with UpdateCamera() (atomic) or MoveCameraBy() (relative). Callbacks are executed via ExecutePendingCallbacks().

### Q: Why are there deprecated UpdateCameraPosition/Orientation methods?
A: Use UpdateCamera() for atomic position+orientation updates to avoid race conditions. The separate methods are kept for backward compatibility but may cause synchronization issues.

### Q: How does CameraStateBuffer work?
A: It uses StateBuffer template for double-buffering. Worker thread writes camera state to back buffer, main thread reads from front buffer for rendering, with SwapBuffers() at frame boundaries.


A: Set light data via `SetLightConstants()` which binds to shader constant buffers. Supports multiple light types.

## Related Files

### Core Rendering Files
- `Renderer.cpp` - Main renderer implementation
- `RenderCommon.cpp` - Shared rendering utilities
- `VertexUtils.cpp` - Vertex manipulation utilities
- `Camera.cpp` - View and projection matrices
- `DebugRenderSystem.cpp` - Debug visualization


### Camera Management API (M4-T8 Async Architecture)
```cpp
class CameraAPI {
    // Camera creation/destruction (async with callbacks)
    CallbackID CreateCamera(Vec3 const& position, EulerAngles const& orientation,
                           std::string const& type, ScriptCallback const& callback);
    CallbackID DestroyCamera(EntityID cameraId, ScriptCallback const& callback);
    
    // Camera updates (fire-and-forget)
    void UpdateCamera(EntityID cameraId, Vec3 const& position, 
                     EulerAngles const& orientation);  // RECOMMENDED: Atomic update
    void MoveCameraBy(EntityID cameraId, Vec3 const& delta);
    void LookAtCamera(EntityID cameraId, Vec3 const& target);
    
    // Camera state management
    CallbackID SetActiveCamera(EntityID cameraId, ScriptCallback const& callback);
    CallbackID UpdateCameraType(EntityID cameraId, std::string const& type,
                                ScriptCallback const& callback);
    uintptr_t GetCameraHandle(EntityID cameraId) const;
    
    // Callback execution (main thread)
    void ExecutePendingCallbacks();
    void NotifyCallbackReady(CallbackID callbackId, EntityID resultId);
};

// Usage from JavaScript (via CameraScriptInterface):
// camera.create({position: {x: -10, y: 0, z: 5}, orientation: {yaw: 0, pitch: 0, roll: 0},
//                type: 'world'}, (cameraId) => { console.log('Camera created:', cameraId); });
// camera.update(cameraId, {x: -5, y: 0, z: 3}, {yaw: 45, pitch: 0, roll: 0});
// camera.moveBy(cameraId, {dx: 1, dy: 0, dz: 0});
// camera.lookAt(cameraId, {x: 0, y: 0, z: 0});
```


- `Texture.cpp` - Texture loading and management
- `Shader.cpp` - Shader compilation and binding
- `Image.cpp` - Image loading and processing
- `BitmapFont.cpp` - Text rendering system

### GPU Resources
- `VertexBuffer.cpp` - Vertex buffer management
- `IndexBuffer.cpp` - Index buffer management
- `ConstantBuffer.cpp` - Constant buffer management

### Vertex Types
- `Vertex_PCU.cpp` - Basic vertex format
- `Vertex_PCUTBN.cpp` - Advanced vertex format with normals

### Specialized Systems
- `Light.cpp` - Lighting system
- `SpriteSheet.cpp` - Sprite animation support
- `SpriteDefinition.cpp` - Sprite management
- `SpriteAnimDefinition.cpp` - Animation definitions

### Shader Integration
- `DefaultShader.hpp` - Default shader definitions


### Camera API (M4-T8 Async Architecture)
- `CameraAPI.cpp` - High-level async camera management
- `CameraAPI.hpp` - Camera API interface
- `CameraScriptInterface.cpp` - JavaScript camera bindings
- `CameraScriptInterface.hpp` - Camera script interface declarations
- `CameraStateBuffer.cpp` - Thread-safe camera state management
- `CameraStateBuffer.hpp` - Camera state double-buffering
- `CameraState.hpp` - Camera state data structure

 - JavaScript binding for renderer
- `RendererScriptInterface.hpp` - Script interface declarations for V8 integration

## Changelog

- 2025-10-27: **M4-T8 Async Architecture Refactoring**
  - Added CameraAPI for high-level async camera management
  - Added CameraScriptInterface for JavaScript camera control
  - Introduced CameraStateBuffer for thread-safe camera state synchronization
  - Refactored camera system for double-buffered async updates
  - Separated camera concerns from entity management (Single Responsibility Principle)
  - Camera creation/updates now async with callback support
  - Lock-free camera state reads for rendering

- 2025-10-07: **JavaScript Scripting Interface and Resource Leak Detection**
  - Added `RendererScriptInterface` class for exposing Renderer APIs to JavaScript/V8
  - JavaScript API for rendering state control (blend, rasterizer, sampler, depth modes)
  - JavaScript API for resource binding (textures, shaders) and draw operations
  - Integrated Renderer with ResourceSubsystem for texture loading
  - Added static leak tracking to `Texture` class with `ReportLeakStatus()`
  - Added static leak tracking to `BitmapFont` class with `ReportLeakStatus()`
  - Improved shutdown sequence: leak reports before DirectX cleanup
  - Added `m_ownsDefaultTexture` flag to track default texture ownership
  - Renderer now attempts to load default texture from ResourceSubsystem
  - Fallback to local texture creation if ResourceSubsystem unavailable
  - Commented out direct texture loading methods (now use ResourceSubsystem)
  - Enhanced resource ownership management for proper cleanup
- 2025-09-06 19:54:50: Initial module documentation created
- Recent developments: RendererScriptInterface for JavaScript integration, resource leak detection, ResourceSubsystem integration, VertexUtils enhancements, AABB3 integration for 3D rendering