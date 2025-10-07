 //----------------------------------------------------------------------------------------------------
// ResourceCache.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
//----------------------------------------------------------------------------------------------------
#include <unordered_map>
#include <mutex>
#include <string>

class IResource;

class ResourceCache
{
public:
    using ResourcePtr = std::shared_ptr<IResource>;

    // 添加資源到快取
    void Add(const std::string& path, ResourcePtr resource);

    // 從快取取得資源
    ResourcePtr Get(const std::string& path) const;

    // 檢查資源是否在快取中
    bool Contains(const std::string& path) const;

    // 移除資源
    void Remove(const std::string& path);

    // 清空快取
    void Clear();

    // 取得快取大小
    size_t GetSize() const;

    // 取得總記憶體使用量
    size_t GetMemoryUsage() const;

    // 移除未使用的資源
    void RemoveUnused();

private:
    mutable std::mutex                           m_mutex;
    std::unordered_map<std::string, ResourcePtr> m_resources;
};
