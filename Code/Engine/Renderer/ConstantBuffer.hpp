//----------------------------------------------------------------------------------------------------
// ConstantBuffer.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once

struct ID3D11Buffer;
struct ID3D11Device;

//----------------------------------------------------------------------------------------------------
class ConstantBuffer
{
    friend class Renderer;
    friend class RendererEx;

public:
    ConstantBuffer(ID3D11Device* device, size_t size);
    ConstantBuffer(ConstantBuffer const& copy) = delete;
    virtual ~ConstantBuffer();

    void Create();
    void Resize(size_t size);
    size_t GetSize() const;

private:
    ID3D11Buffer* m_buffer = nullptr;
    ID3D11Device* m_device = nullptr;
    size_t        m_size   = 0;
};
