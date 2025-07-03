//----------------------------------------------------------------------------------------------------
// ResourceSubsystem.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include <filesystem>

#include "ResourceCache.hpp"
#include "IResourceLoader.hpp"
#include <thread>
#include <queue>
#include <future>

#include "ResourceHandle.hpp"

//----------------------------------------------------------------------------------------------------
class ResourceSubsystem
{
public:
    static ResourceSubsystem& GetInstance()
    {
        static ResourceSubsystem instance;
        return instance;
    }

    // 初始化
    void Initialize(size_t numThreads = 4);
    void Shutdown();

    // 註冊載入器
    void RegisterLoader(std::unique_ptr<IResourceLoader> loader);

    // 同步載入
    template <typename T>
    ResourceHandle<T> Load(const std::string& path)
    {
        // 先檢查快取
        auto cached = m_cache.Get<T>(path);
        if (cached)
        {
            cached->AddRef();
            return ResourceHandle<T>(cached);
        }

        // 載入新資源
        auto resource = LoadResource(path);
        if (resource)
        {
            auto typedResource = std::static_pointer_cast<T>(resource);
            typedResource->AddRef();
            m_cache.Add(path, resource);
            return ResourceHandle<T>(typedResource);
        }

        return ResourceHandle<T>();
    }

    // 異步載入
    template <typename T>
    std::future<ResourceHandle<T>> LoadAsync(const std::string& path)
    {
        return std::async(std::launch::async, [this, path]() {
            return Load<T>(path);
        });
    }

    // 批量載入
    template <typename T>
    std::vector<ResourceHandle<T>> LoadBatch(const std::vector<std::string>& paths)
    {
        std::vector<std::future<ResourceHandle<T>>> futures;

        for (const auto& path : paths)
        {
            futures.push_back(LoadAsync<T>(path));
        }

        std::vector<ResourceHandle<T>> results;
        for (auto& future : futures)
        {
            results.push_back(future.get());
        }

        return results;
    }

    // 資源管理
    void   UnloadUnused();
    void   SetMemoryLimit(size_t limitInBytes) { m_memoryLimit = limitInBytes; }
    size_t GetMemoryUsage() const { return m_cache.GetMemoryUsage(); }

    // 熱重載支援
    void EnableHotReload(bool enable) { m_hotReloadEnabled = enable; }
    void CheckForModifiedResources();

private:
    ResourceSubsystem() = default;
    ~ResourceSubsystem() { Shutdown(); }

    ResourceSubsystem(const ResourceSubsystem&)            = delete;
    ResourceSubsystem& operator=(const ResourceSubsystem&) = delete;

    std::shared_ptr<IResource> LoadResource(const std::string& path);
    std::string                GetFileExtension(const std::string& path);

private:
    ResourceCache                                                    m_cache;
    std::vector<std::unique_ptr<IResourceLoader>>                    m_loaders;
    std::vector<std::thread>                                         m_loadingThreads;
    std::queue<std::function<void()>>                                m_loadingQueue;
    std::mutex                                                       m_queueMutex;
    std::condition_variable                                          m_cv;
    bool                                                             m_running          = false;
    size_t                                                           m_memoryLimit      = 0;
    bool                                                             m_hotReloadEnabled = false;
    std::unordered_map<std::string, std::filesystem::file_time_type> m_fileTimestamps;
};
