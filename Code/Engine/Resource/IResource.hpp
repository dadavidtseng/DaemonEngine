// IResource.hpp - 資源基類介面
#pragma once
#include <string>
#include <atomic>

#include "ResourceCommon.hpp"

class IResource
{
public:
    IResource(const std::string& path)
        : m_path(path)
        , m_state(ResourceState::Unloaded)
        , m_refCount(0)
    {}

    virtual ~IResource() = default;

    // 純虛函數
    virtual bool Load() = 0;
    virtual void Unload() = 0;
    virtual ResourceType GetType() const = 0;
    virtual size_t GetMemorySize() const = 0;

    // 通用函數
    const std::string& GetPath() const { return m_path; }
    ResourceState GetState() const { return m_state; }

    void AddRef() { ++m_refCount; }
    void Release()
    {
        if (--m_refCount == 0)
        {
            Unload();
        }
    }

    int GetRefCount() const { return m_refCount.load(); }

protected:
    std::string m_path;
    ResourceState m_state;
    std::atomic<int> m_refCount;
};