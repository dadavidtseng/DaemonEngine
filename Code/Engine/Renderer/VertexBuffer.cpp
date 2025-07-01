//----------------------------------------------------------------------------------------------------
// VertexBuffer.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Renderer/VertexBuffer.hpp"

#include <d3d11.h>

#include "Engine/Core/ErrorWarningAssert.hpp"

//----------------------------------------------------------------------------------------------------
VertexBuffer::VertexBuffer(ID3D11Device*      device,
                           unsigned int const size,
                           unsigned int const stride)
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
    bufferDesc.Usage             = D3D11_USAGE_DYNAMIC;
    bufferDesc.ByteWidth         = m_size;
    bufferDesc.BindFlags         = D3D11_BIND_VERTEX_BUFFER;
    bufferDesc.CPUAccessFlags    = D3D11_CPU_ACCESS_WRITE;

    HRESULT const hr = m_device->CreateBuffer(&bufferDesc, nullptr, &m_buffer);
    if (!SUCCEEDED(hr))
    {
        ERROR_AND_DIE("Failed to create vertex buffer.")
    }
}

VertexBuffer* VertexBuffer::CreateStagingCopy(ID3D11DeviceContext* context)
{
    // 建立 staging buffer 本體
    VertexBuffer* stagingVB = new VertexBuffer(m_device, m_size, m_stride);

    // 建立 staging 用 buffer
    D3D11_BUFFER_DESC desc = {};
    desc.Usage = D3D11_USAGE_STAGING;
    desc.ByteWidth = m_size;
    desc.BindFlags = 0;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    desc.MiscFlags = 0;
    desc.StructureByteStride = m_stride;

    HRESULT hr = m_device->CreateBuffer(&desc, nullptr, &stagingVB->m_buffer);
    if (FAILED(hr)) {
        ERROR_AND_DIE("Failed to create staging vertex buffer");
    }

    // 把原始 buffer 的資料複製到 staging buffer
    context->CopyResource(stagingVB->m_buffer, m_buffer);

    return stagingVB;
}

//----------------------------------------------------------------------------------------------------
void VertexBuffer::Resize(unsigned int const size)
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
unsigned int VertexBuffer::GetSize() const
{
    return m_size;
}

//----------------------------------------------------------------------------------------------------
unsigned int VertexBuffer::GetStride() const
{
    return m_stride;
}
