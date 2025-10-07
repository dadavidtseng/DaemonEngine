//----------------------------------------------------------------------------------------------------
// IResource.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Resource/IResource.hpp"

//----------------------------------------------------------------------------------------------------
IResource::IResource(String const&       path,
                     eResourceType const type)
    : m_path(path),
      m_type(type)
{
    // 生成唯一 ID
    static std::atomic<uint32_t> s_nextId{0};
    m_id = s_nextId++;
}

uint32_t IResource::GetId() const
{
    return m_id;
}

const std::string& IResource::GetPath() const
{
    return m_path;
}

eResourceType IResource::GetType() const
{
    return m_type;
}

eResourceState IResource::GetState() const
{
    return m_state.load();
}

size_t IResource::GetMemorySize() const
{
    return m_memorySize;
}
