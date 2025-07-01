//----------------------------------------------------------------------------------------------------
// VertexBuffer.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include <d3d11.h>

struct ID3D11Buffer;
struct ID3D11Device;

//----------------------------------------------------------------------------------------------------
class VertexBuffer
{
    friend class Renderer;
    friend class RendererEx;

public:
    VertexBuffer(ID3D11Device* device, unsigned int size, unsigned int stride);
    VertexBuffer(VertexBuffer const& copy) = delete;
    virtual ~VertexBuffer();

    void          Create();
    VertexBuffer* CreateStagingCopy(ID3D11DeviceContext* context);
    void          Resize(unsigned int size);

    unsigned int GetSize() const;
    unsigned int GetStride() const;

private:
    /// A buffer interface accesses a buffer resource, which is unstructured memory.
    /// Buffers typically store vertex or index data.
    ID3D11Buffer* m_buffer = nullptr;
    /// The device interface represents a virtual adapter; it is used to create resources.
    ID3D11Device* m_device = nullptr;
    unsigned int  m_size   = 0;
    unsigned int  m_stride = 0;
};
