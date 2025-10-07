//----------------------------------------------------------------------------------------------------
// IResource.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
//----------------------------------------------------------------------------------------------------
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Resource/ResourceCommon.hpp"

//----------------------------------------------------------------------------------------------------
class IResource
{
public:
    IResource(String const& path, eResourceType type);
    virtual ~IResource() = default;

    virtual bool   Load() = 0;
    virtual void   Unload() = 0;
    virtual size_t CalculateMemorySize() const = 0;

    // 通用屬性存取
    uint32_t              GetId() const;
    const std::string&    GetPath() const;
    virtual eResourceType GetType() const;
    eResourceState        GetState() const;
    virtual size_t        GetMemorySize() const;

protected:
    friend class ResourceSubsystem;

    uint32_t                    m_id;
    std::string                 m_path;
    eResourceType               m_type;
    std::atomic<eResourceState> m_state      = eResourceState::Unloaded;
    size_t                      m_memorySize = 0;
};
