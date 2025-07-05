// ============================================
// ResourceHandle.hpp - 智慧資源句柄
// ============================================
#pragma once
#include <memory>

template<typename T>
class ResourceHandle
{
public:
    ResourceHandle() = default;

    explicit ResourceHandle(std::shared_ptr<T> resource)
        : m_resource(resource)
    {
        if (m_resource)
        {
            m_resource->AddRef();
        }
    }

    ~ResourceHandle()
    {
        Release();
    }

    // 複製建構與賦值
    ResourceHandle(const ResourceHandle& other)
        : m_resource(other.m_resource)
    {
        if (m_resource)
        {
            m_resource->AddRef();
        }
    }

    ResourceHandle& operator=(const ResourceHandle& other)
    {
        if (this != &other)
        {
            Release();
            m_resource = other.m_resource;
            if (m_resource)
            {
                m_resource->AddRef();
            }
        }
        return *this;
    }

    // 移動建構與賦值
    ResourceHandle(ResourceHandle&& other) noexcept
        : m_resource(std::move(other.m_resource))
    {
        other.m_resource.reset();
    }

    ResourceHandle& operator=(ResourceHandle&& other) noexcept
    {
        if (this != &other)
        {
            Release();
            m_resource = std::move(other.m_resource);
            other.m_resource.reset();
        }
        return *this;
    }

    // 資源存取
    T* Get() const { return m_resource.get(); }
    T* operator->() const { return m_resource.get(); }
    T& operator*() const { return *m_resource; }
    bool IsValid() const { return m_resource != nullptr; }

    void Release()
    {
        if (m_resource)
        {
            m_resource->Release();
            m_resource.reset();
        }
    }

private:
    std::shared_ptr<T> m_resource;
};