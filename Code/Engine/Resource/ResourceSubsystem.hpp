//----------------------------------------------------------------------------------------------------
// ResourceSubsystem.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <queue>
#include <thread>
#include <vector>
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Resource/ResourceLoader/IResourceLoader.hpp"
#include "Engine/Resource/ResourceCache.hpp"
#include "Engine/Resource/ResourceHandle.hpp"

struct sResourceSubsystemConfig
{
    int m_threadCount = 0;
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
    template <typename T>
    std::future<ResourceHandle<T>> LoadResourceAsync(String const& path)
    {
        return std::async(std::launch::async, [this, path]() {
            return LoadResource<T>(path);
        });
    }

    // 預載入資源列表
    void PreloadResources( std::vector<std::string> const& paths);

    // 卸載未使用的資源
    void UnloadUnusedResources();

    // 取得記憶體使用情況
    size_t GetMemoryUsage() const;
    size_t GetResourceCount() const;

    // 設定記憶體限制
    void SetMemoryLimit(size_t bytes) { m_memoryLimit = bytes; }

private:
    sResourceSubsystemConfig m_config;
    ~ResourceSubsystem() { Shutdown(); }

    // 禁止複製
    ResourceSubsystem( ResourceSubsystem const&)            = delete;
    ResourceSubsystem& operator=( ResourceSubsystem const&) = delete;

    // 內部載入方法
    std::shared_ptr<IResource> LoadResourceInternal( String const& path);
    String                GetFileExtension( String const& path) const;

    // 工作執行緒
    void WorkerThread();

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
