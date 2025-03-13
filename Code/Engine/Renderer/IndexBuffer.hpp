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

public:
    IndexBuffer(unsigned int size);
    IndexBuffer(IndexBuffer const& copy);
    virtual ~IndexBuffer();

    unsigned int GetSize() const;
    unsigned int GetStride() const;
    unsigned int GetCount() const;

private:
    ID3D11Buffer* m_buffer = nullptr;
    unsigned int  m_size   = 0;
};
