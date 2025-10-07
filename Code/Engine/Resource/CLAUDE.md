[Root Directory](../../../CLAUDE.md) > [Code](../../) > [Engine](../) > **Resource**

# Resource Module Documentation

## Module Responsibilities

The Resource module provides a comprehensive asset loading, caching, and management system with multi-threaded loading capabilities, memory management, and extensible loader architecture supporting textures, models, shaders, audio, fonts, animations, and materials.

## Entry and Startup

### Primary Entry Points
- `ResourceSubsystem.hpp` - Main resource management system
- `ResourceHandle.hpp` - Smart pointer wrapper for resources
- `ResourceCache.hpp` - Efficient resource caching system
- `IResourceLoader.hpp` - Extensible loader interface

### Initialization Pattern
```cpp
sResourceSubsystemConfig config;
config.m_threadCount = 4; // Multi-threaded loading
ResourceSubsystem* resourceSystem = new ResourceSubsystem(config);
resourceSystem->Startup();

// Register loaders for different asset types
resourceSystem->RegisterLoader(std::make_unique<TextureLoader>());
resourceSystem->RegisterLoader(std::make_unique<ModelLoader>());
resourceSystem->RegisterLoader(std::make_unique<AudioLoader>());
resourceSystem->RegisterLoader(std::make_unique<ShaderLoader>());

// Synchronous loading
auto textureHandle = resourceSystem->LoadResource<TextureResource>("Data/Textures/player.png");
if (textureHandle.IsValid()) {
    TextureResource* texture = textureHandle.Get();
    // Use texture...
}

// Asynchronous loading
auto futureModel = resourceSystem->LoadResourceAsync<ModelResource>("Data/Models/character.obj");
// Continue with other work...
auto modelHandle = futureModel.get(); // Wait for completion
```

## External Interfaces

### Core Resource Management API
```cpp
class ResourceSubsystem {
    // System lifecycle
    void Startup();
    void Shutdown();
    
    // Loader registration
    void RegisterLoader(std::unique_ptr<IResourceLoader> loader);
    
    // Synchronous resource loading
    template<typename T>
    ResourceHandle<T> LoadResource(String const& path);
    
    // Asynchronous resource loading
    template<typename T>
    std::future<ResourceHandle<T>> LoadResourceAsync(String const& path);
    
    // Bulk operations
    void PreloadResources(std::vector<std::string> const& paths);
    void UnloadUnusedResources();
    
    // Memory management
    size_t GetMemoryUsage() const;
    size_t GetResourceCount() const;
    void SetMemoryLimit(size_t bytes);
};
```

### Resource Handle System
```cpp
template<typename T>
class ResourceHandle {
    // Smart pointer semantics
    T* Get() const;
    T* operator->() const;
    T& operator*() const;
    
    // Validity and reference counting
    bool IsValid() const;
    bool IsLoaded() const;
    int GetReferenceCount() const;
    
    // Resource identification
    String GetPath() const;
    size_t GetResourceId() const;
};
```

### Extensible Loader Interface
```cpp
class IResourceLoader {
    // Loader capabilities
    virtual bool CanLoad(String const& extension) const = 0;
    virtual String GetLoaderName() const = 0;
    
    // Resource loading
    virtual std::shared_ptr<IResource> LoadResource(String const& path) = 0;
    
    // Memory estimation
    virtual size_t EstimateMemoryUsage(String const& path) const = 0;
};
```

## Key Dependencies and Configuration

### External Dependencies
- **Standard Threading Libraries**: `<thread>`, `<future>`, `<condition_variable>`
- **STB Libraries**: For image loading (stb_image)
- **File System**: For asset path resolution and validation

### Internal Dependencies
- Core module for string utilities and basic types
- Platform module for file system access
- Math module for geometry and vector operations

### Configuration Structure
```cpp
struct sResourceSubsystemConfig {
    int m_threadCount = 0;  // 0 = single-threaded, >0 = multi-threaded
};
```

### Resource Type System
```cpp
// Resource type enumeration (engine naming convention)
enum class eResourceType : int8_t {
    TEXTURE,
    MODEL,
    SHADER,
    AUDIO,
    FONT,
    ANIMATION,
    MATERIAL
};

// Resource state enumeration
enum class eResourceState : int8_t {
    Unloaded,    // Resource not yet loaded
    Loading,     // Currently loading
    Loaded,      // Successfully loaded
    Failed       // Load failed
};

// Base resource interface (modern smart pointer approach)
class IResource {
    virtual ~IResource() = default;
    virtual eResourceType GetType() const = 0;
    virtual eResourceState GetState() const = 0;
    virtual size_t GetMemorySize() const = 0;
    virtual bool Load() = 0;
    virtual void Unload() = 0;

protected:
    uint32_t                    m_id;
    std::string                 m_path;
    eResourceType               m_type;
    std::atomic<eResourceState> m_state = eResourceState::Unloaded;
    size_t                      m_memorySize = 0;
};
```

## Data Models

### Built-in Resource Types
```cpp
// Texture resources
class TextureResource : public IResource {
    // DirectX texture data and metadata
    ID3D11Texture2D* m_texture;
    ID3D11ShaderResourceView* m_shaderResourceView;
    int m_width, m_height, m_channels;
};

// Model/Geometry resources
class ModelResource : public IResource {
    // Vertex and index data with material assignments
    std::vector<Vertex_PCUTBN> m_vertices;
    std::vector<unsigned int> m_indices;
    std::vector<MaterialAssignment> m_materials;
};

// Shader resources
class ShaderResource : public IResource {
    // Compiled shader bytecode and reflection data
    ID3D11VertexShader* m_vertexShader;
    ID3D11PixelShader* m_pixelShader;
    ShaderReflectionData m_reflectionData;
};

// Material resources
class MaterialResource : public IResource {
    // Material properties and texture references
    ResourceHandle<TextureResource> m_diffuseTexture;
    ResourceHandle<TextureResource> m_normalMap;
    MaterialProperties m_properties;
};

// Audio resources
class AudioSource : public IResource {
    // Audio data and FMOD sound objects
    FMOD::Sound* m_fmodSound;
    AudioFormat m_format;
    float m_duration;
};

// Font resources
class FontResource : public IResource {
    // Bitmap font data and character metrics
    ResourceHandle<TextureResource> m_fontTexture;
    std::map<char, GlyphData> m_glyphMap;
};

// Animation resources
class AnimationResource : public IResource {
    // Keyframe animation data
    std::vector<AnimationChannel> m_channels;
    float m_duration;
    float m_ticksPerSecond;
};
```

### Resource Cache Management
```cpp
class ResourceCache {
    // Thread-safe resource storage
    std::shared_ptr<IResource> Get(String const& path);
    void Add(String const& path, std::shared_ptr<IResource> resource);
    void Remove(String const& path);
    
    // Memory management
    void ClearUnused();
    size_t GetTotalMemoryUsage() const;
    
private:
    std::unordered_map<String, std::weak_ptr<IResource>> m_cache;
    mutable std::shared_mutex m_cacheMutex;
};
```

### Multi-threaded Loading System
```cpp
// Worker thread management
std::vector<std::thread> m_workerThreads;
std::queue<std::function<void()>> m_taskQueue;
std::mutex m_queueMutex;
std::condition_variable m_condition;
bool m_running = false;

void WorkerThread(); // Thread entry point for background loading
```

## Testing and Quality

### Built-in Quality Features
- **Memory Usage Tracking**: Comprehensive monitoring of resource memory consumption
- **Reference Counting**: Automatic cleanup of unused resources
- **Thread Safety**: Robust multi-threaded access with proper synchronization
- **Error Handling**: Graceful handling of loading failures and invalid resources

### Current Testing Approach
- Manual resource loading verification through asset inspection
- Memory usage monitoring during gameplay sessions
- Performance profiling of loading operations
- Multi-threaded safety validation

### Quality Assurance Features
- Automatic memory limit enforcement with configurable thresholds
- Resource leak detection through reference tracking
- Loading failure recovery with fallback mechanisms
- Comprehensive logging of resource operations

### Recommended Testing Additions
- Automated resource loading regression tests
- Performance benchmarks for different resource types
- Memory pressure testing with large asset sets
- Cross-platform asset format compatibility testing

## FAQ

### Q: How do I add support for a new resource type?
A: 1) Create a class inheriting from `IResource`, 2) Implement a corresponding loader inheriting from `IResourceLoader`, 3) Register the loader with the ResourceSubsystem.

### Q: When should I use synchronous vs asynchronous loading?
A: Use synchronous loading for critical assets needed immediately, asynchronous loading for background streaming and non-blocking asset preparation.

### Q: How does memory management work with ResourceHandle?
A: ResourceHandle uses shared_ptr semantics with automatic cleanup when all handles to a resource are destroyed, integrated with the cache cleanup system.

### Q: Can I reload resources at runtime?
A: The system supports runtime reloading by clearing cached resources and re-loading from disk, useful for content iteration during development.

### Q: How do I optimize memory usage for large asset sets?
A: Use `SetMemoryLimit()` for automatic cleanup, `UnloadUnusedResources()` for manual cleanup, and asynchronous loading to spread memory allocation over time.

### Q: Is the resource system thread-safe?
A: Yes, all public APIs are thread-safe with proper synchronization, allowing safe access from multiple threads including the rendering thread.

## Related Files

### Core System Files
- `ResourceSubsystem.cpp` - Main resource management implementation
- `ResourceCache.cpp` - Resource caching and memory management
- `ResourceHandle.cpp` - Smart handle implementation
- `ResourceCommon.cpp` - Shared utilities and constants

### Base Classes and Interfaces
- `IResource.cpp` - Base resource interface implementation
- `IResourceLoader.cpp` - Base loader interface and utilities

### Specific Resource Loaders
- `TextureLoader.cpp` - Image and texture loading (PNG, JPG, DDS, etc.)
- `ModelLoader.cpp` - 3D model loading infrastructure
- `ObjModelLoader.cpp` - Wavefront OBJ format support
- `ShaderLoader.cpp` - HLSL shader compilation and loading
- `AudioLoader.cpp` - Audio file loading with FMOD integration
- `FontLoader.cpp` - Bitmap font loading and glyph extraction
- `MaterialLoader.cpp` - Material definition and texture binding
- `AnimationLoader.cpp` - Animation data loading and processing

### Resource Type Implementations
- `TextureResource.cpp` - DirectX texture resource management
- `ModelResource.cpp` - 3D geometry and mesh data
- `ShaderResource.cpp` - Compiled shader management
- `MaterialResource.cpp` - Material properties and texture references
- `AudioSource.cpp` - Audio data and FMOD integration
- `FontResource.cpp` - Font rendering and character metrics
- `AnimationResource.cpp` - Keyframe animation data

### Integration Points
The Resource module integrates with:
- **Renderer Module**: For texture, shader, and model resource creation
- **Audio System**: Through AudioLoader and AudioSource resources
- **Platform Module**: For file system access and path resolution
- **Core Module**: For string utilities and basic data structures

## Changelog

- 2025-10-07: **Resource Subsystem Architecture Modernization**
  - Removed manual reference counting in favor of std::shared_ptr for automatic lifetime management
  - Eliminated static singleton pattern from ResourceSubsystem (no more `s_instance`, `s_renderer`)
  - Injected Renderer dependency through constructor config instead of static member
  - Simplified ResourceHandle by removing custom ref counting logic (now wraps shared_ptr)
  - Renamed `ResourceType` → `eResourceType` (engine naming convention)
  - Renamed `ResourceState` → `eResourceState` (consistency with other enums)
  - Removed `IResource::AddRef()`, `Release()`, and `GetRefCount()` (now implicit in shared_ptr)
  - ResourceCache now uses `std::weak_ptr` for automatic cleanup of unreferenced resources
  - All resource loaders return `std::shared_ptr<IResource>` instead of raw pointers
  - Improved thread safety through std::shared_ptr's atomic ref counting
  - Enhanced shutdown sequence with comprehensive debug logging
- 2025-09-06 21:17:11: Initial Resource module documentation created
- Recent developments: Multi-threaded loading system, comprehensive resource type support, memory management improvements, modern C++ smart pointer adoption