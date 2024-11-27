//----------------------------------------------------------------------------------------------------
// HeatMaps.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include <vector>

#include "Rgba8.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Math/IntVec2.hpp"

//----------------------------------------------------------------------------------------------------
struct IntVec2;

//----------------------------------------------------------------------------------------------------
class TileHeatMap
{
public:
    TileHeatMap(IntVec2 const& dimensions, float initialValue);

    int        GetTileNums() const { return m_dimensions.x * m_dimensions.y; }
    int        GetTileIndex(IntVec2 const& tileCoords) const;
    int        GetTileIndex(int tileX, int tileY) const;
    float      GetValueAtCoords(int tileX, int tileY) const;
    float      GetValueAtCoords(IntVec2 tileCoords) const;
    void       SetValueAtIndex(int tileIndex, float value) const;
    FloatRange GetRangeOfValuesExcludingSpecial(float specialValueToIgnore) const;

    bool IsOutOfBounds(int tileIndex) const;
    bool IsOutOfBounds(int tileX, int tileY) const;
    bool IsOutOfBounds(IntVec2 const& tileCoords) const;

    void SetAllValues(int value) const;
    void SetValueAtCoords(IntVec2 const& tileCoords, float x) const;
    void AddVertsForDebugDraw(VertexList&       verts,
                              AABB2 const&      totalBounds,
                              FloatRange const& valueRange   = FloatRange(0.f, 1.f),
                              Rgba8 const&      lowColor     = Rgba8(0, 0, 0, 100),
                              Rgba8 const&      highColor    = Rgba8(255, 255, 255, 100),
                              float             specialValue = 999.f,
                              Rgba8 const&      specialColor = Rgba8(255, 0, 0)) const;

    IntVec2 m_dimensions   = IntVec2::ZERO;
    float*  m_values       = nullptr;
    float   m_lowestValue  = FLT_MAX;
    float   m_highestValue = -FLT_MAX;
};
