// ResourceHandle.hpp - 資源句柄
#pragma once
#include <memory>
#include <atomic>

template<typename T>
class ResourceHandle
{
public:
    ResourceHandle() = default;
    explicit ResourceHandle(std::shared_ptr<T> resource) : m_resource(resource) {}

    T* Get() const { return m_resource.get(); }
    T* operator->() const { return m_resource.get(); }
    T& operator*() const { return *m_resource; }
    bool IsValid() const { return m_resource != nullptr; }

    void Release() { m_resource.reset(); }

private:
    std::shared_ptr<T> m_resource;
};