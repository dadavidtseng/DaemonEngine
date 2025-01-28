//----------------------------------------------------------------------------------------------------
// VertexBuffer.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Renderer/VertexBuffer.hpp"

#include <d3d11.h>

#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/Vertex_PCU.hpp"

//----------------------------------------------------------------------------------------------------
VertexBuffer::VertexBuffer(ID3D11Device* device, unsigned int const size, unsigned int const stride)
    : m_device(device),
      m_size(size),
      m_stride(stride)
{
    Create();
}

//----------------------------------------------------------------------------------------------------
VertexBuffer::~VertexBuffer()
{
    if (m_buffer)
    {
        m_buffer->Release();
        m_buffer = nullptr;
    }
}

//----------------------------------------------------------------------------------------------------
void VertexBuffer::Create()
{
    // Create vertex buffer
    // Create a local D3D11_BUFFER_DESC variable.
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.ByteWidth = m_size;
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    HRESULT hr = m_device->CreateBuffer(&bufferDesc, nullptr, &m_buffer);
    if (FAILED(hr))
    {
        ERROR_AND_DIE("Failed to create vertex buffer.");
    }
}

//----------------------------------------------------------------------------------------------------
void VertexBuffer::Resize(unsigned int size)
{
    if (m_buffer)
    {
        m_buffer->Release();
        m_buffer = nullptr;
    }
    m_size = size;
    Create();
}

//----------------------------------------------------------------------------------------------------
unsigned int VertexBuffer::GetSize() const
{
    return m_size;
}

//----------------------------------------------------------------------------------------------------
unsigned int VertexBuffer::GetStride() const
{
    return m_stride;
}
