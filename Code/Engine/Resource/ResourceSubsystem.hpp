//----------------------------------------------------------------------------------------------------
// ResourceSubsystem.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include <functional>
#include <future>
#include <memory>
#include <vector>
#include "Game/EngineBuildPreferences.hpp"  // Phase 3: Required for ENGINE_SCRIPTING_ENABLED
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Resource/IResourceLoader.hpp"
#include "Engine/Resource/ResourceCache.hpp"
#include "Engine/Resource/ResourceHandle.hpp"
#include "Engine/Renderer/RenderCommon.hpp"

//----------------------------------------------------------------------------------------------------
// Forward declarations
class Texture;
class BitmapFont;
class Shader;
class Renderer;
class TextureResource;
class TextureLoader;
class JobSystem;

#ifdef ENGINE_SCRIPTING_ENABLED
class ResourceCommandQueue;
class CallbackQueue;
#endif

//----------------------------------------------------------------------------------------------------
struct sResourceSubsystemConfig
{
    Renderer* m_renderer    = nullptr;
    int       m_threadCount = 0;  // Deprecated: JobSystem now manages worker threads
};

//----------------------------------------------------------------------------------------------------
class ResourceSubsystem
{
public:
    explicit ResourceSubsystem(sResourceSubsystemConfig const& config);

    void Startup();
    void Shutdown();

    // 註冊載入器
    void RegisterLoader(std::unique_ptr<IResourceLoader> loader);

    // 同步載入資源
    template <typename T>
    ResourceHandle<T> LoadResource(String const& path)
    {
        // 檢查快取
        if (std::shared_ptr<IResource> const cached = m_cache.Get(path))
        {
            return ResourceHandle<T>(std::static_pointer_cast<T>(cached));
        }

        // 載入新資源
        if (std::shared_ptr<IResource> const resource = LoadResourceInternal(path))
        {
            m_cache.Add(path, resource);
            return ResourceHandle<T>(std::static_pointer_cast<T>(resource));
        }

        return ResourceHandle<T>();
    }

    // 異步載入資源
    // Phase 3: Uses std::async temporarily for API compatibility
    // TODO: Refactor to use JobSystem internally while maintaining std::future return type
    template <typename T>
    std::future<ResourceHandle<T>> LoadResourceAsync(String const& path)
    {
        // Note: std::async creates its own thread, bypassing JobSystem
        // This is acceptable for now to preserve existing C++ API
        // JavaScript resource loading uses ResourceCommandQueue → JobSystem path instead
        return std::async(std::launch::async, [this, path]()
        {
            return LoadResource<T>(path);
        });
    }

    // 預載入資源列表
    void PreloadResources(std::vector<std::string> const& paths);

    // 卸載未使用的資源
    void UnloadUnusedResources();

    // 取得記憶體使用情況
    size_t GetMemoryUsage() const;
    size_t GetResourceCount() const;

    // 設定記憶體限制
    void SetMemoryLimit(size_t bytes) { m_memoryLimit = bytes; }

    // Global singleton access methods
    // static void               Initialize(Renderer* renderer, sResourceSubsystemConfig const& config = sResourceSubsystemConfig());
    // static void               GlobalShutdown();
    // static ResourceSubsystem* Get() { return s_instance; }

    // Global resource access methods - delegates to Renderer for actual loading
     Texture*    CreateOrGetTextureFromFile(String const& path);
     BitmapFont* CreateOrGetBitmapFontFromFile(String const& path);
     Shader*     CreateOrGetShaderFromFile(String const& path, eVertexType vertexType = eVertexType::VERTEX_PCU);

    // Default texture access (Phase 4: Migration from Renderer)
     ResourceHandle<TextureResource> GetDefaultTexture();

    //------------------------------------------------------------------------------------------------
    // Phase 3: JobSystem Integration
    //------------------------------------------------------------------------------------------------
    // Set JobSystem pointer for async resource loading via I/O worker threads
    void SetJobSystem(JobSystem* jobSystem);

#ifdef ENGINE_SCRIPTING_ENABLED
    // Set ResourceCommandQueue and CallbackQueue for JavaScript integration
    // Must be called after SetJobSystem() if JavaScript resource loading is needed
    void SetCommandQueue(ResourceCommandQueue* commandQueue, CallbackQueue* callbackQueue);

    // Process pending resource loading commands from ResourceCommandQueue
    // Creates ResourceLoadJob instances and submits to JobSystem
    // Called from main thread (typically in App::Update or similar)
    void ProcessPendingCommands();
#endif

private:
    // Helper method to create default white texture
    void                     CreateDefaultTexture();
    sResourceSubsystemConfig m_config;

    // Singleton instance
    // static ResourceSubsystem* s_instance;
    // static Renderer*          s_renderer;

    // 禁止複製
    ResourceSubsystem(ResourceSubsystem const&)            = delete;
    ResourceSubsystem& operator=(ResourceSubsystem const&) = delete;

    // 內部載入方法
    std::shared_ptr<IResource> LoadResourceInternal(String const& path);
    String                     GetFileExtension(String const& path) const;

    ResourceCache                                 m_cache;
    std::vector<std::unique_ptr<IResourceLoader>> m_loaders;

    //------------------------------------------------------------------------------------------------
    // Phase 3: JobSystem Integration (replaces custom worker threads)
    //------------------------------------------------------------------------------------------------
    JobSystem* m_jobSystem = nullptr;  // JobSystem for async I/O operations

#ifdef ENGINE_SCRIPTING_ENABLED
    ResourceCommandQueue* m_commandQueue  = nullptr;  // Command queue from JavaScript
    CallbackQueue*        m_callbackQueue = nullptr;  // Callback queue to JavaScript
#endif

    // 記憶體管理
    size_t m_memoryLimit = 0;
};
