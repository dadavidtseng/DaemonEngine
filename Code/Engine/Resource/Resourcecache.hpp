


#pragma once
#include <unordered_map>
#include <memory>
#include <mutex>

#include "IResource.hpp"

class IResource;

class ResourceCache
{
public:
    template<typename T>
    std::shared_ptr<T> Get(const std::string& path)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_resources.find(path);
        if (it != m_resources.end())
        {
            return std::static_pointer_cast<T>(it->second);
        }

        return nullptr;
    }

    void Add(const std::string& path, std::shared_ptr<IResource> resource)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_resources[path] = resource;
    }

    void Remove(const std::string& path)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_resources.erase(path);
    }

    void Clear()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_resources.clear();
    }

    size_t GetMemoryUsage() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        size_t totalSize = 0;
        for (const auto& pair : m_resources)
        {
            totalSize += pair.second->GetMemorySize();
        }
        return totalSize;
    }

private:
    mutable std::mutex m_mutex;
    std::unordered_map<std::string, std::shared_ptr<IResource>> m_resources;
};
