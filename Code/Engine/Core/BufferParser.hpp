//----------------------------------------------------------------------------------------------------
// BufferParser.hpp
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
class BufferParser
{
public:
    BufferParser(uint8_t const* data, size_t size);
    BufferParser(std::vector<uint8_t> const& buffer);

    // Primitives (10 types)
    uint8_t        ParseByte();
    char           ParseChar();
    unsigned short ParseUshort();
    short          ParseShort();
    unsigned int   ParseUint32();
    int            ParseInt32();
    uint64_t       ParseUint64();
    int64_t        ParseInt64();
    float          ParseFloat();
    double         ParseDouble();

    // Strings
    void ParseZeroTerminatedString(std::string& out_str);
    void ParseLengthPrecededString(std::string& out_str);

    // Engine semi-primitives
    Vec2       ParseVec2();
    Vec3       ParseVec3();
    IntVec2    ParseIntVec2();
    Rgba8      ParseRgba8();
    AABB2      ParseAABB2();
    Plane2     ParsePlane2();
    Vertex_PCU ParseVertexPCU();

    // Position control
    size_t GetCurrentPosition() const;
    void   SetCurrentPosition(size_t position);

    // Endianness control
    void        SetEndianMode(eEndianMode mode);
    eEndianMode GetEndianMode() const;

protected:
    void GuardRead(size_t bytesNeeded) const;
    void ReverseBytes(void* data, size_t size);
    bool NeedsSwap() const;

    uint8_t const* m_data;
    size_t         m_size;
    size_t         m_currentPosition;
    eEndianMode    m_endianMode;
    eEndianMode    m_localEndianMode;
};
