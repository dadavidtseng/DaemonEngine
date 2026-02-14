//----------------------------------------------------------------------------------------------------
// BufferWriter.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include "Engine/Core/EngineCommon.hpp"

#include <cstdint>
#include <string>
#include <vector>

//----------------------------------------------------------------------------------------------------
struct Vec2;
struct Vec3;
struct IntVec2;
struct Rgba8;
struct AABB2;
struct Plane2;
struct Vertex_PCU;

//----------------------------------------------------------------------------------------------------
class BufferWriter
{
public:
    explicit BufferWriter(std::vector<uint8_t>& buffer);

    // Primitives (10 types)
    void AppendByte(uint8_t value);
    void AppendChar(char value);
    void AppendUshort(unsigned short value);
    void AppendShort(short value);
    void AppendUint32(unsigned int value);
    void AppendInt32(int value);
    void AppendUint64(uint64_t value);
    void AppendInt64(int64_t value);
    void AppendFloat(float value);
    void AppendDouble(double value);

    // Strings
    void AppendZeroTerminatedString(std::string const& str);
    void AppendLengthPrecededString(std::string const& str);

    // Engine semi-primitives
    void AppendVec2(Vec2 const& v);
    void AppendVec3(Vec3 const& v);
    void AppendIntVec2(IntVec2 const& v);
    void AppendRgba8(Rgba8 const& color);
    void AppendAABB2(AABB2 const& box);
    void AppendPlane2(Plane2 const& plane);
    void AppendVertexPCU(Vertex_PCU const& vert);

    // Random-access overwrite
    void OverwriteUint32(size_t position, unsigned int value);

    // Endianness control
    void        SetEndianMode(eEndianMode mode);
    eEndianMode GetEndianMode() const;
    size_t      GetTotalSize() const;

protected:
    void ReverseBytes(void* data, size_t size);
    bool NeedsSwap() const;

    std::vector<uint8_t>& m_buffer;
    eEndianMode           m_endianMode;
    eEndianMode           m_localEndianMode;
};
