

#pragma once
#include "Engine/Resource/ResourceHandle.hpp"
#include "Engine/Resource/ResourceCache.hpp"
#include "Engine/Resource/IResourceLoader.hpp"
#include <vector>
#include <memory>
#include <thread>
#include <queue>
#include <future>
#include <functional>
#include <condition_variable>

class ResourceSubsystem
{
public:
    static ResourceSubsystem& GetInstance()
    {
        static ResourceSubsystem instance;
        return instance;
    }

    // 初始化與關閉
    void Initialize(size_t threadCount = 4);
    void Shutdown();

    // 註冊載入器
    void RegisterLoader(std::unique_ptr<IResourceLoader> loader);

    // 同步載入資源
    template <typename T>
    ResourceHandle<T> LoadResource(const std::string& path)
    {
        // 檢查快取
        auto cached = m_cache.Get(path);
        if (cached)
        {
            return ResourceHandle<T>(std::static_pointer_cast<T>(cached));
        }

        // 載入新資源
        auto resource = LoadResourceInternal(path);
        if (resource)
        {
            m_cache.Add(path, resource);
            return ResourceHandle<T>(std::static_pointer_cast<T>(resource));
        }

        return ResourceHandle<T>();
    }

    // 異步載入資源
    template <typename T>
    std::future<ResourceHandle<T>> LoadResourceAsync(const std::string& path)
    {
        return std::async(std::launch::async, [this, path]() {
            return LoadResource<T>(path);
        });
    }

    // 預載入資源列表
    void PreloadResources(const std::vector<std::string>& paths);

    // 卸載未使用的資源
    void UnloadUnusedResources();

    // 取得記憶體使用情況
    size_t GetMemoryUsage() const { return m_cache.GetMemoryUsage(); }
    size_t GetResourceCount() const { return m_cache.GetSize(); }

    // 設定記憶體限制
    void SetMemoryLimit(size_t bytes) { m_memoryLimit = bytes; }

private:
    ResourceSubsystem() = default;
    ~ResourceSubsystem() { Shutdown(); }

    // 禁止複製
    ResourceSubsystem(const ResourceSubsystem&)            = delete;
    ResourceSubsystem& operator=(const ResourceSubsystem&) = delete;

    // 內部載入方法
    std::shared_ptr<IResource> LoadResourceInternal(const std::string& path);
    std::string                GetFileExtension(const std::string& path) const;

    // 工作執行緒
    void WorkerThread();

private:
    ResourceCache                                 m_cache;
    std::vector<std::unique_ptr<IResourceLoader>> m_loaders;

    // 多執行緒支援
    std::vector<std::thread>          m_workerThreads;
    std::queue<std::function<void()>> m_taskQueue;
    std::mutex                        m_queueMutex;
    std::condition_variable           m_condition;
    bool                              m_running = false;

    // 記憶體管理
    size_t m_memoryLimit = 0;
};
