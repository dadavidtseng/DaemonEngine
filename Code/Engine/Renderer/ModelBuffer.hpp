//----------------------------------------------------------------------------------------------------
// ModelBuffer.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once

struct ID3D11Buffer;
struct ID3D11Device;

//----------------------------------------------------------------------------------------------------
class ModelBuffer
{
    friend class Renderer;

public:
    ModelBuffer(ID3D11Device* device, size_t size);
    ModelBuffer(ModelBuffer const& copy) = delete;
    virtual ~ModelBuffer();

    void Create();
    void Resize(size_t size);
    size_t GetSize() const;

private:
    ID3D11Buffer* m_buffer = nullptr;
    ID3D11Device* m_device = nullptr;
    size_t        m_size   = 0;
};
