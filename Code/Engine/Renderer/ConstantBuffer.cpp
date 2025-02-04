//----------------------------------------------------------------------------------------------------
// ConstantBuffer.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Renderer/ConstantBuffer.hpp"

#include <d3d11.h>

#include "Engine/Core/ErrorWarningAssert.hpp"

//----------------------------------------------------------------------------------------------------
ConstantBuffer::ConstantBuffer(ID3D11Device* device, size_t const size)
    : m_device(device), m_size(size)
{
    Create();
}

//----------------------------------------------------------------------------------------------------
ConstantBuffer::~ConstantBuffer()
{
    if (m_buffer)
    {
        m_buffer->Release();
        m_buffer = nullptr;
    }
}

//----------------------------------------------------------------------------------------------------
void ConstantBuffer::Create()
{
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.Usage             = D3D11_USAGE_DYNAMIC;
    bufferDesc.ByteWidth         = static_cast<UINT>(m_size);
    bufferDesc.BindFlags         = D3D11_BIND_CONSTANT_BUFFER;
    bufferDesc.CPUAccessFlags    = D3D11_CPU_ACCESS_WRITE;

    HRESULT const hr = m_device->CreateBuffer(&bufferDesc, nullptr, &m_buffer);
    if (!SUCCEEDED(hr))
    {
        ERROR_AND_DIE("Failed to create constant buffer.")
    }
}

//----------------------------------------------------------------------------------------------------
void ConstantBuffer::Resize(size_t const size)
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
size_t ConstantBuffer::GetSize() const
{
    return m_size;
}
