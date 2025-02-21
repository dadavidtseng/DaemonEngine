//----------------------------------------------------------------------------------------------------
// ModelBuffer.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Renderer/ModelBuffer.hpp"

#include <d3d11.h>

#include "Engine/Core/ErrorWarningAssert.hpp"

//----------------------------------------------------------------------------------------------------
ModelBuffer::ModelBuffer(ID3D11Device* device, size_t const size)
    : m_device(device), m_size(size)
{
    Create();
}

//----------------------------------------------------------------------------------------------------
ModelBuffer::~ModelBuffer()
{
    if (m_buffer)
    {
        m_buffer->Release();
        m_buffer = nullptr;
    }
}

//----------------------------------------------------------------------------------------------------
void ModelBuffer::Create()
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
void ModelBuffer::Resize(size_t const size)
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
size_t ModelBuffer::GetSize() const
{
    return m_size;
}
