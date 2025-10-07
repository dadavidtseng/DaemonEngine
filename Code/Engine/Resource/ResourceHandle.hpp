// ============================================
// ResourceHandle.hpp - Smart Resource Handle
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
        // No manual ref counting needed - shared_ptr handles it automatically
    }

    ~ResourceHandle() = default;

    // Copy constructor and assignment (shared_ptr handles ref counting)
    ResourceHandle(const ResourceHandle& other) = default;
    ResourceHandle& operator=(const ResourceHandle& other) = default;

    // Move constructor and assignment (shared_ptr handles transfer)
    ResourceHandle(ResourceHandle&& other) noexcept = default;
    ResourceHandle& operator=(ResourceHandle&& other) noexcept = default;

    // Resource access
    T* Get() const { return m_resource.get(); }
    T* operator->() const { return m_resource.get(); }
    T& operator*() const { return *m_resource; }
    bool IsValid() const { return m_resource != nullptr; }

    void Release()
    {
        // Simply reset the shared_ptr - automatic ref count decrement
        m_resource.reset();
    }

private:
    std::shared_ptr<T> m_resource;
};