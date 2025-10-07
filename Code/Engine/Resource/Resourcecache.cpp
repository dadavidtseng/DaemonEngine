//----------------------------------------------------------------------------------------------------
// ResourceCache.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Resource/ResourceCache.hpp"
//----------------------------------------------------------------------------------------------------
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Resource/IResource.hpp"

void ResourceCache::Add(const std::string& path, ResourcePtr resource)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_resources[path] = resource;
}

ResourceCache::ResourcePtr ResourceCache::Get(const std::string& path) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto                        it = m_resources.find(path);
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
    size_t count = m_resources.size();
    DebuggerPrintf("[ResourceCache] Clear: Clearing %zu resources from cache\n", count);

    // Explicitly unload all resources before clearing
    for (auto& pair : m_resources)
    {
        if (pair.second)
        {
            DebuggerPrintf("[ResourceCache] Clear: Unloading resource '%s'\n", pair.first.c_str());
            pair.second->Unload();
        }
    }

    m_resources.clear();
    DebuggerPrintf("[ResourceCache] Clear: Cache cleared\n");
}

size_t ResourceCache::GetSize() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_resources.size();
}

size_t ResourceCache::GetMemoryUsage() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    size_t                      totalMemory = 0;

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
        // Use shared_ptr::use_count() to check references
        // use_count() == 1 means only the cache holds a reference (no external users)
        if (it->second && it->second.use_count() == 1)
        {
            it = m_resources.erase(it);
        }
        else
        {
            ++it;
        }
    }
}
