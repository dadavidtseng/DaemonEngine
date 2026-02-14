//----------------------------------------------------------------------------------------------------
// BufferWriter.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Core/BufferWriter.hpp"

#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/Plane2.hpp"
#include "Engine/Renderer/Vertex_PCU.hpp"

//----------------------------------------------------------------------------------------------------
BufferWriter::BufferWriter(std::vector<uint8_t>& buffer)
    : m_buffer(buffer)
    , m_endianMode(eEndianMode::NATIVE)
    , m_localEndianMode(GetPlatformLocalEndian())
{
}

//----------------------------------------------------------------------------------------------------
void BufferWriter::AppendByte(uint8_t value)
{
    m_buffer.push_back(value);
}

//----------------------------------------------------------------------------------------------------
void BufferWriter::AppendChar(char value)
{
    m_buffer.push_back(static_cast<uint8_t>(value));
}

//----------------------------------------------------------------------------------------------------
void BufferWriter::AppendUshort(unsigned short value)
{
    if (NeedsSwap())
    {
        ReverseBytes(&value, sizeof(value));
    }

    uint8_t const* bytes = reinterpret_cast<uint8_t const*>(&value);
    m_buffer.push_back(bytes[0]);
    m_buffer.push_back(bytes[1]);
}

//----------------------------------------------------------------------------------------------------
void BufferWriter::AppendShort(short value)
{
    if (NeedsSwap())
    {
        ReverseBytes(&value, sizeof(value));
    }

    uint8_t const* bytes = reinterpret_cast<uint8_t const*>(&value);
    m_buffer.push_back(bytes[0]);
    m_buffer.push_back(bytes[1]);
}

//----------------------------------------------------------------------------------------------------
void BufferWriter::AppendUint32(unsigned int value)
{
    if (NeedsSwap())
    {
        ReverseBytes(&value, sizeof(value));
    }

    uint8_t const* bytes = reinterpret_cast<uint8_t const*>(&value);
    m_buffer.push_back(bytes[0]);
    m_buffer.push_back(bytes[1]);
    m_buffer.push_back(bytes[2]);
    m_buffer.push_back(bytes[3]);
}

//----------------------------------------------------------------------------------------------------
void BufferWriter::AppendInt32(int value)
{
    if (NeedsSwap())
    {
        ReverseBytes(&value, sizeof(value));
    }

    uint8_t const* bytes = reinterpret_cast<uint8_t const*>(&value);
    m_buffer.push_back(bytes[0]);
    m_buffer.push_back(bytes[1]);
    m_buffer.push_back(bytes[2]);
    m_buffer.push_back(bytes[3]);
}

//----------------------------------------------------------------------------------------------------
void BufferWriter::AppendUint64(uint64_t value)
{
    if (NeedsSwap())
    {
        ReverseBytes(&value, sizeof(value));
    }

    uint8_t const* bytes = reinterpret_cast<uint8_t const*>(&value);
    for (size_t i = 0; i < sizeof(value); ++i)
    {
        m_buffer.push_back(bytes[i]);
    }
}

//----------------------------------------------------------------------------------------------------
void BufferWriter::AppendInt64(int64_t value)
{
    if (NeedsSwap())
    {
        ReverseBytes(&value, sizeof(value));
    }

    uint8_t const* bytes = reinterpret_cast<uint8_t const*>(&value);
    for (size_t i = 0; i < sizeof(value); ++i)
    {
        m_buffer.push_back(bytes[i]);
    }
}

//----------------------------------------------------------------------------------------------------
void BufferWriter::AppendFloat(float value)
{
    if (NeedsSwap())
    {
        ReverseBytes(&value, sizeof(value));
    }

    uint8_t const* bytes = reinterpret_cast<uint8_t const*>(&value);
    m_buffer.push_back(bytes[0]);
    m_buffer.push_back(bytes[1]);
    m_buffer.push_back(bytes[2]);
    m_buffer.push_back(bytes[3]);
}

//----------------------------------------------------------------------------------------------------
void BufferWriter::AppendDouble(double value)
{
    if (NeedsSwap())
    {
        ReverseBytes(&value, sizeof(value));
    }

    uint8_t const* bytes = reinterpret_cast<uint8_t const*>(&value);
    for (size_t i = 0; i < sizeof(value); ++i)
    {
        m_buffer.push_back(bytes[i]);
    }
}

//----------------------------------------------------------------------------------------------------
void BufferWriter::AppendZeroTerminatedString(std::string const& str)
{
    for (size_t i = 0; i < str.size(); ++i)
    {
        AppendChar(str[i]);
    }
    AppendChar('\0');
}

//----------------------------------------------------------------------------------------------------
void BufferWriter::AppendLengthPrecededString(std::string const& str)
{
    AppendUint32(static_cast<unsigned int>(str.size()));
    for (size_t i = 0; i < str.size(); ++i)
    {
        AppendChar(str[i]);
    }
}

//----------------------------------------------------------------------------------------------------
void BufferWriter::AppendVec2(Vec2 const& v)
{
    AppendFloat(v.x);
    AppendFloat(v.y);
}

//----------------------------------------------------------------------------------------------------
void BufferWriter::AppendVec3(Vec3 const& v)
{
    AppendFloat(v.x);
    AppendFloat(v.y);
    AppendFloat(v.z);
}

//----------------------------------------------------------------------------------------------------
void BufferWriter::AppendIntVec2(IntVec2 const& v)
{
    AppendInt32(v.x);
    AppendInt32(v.y);
}

//----------------------------------------------------------------------------------------------------
void BufferWriter::AppendRgba8(Rgba8 const& color)
{
    AppendByte(color.r);
    AppendByte(color.g);
    AppendByte(color.b);
    AppendByte(color.a);
}

//----------------------------------------------------------------------------------------------------
void BufferWriter::AppendAABB2(AABB2 const& box)
{
    AppendVec2(box.m_mins);
    AppendVec2(box.m_maxs);
}

//----------------------------------------------------------------------------------------------------
void BufferWriter::AppendPlane2(Plane2 const& plane)
{
    AppendVec2(plane.m_normal);
    AppendFloat(plane.m_distanceFromOrigin);
}

//----------------------------------------------------------------------------------------------------
void BufferWriter::AppendVertexPCU(Vertex_PCU const& vert)
{
    AppendVec3(vert.m_position);
    AppendRgba8(vert.m_color);
    AppendVec2(vert.m_uvTexCoords);
}

//----------------------------------------------------------------------------------------------------
void BufferWriter::OverwriteUint32(size_t position, unsigned int value)
{
    if (position + sizeof(unsigned int) > m_buffer.size())
    {
        return;
    }

    if (NeedsSwap())
    {
        ReverseBytes(&value, sizeof(value));
    }

    uint8_t const* bytes = reinterpret_cast<uint8_t const*>(&value);
    m_buffer[position]     = bytes[0];
    m_buffer[position + 1] = bytes[1];
    m_buffer[position + 2] = bytes[2];
    m_buffer[position + 3] = bytes[3];
}

//----------------------------------------------------------------------------------------------------
void BufferWriter::SetEndianMode(eEndianMode mode)
{
    m_endianMode = mode;
}

//----------------------------------------------------------------------------------------------------
eEndianMode BufferWriter::GetEndianMode() const
{
    return m_endianMode;
}

//----------------------------------------------------------------------------------------------------
size_t BufferWriter::GetTotalSize() const
{
    return m_buffer.size();
}

//----------------------------------------------------------------------------------------------------
void BufferWriter::ReverseBytes(void* data, size_t size)
{
    uint8_t* bytes = static_cast<uint8_t*>(data);
    for (size_t i = 0; i < size / 2; ++i)
    {
        uint8_t temp = bytes[i];
        bytes[i] = bytes[size - 1 - i];
        bytes[size - 1 - i] = temp;
    }
}

//----------------------------------------------------------------------------------------------------
bool BufferWriter::NeedsSwap() const
{
    if (m_endianMode == eEndianMode::NATIVE)
    {
        return false;
    }
    return m_endianMode != m_localEndianMode;
}
