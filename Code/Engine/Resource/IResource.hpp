// ============================================
// IResource.hpp - 資源基礎介面
// ============================================
#pragma once
#include "Engine/Resource/ResourceCommon.hpp"
#include <string>
#include <atomic>

class IResource
{
public:
    IResource(const std::string& path, ResourceType type)
        : m_path(path)
        , m_type(type)
        , m_state(ResourceState::Unloaded)
        , m_refCount(0)
        , m_memorySize(0)
    {
        // 生成唯一 ID
        static std::atomic<uint32_t> s_nextId{0};
        m_id = s_nextId++;
    }

    virtual ~IResource() = default;

    // 子類必須實作的介面
    virtual bool Load() = 0;
    virtual void Unload() = 0;
    virtual size_t CalculateMemorySize() const = 0;

    // 通用屬性存取
    uint32_t GetId() const { return m_id; }
    const std::string& GetPath() const { return m_path; }
    ResourceType GetType() const { return m_type; }
    ResourceState GetState() const { return m_state.load(); }
    size_t GetMemorySize() const { return m_memorySize; }

    // 引用計數管理
    void AddRef() { ++m_refCount; }
    void Release()
    {
        if (--m_refCount == 0)
        {
            m_state = ResourceState::Unloading;
            Unload();
            m_state = ResourceState::Unloaded;
        }
    }
    int GetRefCount() const { return m_refCount.load(); }

protected:
    friend class ResourceSubsystem;

    uint32_t m_id;
    std::string m_path;
    ResourceType m_type;
    std::atomic<ResourceState> m_state;
    std::atomic<int> m_refCount;
    size_t m_memorySize;
};
