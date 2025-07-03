//----------------------------------------------------------------------------------------------------
// IResource.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Resource/IResource.hpp"


IResource::IResource(String const& path)
    : m_path(path),
      m_state(ResourceState::Unloaded),
      m_refCount(0)
{
}

String const& IResource::GetPath() const
{
    return m_path;
}

ResourceState IResource::GetState() const
{
    return m_state;
}

void IResource::AddRef()
{
    ++m_refCount;
}

void IResource::Release()
{
    if (--m_refCount == 0)
    {
        Unload();
    }
}

int IResource::GetRefCount() const
{
    return m_refCount.load();
}
