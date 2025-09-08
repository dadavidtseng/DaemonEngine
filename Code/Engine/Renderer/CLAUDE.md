[Root Directory](../../../CLAUDE.md) > [Code](../../) > [Engine](../) > **Renderer**

# Renderer Module Documentation

## Module Responsibilities

The Renderer module provides a comprehensive DirectX 11-based graphics rendering system with advanced features including sprite batching, lighting, shader management, texture handling, and debug visualization. It serves as the primary interface for all graphics operations in the engine.

## Entry and Startup

### Primary Entry Point
- `Renderer.hpp` - Main renderer class and rendering pipeline
- `RenderCommon.hpp` - Shared rendering constants and enums
- `Camera.hpp` - View and projection management

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

### Resource Management
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

### Q: How does the lighting system work?
A: Set light data via `SetLightConstants()` which binds to shader constant buffers. Supports multiple light types.

## Related Files

### Core Rendering Files
- `Renderer.cpp` - Main renderer implementation
- `RenderCommon.cpp` - Shared rendering utilities
- `VertexUtils.cpp` - Vertex manipulation utilities
- `Camera.cpp` - View and projection matrices
- `DebugRenderSystem.cpp` - Debug visualization

### Resource Management
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

## Changelog

- 2025-09-06 19:54:50: Initial module documentation created
- Recent developments: VertexUtils enhancements, AABB3 integration for 3D rendering