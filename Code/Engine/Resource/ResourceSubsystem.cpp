//----------------------------------------------------------------------------------------------------
// ResourceSubsystem.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Resource/ResourceSubsystem.hpp"
#include <filesystem>
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Renderer/Image.hpp"
#include "Engine/Resource/ObjModelLoader.hpp"
#include "Engine/Resource/TextureLoader.hpp"
#include "Engine/Resource/FontLoader.hpp"
#include "Engine/Resource/ShaderLoader.hpp"
#include "Engine/Resource/TextureResource.hpp"
#include "Engine/Resource/FontResource.hpp"
#include "Engine/Resource/ShaderResource.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/Shader.hpp"

// Phase 3: JobSystem Integration
#include "Engine/Core/JobSystem.hpp"
#include "Engine/Resource/ResourceLoadJob.hpp"

#ifdef ENGINE_SCRIPTING_ENABLED
#include "Engine/Resource/ResourceCommandQueue.hpp"
#include "Engine/Core/CallbackQueue.hpp"
#endif

//----------------------------------------------------------------------------------------------------
ResourceSubsystem::ResourceSubsystem(sResourceSubsystemConfig const& config)
    : m_config(config)
{
}


void ResourceSubsystem::Shutdown()
{
    DebuggerPrintf("[ResourceSubsystem] Shutdown: Starting shutdown process\n");

    // Phase 3: No custom worker threads to stop (JobSystem manages worker threads)
    // JobSystem will be shut down by the application, not by ResourceSubsystem

    DebuggerPrintf("[ResourceSubsystem] Shutdown: Clearing cache and loaders\n");

    // 清理資源
    m_cache.Clear();
    m_loaders.clear();

    m_config.m_renderer = nullptr;
    m_jobSystem         = nullptr;

#ifdef ENGINE_SCRIPTING_ENABLED
    m_commandQueue  = nullptr;
    m_callbackQueue = nullptr;
#endif

    DebuggerPrintf("[ResourceSubsystem] Shutdown: Shutdown complete\n");
}

void ResourceSubsystem::RegisterLoader(std::unique_ptr<IResourceLoader> loader)
{
    m_loaders.push_back(std::move(loader));
}

// TODO: handle this with
std::shared_ptr<IResource> ResourceSubsystem::LoadResourceInternal(String const& path)
{
    String extension = GetFileExtension(path);

    // 尋找適合的載入器
    for (std::unique_ptr<IResourceLoader> const& loader : m_loaders)
    {
        if (loader->CanLoad(extension))
        {
            return loader->Load(path);
        }
    }

    ERROR_RECOVERABLE(Stringf("No loader found for file: %s", path.c_str()))
    return nullptr;
}

std::string ResourceSubsystem::GetFileExtension(String const& path) const
{
    std::filesystem::path filePath(path);
    return filePath.extension().string();
}

//----------------------------------------------------------------------------------------------------
// Phase 3: WorkerThread() REMOVED - JobSystem now handles worker thread management
//----------------------------------------------------------------------------------------------------

void ResourceSubsystem::PreloadResources(const std::vector<std::string>& paths)
{
    // Phase 3: Use JobSystem for async loading instead of custom worker threads
    if (!m_jobSystem)
    {
        DebuggerPrintf("Warning: PreloadResources called without JobSystem - loading synchronously\n");
        for (const auto& path : paths)
        {
            LoadResourceInternal(path);
        }
        return;
    }

    // Submit jobs to JobSystem for async loading
    for (const auto& path : paths)
    {
        // Create a simple lambda-based job for preloading
        // Note: This uses std::async temporarily until we create a dedicated PreloadJob class
        std::async(std::launch::async, [this, path]()
        {
            LoadResourceInternal(path);
        });
    }
}

void ResourceSubsystem::UnloadUnusedResources()
{
    m_cache.RemoveUnused();

    // 檢查記憶體限制
    if (m_memoryLimit > 0 && GetMemoryUsage() > m_memoryLimit)
    {
        // 實作 LRU 或其他策略來釋放資源
        // 這裡簡單地移除所有未使用的資源
        m_cache.RemoveUnused();
    }
}

size_t ResourceSubsystem::GetMemoryUsage() const
{
    return m_cache.GetMemoryUsage();
}

size_t ResourceSubsystem::GetResourceCount() const
{
    return m_cache.GetSize();
}

//----------------------------------------------------------------------------------------------------
// Global singleton methods
//----------------------------------------------------------------------------------------------------

// Static member definitions
// ResourceSubsystem* ResourceSubsystem::s_instance = nullptr;
// Renderer* ResourceSubsystem::s_renderer = nullptr;

//----------------------------------------------------------------------------------------------------
void ResourceSubsystem::Startup()
{
    // Phase 3: m_running removed - JobSystem manages worker thread lifecycle

    if (m_config.m_renderer != nullptr)
    {
        ID3D11Device* device = m_config.m_renderer->m_device;
        if (device)
        {
            RegisterLoader(std::make_unique<ObjModelLoader>());

            auto textureLoader = std::make_unique<TextureLoader>(device);
            RegisterLoader(std::move(textureLoader));
            DebuggerPrintf("Info: ResourceSubsystem initialized with TextureLoader.\n");

            auto fontLoader = std::make_unique<FontLoader>(m_config.m_renderer);
            RegisterLoader(std::move(fontLoader));
            DebuggerPrintf("Info: ResourceSubsystem initialized with FontLoader.\n");

            auto shaderLoader = std::make_unique<ShaderLoader>(device);
            RegisterLoader(std::move(shaderLoader));
            DebuggerPrintf("Info: ResourceSubsystem initialized with ShaderLoader.\n");

            // Phase 4: Create default texture AFTER loaders are registered
            CreateDefaultTexture();
        }
        else
        {
            DebuggerPrintf("Warning: ResourceSubsystem could not get D3D device from Renderer.\n");
        }
    }
    else
    {
        DebuggerPrintf("Warning: ResourceSubsystem initialized without Renderer.\n");
    }

    // Phase 3: Custom worker thread creation REMOVED
    // JobSystem must be set via SetJobSystem() for async resource loading
    DebuggerPrintf("Info: ResourceSubsystem startup complete. Call SetJobSystem() for async loading support.\n");
}

// void ResourceSubsystem::Initialize(Renderer* renderer, sResourceSubsystemConfig const& config)
// {
//     s_instance->Startup();
//
//     // Register standard loaders with device access
// }
//
// void ResourceSubsystem::GlobalShutdown()
// {
//     if (s_instance)
//     {
//         s_instance->Shutdown();
//         delete s_instance;
//         s_instance = nullptr;
//         s_renderer = nullptr;
//         DebuggerPrintf("Info: ResourceSubsystem shut down.\n");
//     }
// }

Texture* ResourceSubsystem::CreateOrGetTextureFromFile(String const& path)
{

    try
    {
        // Load texture through ResourceSubsystem
        auto textureHandle = LoadResource<TextureResource>(path);

        if (textureHandle.IsValid() && textureHandle.Get())
        {
            TextureResource* textureResource = textureHandle.Get();
            Texture*         rendererTexture = textureResource->GetRendererTexture();

            if (rendererTexture)
            {
                return rendererTexture;
            }
        }
    }
    catch (...)
    {
        DebuggerPrintf("Warning: ResourceSubsystem failed to load texture '%s'.\n", path.c_str());
    }

    return nullptr;
}

BitmapFont* ResourceSubsystem::CreateOrGetBitmapFontFromFile(String const& path)
{
        try
    {
        // Load font through ResourceSubsystem
        auto fontHandle = LoadResource<FontResource>(path);

        if (fontHandle.IsValid() && fontHandle.Get())
        {
            FontResource* fontResource       = fontHandle.Get();
            BitmapFont*   rendererBitmapFont = fontResource->GetRendererBitmapFont();

            if (rendererBitmapFont)
            {
                return rendererBitmapFont;
            }
        }
    }
    catch (...)
    {
        DebuggerPrintf("Warning: ResourceSubsystem failed to load font '%s'.\n", path.c_str());
    }

    return nullptr;
}

Shader* ResourceSubsystem::CreateOrGetShaderFromFile(String const& path, eVertexType vertexType)
{
    // if (!s_instance || !s_renderer)
    // {
    //     DebuggerPrintf("Warning: ResourceSubsystem not initialized, falling back to Renderer.\n");
    //     return s_renderer ? s_renderer->CreateOrGetShaderFromFile(path.c_str(), vertexType) : nullptr;
    // }

    try
    {
        // Check cache first with vertex type in key
        String cacheKey = path + "_" + std::to_string(static_cast<int>(vertexType));
        if (std::shared_ptr<IResource> const cached = m_cache.Get(cacheKey))
        {
            if (ShaderResource* shaderResource = static_cast<ShaderResource*>(cached.get()))
            {
                return shaderResource->GetRendererShader();
            }
        }

        // Find ShaderLoader and load with vertex type
        for (std::unique_ptr<IResourceLoader> const& loader : m_loaders)
        {
            if (ShaderLoader* shaderLoader = dynamic_cast<ShaderLoader*>(loader.get()))
            {
                if (shaderLoader->CanLoad(GetFileExtension(path)))
                {
                    auto shaderResource = shaderLoader->LoadShader(path, vertexType);
                    if (shaderResource)
                    {
                        m_cache.Add(cacheKey, shaderResource);
                        if (ShaderResource* shader = static_cast<ShaderResource*>(shaderResource.get()))
                        {
                            return shader->GetRendererShader();
                        }
                    }
                }
                break;
            }
        }
    }
    catch (...)
    {
        DebuggerPrintf("Warning: ResourceSubsystem failed to load shader '%s'.\n", path.c_str());
    }

    return nullptr;
}

//----------------------------------------------------------------------------------------------------
// Phase 3: JobSystem Integration Methods
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
void ResourceSubsystem::SetJobSystem(JobSystem* jobSystem)
{
    m_jobSystem = jobSystem;
    DebuggerPrintf("[ResourceSubsystem] JobSystem set for async resource loading.\n");
}

#ifdef ENGINE_SCRIPTING_ENABLED
//----------------------------------------------------------------------------------------------------
void ResourceSubsystem::SetCommandQueue(ResourceCommandQueue* commandQueue, CallbackQueue* callbackQueue)
{
    m_commandQueue  = commandQueue;
    m_callbackQueue = callbackQueue;

    if (m_commandQueue && m_callbackQueue)
    {
        DebuggerPrintf("[ResourceSubsystem] ResourceCommandQueue and CallbackQueue set for JavaScript integration.\n");
    }
    else
    {
        DebuggerPrintf("[ResourceSubsystem] Warning: SetCommandQueue called with null pointers.\n");
    }
}

//----------------------------------------------------------------------------------------------------
void ResourceSubsystem::ProcessPendingCommands()
{
    // Validate dependencies
    if (!m_commandQueue || !m_callbackQueue)
    {
        DebuggerPrintf("[ResourceSubsystem] Warning: ProcessPendingCommands called without command/callback queues.\n");
        return;
    }

    if (!m_jobSystem)
    {
        DebuggerPrintf("[ResourceSubsystem] Warning: ProcessPendingCommands called without JobSystem - commands cannot be processed.\n");
        return;
    }

    // Consume all pending resource commands from ResourceCommandQueue
    m_commandQueue->ConsumeAll([this](sResourceCommand const& cmd)
    {
        // Create ResourceLoadJob for each command
        auto* job = new ResourceLoadJob(cmd, this, m_callbackQueue);

        // Submit job to JobSystem for execution on I/O worker thread
        m_jobSystem->SubmitJob(job);
    });
}
#endif

//----------------------------------------------------------------------------------------------------
// Phase 4: Default Texture Management
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
void ResourceSubsystem::CreateDefaultTexture()
{
    // Check if Renderer and device are available
    // if (!s_renderer || !s_renderer->m_device)
    // {
    //     DebuggerPrintf("Warning: Cannot create default texture - Renderer or D3D device not available.\n");
    //     return;
    // }

    // Create a 2x2 white texture (matching Renderer's original implementation)
    Image const defaultImage(IntVec2(2, 2), Rgba8::WHITE);

    // Find TextureLoader
    TextureLoader* textureLoader = nullptr;
    for (std::unique_ptr<IResourceLoader> const& loader : m_loaders)
    {
        if (TextureLoader* tl = dynamic_cast<TextureLoader*>(loader.get()))
        {
            textureLoader = tl;
            break;
        }
    }

    if (!textureLoader)
    {
        DebuggerPrintf("Error: Cannot create default texture - TextureLoader not found.\n");
        return;
    }

    // Create TextureResource
    auto defaultTextureRes = std::make_shared<TextureResource>("__default_white__", eResourceType::TEXTURE);

    // Create Renderer::Texture from image using TextureLoader's method
    // Note: We're accessing private method CreateTextureFromImage, but TextureLoader should expose a public method
    // For now, we'll create the texture directly using Renderer's method temporarily
    Texture* texture = m_config.m_renderer->CreateTextureFromImage(defaultImage);
    if (texture)
    {
        texture->m_name = "__default_white__";
        defaultTextureRes->SetRendererTexture(texture);
        defaultTextureRes->SetName("__default_white__");

        // Cache as special resource
        m_cache.Add("__default_white__", defaultTextureRes);

        DebuggerPrintf("[ResourceSubsystem] Created default white texture.\n");
    }
    else
    {
        DebuggerPrintf("Error: Failed to create default white texture.\n");
    }
}

//----------------------------------------------------------------------------------------------------
ResourceHandle<TextureResource> ResourceSubsystem::GetDefaultTexture()
{
    // if (!s_instance)
    // {
    //     DebuggerPrintf("Error: ResourceSubsystem not initialized - cannot get default texture.\n");
    //     return ResourceHandle<TextureResource>();
    // }

    // return LoadResource<TextureResource>("__default_white__");
    return LoadResource<TextureResource>("Data/Images/TestUV.png");
}
