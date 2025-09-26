//----------------------------------------------------------------------------------------------------
// IResource.hpp
//----------------------------------------------------------------------------------------------------

#pragma once
#include "Engine/Resource/ResourceCommon.hpp"
#include <string>
#include <atomic>

#include "Engine/Core/StringUtils.hpp"

class IResource
{
public:
    IResource(String const& path, ResourceType type);

    virtual ~IResource() = default;

    // 子類必須實作的介面
    virtual bool Load() = 0;
    virtual void Unload() = 0;
    virtual size_t CalculateMemorySize() const = 0;

    // 通用屬性存取
    uint32_t             GetId() const;
    const std::string&   GetPath() const;
    virtual ResourceType GetType() const;
    ResourceState        GetState() const;
    virtual size_t       GetMemorySize() const;

    // 引用計數管理
    void AddRef();
    void Release();
    int  GetRefCount() const;

protected:
    friend class ResourceSubsystem;

    uint32_t m_id;
    std::string m_path;
    ResourceType m_type;
    std::atomic<ResourceState> m_state;
    std::atomic<int> m_refCount;
    size_t m_memorySize;
};
