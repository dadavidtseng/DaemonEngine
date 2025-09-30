//----------------------------------------------------------------------------------------------------
// ResourceSubsystem.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Resource/ResourceSubsystem.hpp"
#include <filesystem>
#include "Engine/Core/ErrorWarningAssert.hpp"
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

//----------------------------------------------------------------------------------------------------
ResourceSubsystem::ResourceSubsystem(sResourceSubsystemConfig const& config)
    : m_config(config)
{
}

//----------------------------------------------------------------------------------------------------
void ResourceSubsystem::Startup()
{
    m_running = true;

    // 註冊預設的載入器
    RegisterLoader(std::make_unique<ObjModelLoader>());
    // 未來可以加入更多載入器
    // RegisterLoader(std::make_unique<TextureLoader>());
    // RegisterLoader(std::make_unique<ShaderLoader>());

    // 創建工作執行緒
    for (int i = 0; i < m_config.m_threadCount; ++i)
    {
        m_workerThreads.emplace_back(&ResourceSubsystem::WorkerThread, this);
    }
}


void ResourceSubsystem::Shutdown()
{
    // 停止工作執行緒
    {
        std::unique_lock lock(m_queueMutex);
        m_running = false;
    }
    m_condition.notify_all();

    // 等待所有執行緒結束
    for (auto& thread : m_workerThreads)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }

    // 清理資源
    m_cache.Clear();
    m_loaders.clear();
}

void ResourceSubsystem::RegisterLoader(std::unique_ptr<IResourceLoader> loader)
{
    m_loaders.push_back(std::move(loader));
}

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

void ResourceSubsystem::WorkerThread()
{
    while (m_running)
    {
        std::function<void()> task;

        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_condition.wait(lock, [this] { return !m_taskQueue.empty() || !m_running; });

            if (!m_running) break;

            if (!m_taskQueue.empty())
            {
                task = std::move(m_taskQueue.front());
                m_taskQueue.pop();
            }
        }

        if (task)
        {
            task();
        }
    }
}

void ResourceSubsystem::PreloadResources(const std::vector<std::string>& paths)
{
    for (const auto& path : paths)
    {
        // 將載入任務加入佇列
        {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            m_taskQueue.push([this, path]() {
                LoadResourceInternal(path);
            });
        }
        m_condition.notify_one();
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
ResourceSubsystem* ResourceSubsystem::s_instance = nullptr;
Renderer* ResourceSubsystem::s_renderer = nullptr;

void ResourceSubsystem::Initialize(Renderer* renderer, sResourceSubsystemConfig const& config)
{
    if (s_instance)
    {
        DebuggerPrintf("Warning: ResourceSubsystem already initialized. Ignoring second initialization.\n");
        return;
    }

    s_renderer = renderer;
    s_instance = new ResourceSubsystem(config);
    s_instance->Startup();

    // Register standard loaders with device access
    if (s_renderer)
    {
        ID3D11Device* device = s_renderer->m_device;
        if (device)
        {
            auto textureLoader = std::make_unique<TextureLoader>(device);
            s_instance->RegisterLoader(std::move(textureLoader));
            DebuggerPrintf("Info: ResourceSubsystem initialized with TextureLoader.\n");

            auto fontLoader = std::make_unique<FontLoader>(s_renderer);
            s_instance->RegisterLoader(std::move(fontLoader));
            DebuggerPrintf("Info: ResourceSubsystem initialized with FontLoader.\n");

            auto shaderLoader = std::make_unique<ShaderLoader>(device);
            s_instance->RegisterLoader(std::move(shaderLoader));
            DebuggerPrintf("Info: ResourceSubsystem initialized with ShaderLoader.\n");
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
}

void ResourceSubsystem::GlobalShutdown()
{
    if (s_instance)
    {
        s_instance->Shutdown();
        delete s_instance;
        s_instance = nullptr;
        s_renderer = nullptr;
        DebuggerPrintf("Info: ResourceSubsystem shut down.\n");
    }
}

Texture* ResourceSubsystem::CreateOrGetTextureFromFile(String const& path)
{
    if (!s_instance || !s_renderer)
    {
        DebuggerPrintf("Warning: ResourceSubsystem not initialized, falling back to Renderer.\n");
        return s_renderer ? s_renderer->CreateOrGetTextureFromFile(path.c_str()) : nullptr;
    }

    try
    {
        // Load texture through ResourceSubsystem
        auto textureHandle = s_instance->LoadResource<TextureResource>(path);

        if (textureHandle.IsValid() && textureHandle.Get())
        {
            TextureResource* textureResource = textureHandle.Get();
            Texture* rendererTexture = textureResource->GetRendererTexture();

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
    if (!s_instance || !s_renderer)
    {
        DebuggerPrintf("Warning: ResourceSubsystem not initialized, falling back to Renderer.\n");
        return s_renderer ? s_renderer->CreateOrGetBitmapFontFromFile(path.c_str()) : nullptr;
    }

    try
    {
        // Load font through ResourceSubsystem
        auto fontHandle = s_instance->LoadResource<FontResource>(path);

        if (fontHandle.IsValid() && fontHandle.Get())
        {
            FontResource* fontResource = fontHandle.Get();
            BitmapFont* rendererBitmapFont = fontResource->GetRendererBitmapFont();

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
    if (!s_instance || !s_renderer)
    {
        DebuggerPrintf("Warning: ResourceSubsystem not initialized, falling back to Renderer.\n");
        return s_renderer ? s_renderer->CreateOrGetShaderFromFile(path.c_str(), vertexType) : nullptr;
    }

    try
    {
        // Check cache first with vertex type in key
        String cacheKey = path + "_" + std::to_string(static_cast<int>(vertexType));
        if (std::shared_ptr<IResource> const cached = s_instance->m_cache.Get(cacheKey))
        {
            if (ShaderResource* shaderResource = static_cast<ShaderResource*>(cached.get()))
            {
                return shaderResource->GetRendererShader();
            }
        }

        // Find ShaderLoader and load with vertex type
        for (std::unique_ptr<IResourceLoader> const& loader : s_instance->m_loaders)
        {
            if (ShaderLoader* shaderLoader = dynamic_cast<ShaderLoader*>(loader.get()))
            {
                if (shaderLoader->CanLoad(s_instance->GetFileExtension(path)))
                {
                    auto shaderResource = shaderLoader->LoadShader(path, vertexType);
                    if (shaderResource)
                    {
                        s_instance->m_cache.Add(cacheKey, shaderResource);
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
