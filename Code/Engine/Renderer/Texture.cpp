//----------------------------------------------------------------------------------------------------
// Texture.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Renderer/Texture.hpp"
//----------------------------------------------------------------------------------------------------
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Renderer/RenderCommon.hpp"

//----------------------------------------------------------------------------------------------------
// Static member initialization
int Texture::s_totalCreated = 0;
int Texture::s_totalDeleted = 0;

//----------------------------------------------------------------------------------------------------
Texture::Texture()
{
    s_totalCreated++;
    DebuggerPrintf("[Texture] Constructor: Created texture #%d (this=%p), Alive=%d\n",
                   s_totalCreated, this, GetAliveCount());
}

//----------------------------------------------------------------------------------------------------
Texture::~Texture()
{
    s_totalDeleted++;
    // Don't call DebuggerPrintf during destruction - logging system may be shut down
    DX_SAFE_RELEASE(m_texture)
    DX_SAFE_RELEASE(m_shaderResourceView)
    DX_SAFE_RELEASE(m_renderTargetView)
}

//----------------------------------------------------------------------------------------------------
int Texture::GetAliveCount()
{
    return s_totalCreated - s_totalDeleted;
}

//----------------------------------------------------------------------------------------------------
int Texture::GetTotalCreated()
{
    return s_totalCreated;
}

//----------------------------------------------------------------------------------------------------
int Texture::GetTotalDeleted()
{
    return s_totalDeleted;
}

//----------------------------------------------------------------------------------------------------
void Texture::ReportLeakStatus()
{
    int alive = GetAliveCount();
    DebuggerPrintf("========================================\n");
    DebuggerPrintf("[Texture] LEAK REPORT:\n");
    DebuggerPrintf("[Texture]   Total Created: %d\n", s_totalCreated);
    DebuggerPrintf("[Texture]   Total Deleted: %d\n", s_totalDeleted);
    DebuggerPrintf("[Texture]   Still Alive:   %d\n", alive);
    if (alive > 0)
    {
        DebuggerPrintf("[Texture]   *** LEAK DETECTED: %d textures not deleted! ***\n", alive);
    }
    else
    {
        DebuggerPrintf("[Texture]   No leaks detected.\n");
    }
    DebuggerPrintf("========================================\n");
}

//----------------------------------------------------------------------------------------------------
IntVec2 Texture::GetDimensions() const
{
    return m_dimensions;
}
