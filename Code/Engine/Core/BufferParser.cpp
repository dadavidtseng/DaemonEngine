//----------------------------------------------------------------------------------------------------
// BufferParser.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Core/BufferParser.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/Plane2.hpp"
#include "Engine/Renderer/Vertex_PCU.hpp"

//----------------------------------------------------------------------------------------------------
BufferParser::BufferParser(uint8_t const* data, size_t size)
    : m_data(data)
    , m_size(size)
    , m_currentPosition(0)
    , m_endianMode(eEndianMode::NATIVE)
    , m_localEndianMode(GetPlatformLocalEndian())
{
}

//----------------------------------------------------------------------------------------------------
BufferParser::BufferParser(std::vector<uint8_t> const& buffer)
    : m_data(buffer.data())
    , m_size(buffer.size())
    , m_currentPosition(0)
    , m_endianMode(eEndianMode::NATIVE)
    , m_localEndianMode(GetPlatformLocalEndian())
{
}

//----------------------------------------------------------------------------------------------------
uint8_t BufferParser::ParseByte()
{
    GuardRead(sizeof(uint8_t));
    uint8_t value = m_data[m_currentPosition];
    m_currentPosition += sizeof(uint8_t);
    return value;
}

//----------------------------------------------------------------------------------------------------
char BufferParser::ParseChar()
{
    GuardRead(sizeof(char));
    char value = static_cast<char>(m_data[m_currentPosition]);
    m_currentPosition += sizeof(char);
    return value;
}

//----------------------------------------------------------------------------------------------------
unsigned short BufferParser::ParseUshort()
{
    GuardRead(sizeof(unsigned short));
    unsigned short value;
    memcpy(&value, m_data + m_currentPosition, sizeof(value));
    if (NeedsSwap())
    {
        ReverseBytes(&value, sizeof(value));
    }
    m_currentPosition += sizeof(unsigned short);
    return value;
}

//----------------------------------------------------------------------------------------------------
short BufferParser::ParseShort()
{
    GuardRead(sizeof(short));
    short value;
    memcpy(&value, m_data + m_currentPosition, sizeof(value));
    if (NeedsSwap())
    {
        ReverseBytes(&value, sizeof(value));
    }
    m_currentPosition += sizeof(short);
    return value;
}

//----------------------------------------------------------------------------------------------------
unsigned int BufferParser::ParseUint32()
{
    GuardRead(sizeof(unsigned int));
    unsigned int value;
    memcpy(&value, m_data + m_currentPosition, sizeof(value));
    if (NeedsSwap())
    {
        ReverseBytes(&value, sizeof(value));
    }
    m_currentPosition += sizeof(unsigned int);
    return value;
}

//----------------------------------------------------------------------------------------------------
int BufferParser::ParseInt32()
{
    GuardRead(sizeof(int));
    int value;
    memcpy(&value, m_data + m_currentPosition, sizeof(value));
    if (NeedsSwap())
    {
        ReverseBytes(&value, sizeof(value));
    }
    m_currentPosition += sizeof(int);
    return value;
}

//----------------------------------------------------------------------------------------------------
uint64_t BufferParser::ParseUint64()
{
    GuardRead(sizeof(uint64_t));
    uint64_t value;
    memcpy(&value, m_data + m_currentPosition, sizeof(value));
    if (NeedsSwap())
    {
        ReverseBytes(&value, sizeof(value));
    }
    m_currentPosition += sizeof(uint64_t);
    return value;
}

//----------------------------------------------------------------------------------------------------
int64_t BufferParser::ParseInt64()
{
    GuardRead(sizeof(int64_t));
    int64_t value;
    memcpy(&value, m_data + m_currentPosition, sizeof(value));
    if (NeedsSwap())
    {
        ReverseBytes(&value, sizeof(value));
    }
    m_currentPosition += sizeof(int64_t);
    return value;
}

//----------------------------------------------------------------------------------------------------
float BufferParser::ParseFloat()
{
    GuardRead(sizeof(float));
    float value;
    memcpy(&value, m_data + m_currentPosition, sizeof(value));
    if (NeedsSwap())
    {
        ReverseBytes(&value, sizeof(value));
    }
    m_currentPosition += sizeof(float);
    return value;
}

//----------------------------------------------------------------------------------------------------
double BufferParser::ParseDouble()
{
    GuardRead(sizeof(double));
    double value;
    memcpy(&value, m_data + m_currentPosition, sizeof(value));
    if (NeedsSwap())
    {
        ReverseBytes(&value, sizeof(value));
    }
    m_currentPosition += sizeof(double);
    return value;
}

//----------------------------------------------------------------------------------------------------
void BufferParser::ParseZeroTerminatedString(std::string& out_str)
{
    out_str.clear();
    while (m_currentPosition < m_size)
    {
        char c = static_cast<char>(m_data[m_currentPosition]);
        m_currentPosition++;
        if (c == '\0')
        {
            return;
        }
        out_str.push_back(c);
    }
    ERROR_RECOVERABLE("BufferParser::ParseZeroTerminatedString - no null terminator found before end of buffer");
}

//----------------------------------------------------------------------------------------------------
void BufferParser::ParseLengthPrecededString(std::string& out_str)
{
    unsigned int length = ParseUint32();
    GuardRead(static_cast<size_t>(length));
    out_str.clear();
    out_str.reserve(length);
    for (unsigned int i = 0; i < length; ++i)
    {
        out_str.push_back(static_cast<char>(m_data[m_currentPosition + i]));
    }
    m_currentPosition += static_cast<size_t>(length);
}

//----------------------------------------------------------------------------------------------------
Vec2 BufferParser::ParseVec2()
{
    Vec2 value;
    value.x = ParseFloat();
    value.y = ParseFloat();
    return value;
}

//----------------------------------------------------------------------------------------------------
Vec3 BufferParser::ParseVec3()
{
    Vec3 value;
    value.x = ParseFloat();
    value.y = ParseFloat();
    value.z = ParseFloat();
    return value;
}

//----------------------------------------------------------------------------------------------------
IntVec2 BufferParser::ParseIntVec2()
{
    IntVec2 value;
    value.x = ParseInt32();
    value.y = ParseInt32();
    return value;
}

//----------------------------------------------------------------------------------------------------
Rgba8 BufferParser::ParseRgba8()
{
    Rgba8 value;
    value.r = ParseByte();
    value.g = ParseByte();
    value.b = ParseByte();
    value.a = ParseByte();
    return value;
}

//----------------------------------------------------------------------------------------------------
AABB2 BufferParser::ParseAABB2()
{
    AABB2 value;
    value.m_mins = ParseVec2();
    value.m_maxs = ParseVec2();
    return value;
}

//----------------------------------------------------------------------------------------------------
Plane2 BufferParser::ParsePlane2()
{
    Plane2 value;
    value.m_normal = ParseVec2();
    value.m_distanceFromOrigin = ParseFloat();
    return value;
}

//----------------------------------------------------------------------------------------------------
Vertex_PCU BufferParser::ParseVertexPCU()
{
    Vertex_PCU vert;
    vert.m_position = ParseVec3();
    vert.m_color = ParseRgba8();
    vert.m_uvTexCoords = ParseVec2();
    return vert;
}

//----------------------------------------------------------------------------------------------------
size_t BufferParser::GetCurrentPosition() const
{
    return m_currentPosition;
}

//----------------------------------------------------------------------------------------------------
void BufferParser::SetCurrentPosition(size_t position)
{
    m_currentPosition = position;
}

//----------------------------------------------------------------------------------------------------
void BufferParser::SetEndianMode(eEndianMode mode)
{
    m_endianMode = mode;
}

//----------------------------------------------------------------------------------------------------
eEndianMode BufferParser::GetEndianMode() const
{
    return m_endianMode;
}

//----------------------------------------------------------------------------------------------------
void BufferParser::GuardRead(size_t bytesNeeded) const
{
    if (m_currentPosition + bytesNeeded > m_size)
    {
        ERROR_RECOVERABLE(Stringf("BufferParser: read of %zu bytes at position %zu exceeds buffer size %zu",
            bytesNeeded, m_currentPosition, m_size));
    }
}

//----------------------------------------------------------------------------------------------------
void BufferParser::ReverseBytes(void* data, size_t size)
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
bool BufferParser::NeedsSwap() const
{
    if (m_endianMode == eEndianMode::NATIVE)
    {
        return false;
    }
    return m_endianMode != m_localEndianMode;
}
