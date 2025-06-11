//----------------------------------------------------------------------------------------------------
// IndexBuffer.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once

struct ID3D11Buffer;
struct ID3D11Device;

//----------------------------------------------------------------------------------------------------
class IndexBuffer
{
    friend class Renderer;
    friend class RendererEx;

public:
    IndexBuffer(ID3D11Device* device, unsigned int size, unsigned int stride);
    IndexBuffer(IndexBuffer const& copy) = delete;
    virtual ~IndexBuffer();

    void Create();
    void Resize(unsigned int size);

    unsigned int GetSize() const;
    unsigned int GetStride() const;

private:
    ID3D11Buffer* m_buffer = nullptr;
    ID3D11Device* m_device = nullptr;
    unsigned int  m_size   = 0;
    unsigned int  m_stride = 0;
};
