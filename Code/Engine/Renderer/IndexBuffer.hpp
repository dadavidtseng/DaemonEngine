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
    IndexBuffer(ID3D11Device* device, unsigned int size, unsigned int stride);
    IndexBuffer(IndexBuffer const& copy) = delete;
    virtual ~IndexBuffer();

    void Create();
    void Resize(unsigned int size);

    unsigned int GetSize() const;
    unsigned int GetStride() const;

    // Leak tracking
    static void PrintLeakReport();

private:
    /// A buffer interface accesses a buffer resource, which is unstructured memory.
    /// Buffers typically store vertex or index data.
    ID3D11Buffer* m_buffer = nullptr;
    /// The device interface represents a virtual adapter; it is used to create resources.
    ID3D11Device* m_device = nullptr;
    unsigned int  m_size   = 0;
    unsigned int  m_stride = 0;

    // Leak tracking counters
    static int s_totalCreated;
    static int s_totalDeleted;
};
