// ============================================
// ResourceCache.cpp
// ============================================
#include "Engine/Resource/ResourceCache.hpp"
#include "Engine/Resource/Resource/IResource.hpp"

void ResourceCache::Add(const std::string& path, ResourcePtr resource)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_resources[path] = resource;
}

ResourceCache::ResourcePtr ResourceCache::Get(const std::string& path) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_resources.find(path);
    return (it != m_resources.end()) ? it->second : nullptr;
}

bool ResourceCache::Contains(const std::string& path) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_resources.find(path) != m_resources.end();
}

void ResourceCache::Remove(const std::string& path)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_resources.erase(path);
}

void ResourceCache::Clear()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_resources.clear();
}

size_t ResourceCache::GetSize() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_resources.size();
}

size_t ResourceCache::GetMemoryUsage() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    size_t totalMemory = 0;

    for (const auto& [path, resource] : m_resources)
    {
        if (resource)
        {
            totalMemory += resource->GetMemorySize();
        }
    }

    return totalMemory;
}

void ResourceCache::RemoveUnused()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_resources.begin();
    while (it != m_resources.end())
    {
        if (it->second && it->second->GetRefCount() == 0)
        {
            it = m_resources.erase(it);
        }
        else
        {
            ++it;
        }
    }
}