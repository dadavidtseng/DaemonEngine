//----------------------------------------------------------------------------------------------------
// IndexBuffer.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Renderer/IndexBuffer.hpp"

#include <d3d11.h>

#include "Engine/Core/ErrorWarningAssert.hpp"

//----------------------------------------------------------------------------------------------------
// Static leak tracking counters
int IndexBuffer::s_totalCreated = 0;
int IndexBuffer::s_totalDeleted = 0;

//----------------------------------------------------------------------------------------------------
IndexBuffer::IndexBuffer(ID3D11Device*      device,
                         unsigned int const size,
                         unsigned int const stride)
    : m_device(device),
      m_size(size),
      m_stride(stride)
{
    Create();
    s_totalCreated++;
}

//----------------------------------------------------------------------------------------------------
IndexBuffer::~IndexBuffer()
{
    if (m_buffer)
    {
        m_buffer->Release();
        m_buffer = nullptr;
        s_totalDeleted++;
    }
}

//----------------------------------------------------------------------------------------------------'
void IndexBuffer::Create()
{
    // Create index buffer
    // Create a local D3D11_BUFFER_DESC variable.
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.Usage             = D3D11_USAGE_DYNAMIC;
    bufferDesc.ByteWidth         = m_size;
    bufferDesc.BindFlags         = D3D11_BIND_INDEX_BUFFER;
    bufferDesc.CPUAccessFlags    = D3D11_CPU_ACCESS_WRITE;

    HRESULT const hr = m_device->CreateBuffer(&bufferDesc, nullptr, &m_buffer);

    if (FAILED(hr) == true)
    {
        ERROR_AND_DIE("Failed to create vertex buffer.")
    }
}

//----------------------------------------------------------------------------------------------------
void IndexBuffer::Resize(unsigned int const size)
{
    if (m_buffer != nullptr)
    {
        m_buffer->Release();
        m_buffer = nullptr;
    }

    m_size = size;
    Create();
}

//----------------------------------------------------------------------------------------------------
unsigned int IndexBuffer::GetSize() const
{
    return m_size;
}

//----------------------------------------------------------------------------------------------------
unsigned int IndexBuffer::GetStride() const
{
    return m_stride;
}

//----------------------------------------------------------------------------------------------------
void IndexBuffer::PrintLeakReport()
{
    int stillAlive = s_totalCreated - s_totalDeleted;

    DebuggerPrintf("========================================\n");
    DebuggerPrintf("[IndexBuffer] LEAK REPORT:\n");
    DebuggerPrintf("[IndexBuffer]   Total Created: %d\n", s_totalCreated);
    DebuggerPrintf("[IndexBuffer]   Total Deleted: %d\n", s_totalDeleted);
    DebuggerPrintf("[IndexBuffer]   Still Alive:   %d\n", stillAlive);

    if (stillAlive == 0)
    {
        DebuggerPrintf("[IndexBuffer]   No leaks detected.\n");
    }
    else
    {
        DebuggerPrintf("[IndexBuffer]   WARNING: %d buffer(s) leaked!\n", stillAlive);
    }

    DebuggerPrintf("========================================\n");
}
