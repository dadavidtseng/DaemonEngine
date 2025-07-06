//----------------------------------------------------------------------------------------------------
// IResource.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Resource/Resource/IResource.hpp"

IResource::IResource(String const& path, ResourceType type)
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

uint32_t IResource::GetId() const
{
    return m_id;
}

const std::string& IResource::GetPath() const
{
    return m_path;
}

ResourceType IResource::GetType() const
{
    return m_type;
}

ResourceState IResource::GetState() const
{
    return m_state.load();
}

size_t IResource::GetMemorySize() const
{
    return m_memorySize;
}

void IResource::AddRef()
{
    ++m_refCount;
}

void IResource::Release()
{
    if (--m_refCount == 0)
    {
        m_state = ResourceState::Unloading;
        Unload();
        m_state = ResourceState::Unloaded;
    }
}

int IResource::GetRefCount() const
{
    return m_refCount.load();
}
