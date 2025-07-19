//----------------------------------------------------------------------------------------------------
// ResourceSubsystem.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Resource/ResourceSubsystem.hpp"
#include <filesystem>
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Resource/ResourceLoader/ObjModelLoader.hpp"

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
    std::string extension = GetFileExtension(path);

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
