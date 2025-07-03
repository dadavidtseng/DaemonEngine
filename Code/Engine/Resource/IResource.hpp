//----------------------------------------------------------------------------------------------------
// IResource.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include <atomic>
#include <string>

#include "Engine/Core/StringUtils.hpp"
#include "Engine/Resource/ResourceCommon.hpp"

//----------------------------------------------------------------------------------------------------
class IResource
{
public:
    explicit IResource(String const& path);
    virtual  ~IResource() = default;

    // 純虛函數
    virtual bool         Load() = 0;
    virtual void         Unload() = 0;
    virtual ResourceType GetType() const = 0;
    virtual size_t       GetMemorySize() const = 0;

    // 通用函數
    String const& GetPath() const;
    ResourceState GetState() const;

    void AddRef();
    void Release();
    int GetRefCount() const;

protected:
    String           m_path;
    ResourceState    m_state;
    std::atomic<int> m_refCount;
};
