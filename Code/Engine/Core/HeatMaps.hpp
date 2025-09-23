//----------------------------------------------------------------------------------------------------
// HeatMaps.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include <vector>

#include "Engine/Core/Rgba8.hpp"
#include "Engine/Renderer/VertexUtils.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Math/IntVec2.hpp"

//----------------------------------------------------------------------------------------------------
class TileHeatMap
{
public:
    TileHeatMap(IntVec2 const& dimensions, float initialValue);

    int        GetTileNums() const { return m_dimensions.x * m_dimensions.y; }
    int        GetTileIndex(int tileX, int tileY) const;
    int        GetTileIndex(IntVec2 const& tileCoords) const;
    float      GetValueAtCoords(int tileX, int tileY) const;
    float      GetValueAtCoords(IntVec2 const& tileCoords) const;
    FloatRange GetRangeOfValuesExcludingSpecial(float specialValueToIgnore) const;

    bool IsOutOfBounds(int tileIndex) const;
    bool IsOutOfBounds(int tileX, int tileY) const;
    bool IsOutOfBounds(IntVec2 const& tileCoords) const;

    void SetValueAtAllTiles(float value) const;
    void SetValueAtIndex(int tileIndex, float value) const;
    void SetValueAtCoords(IntVec2 const& tileCoords, float value) const;
    void AddVertsForDebugDraw(VertexList_PCU& verts,
                              AABB2 const&    totalBounds,
                              Rgba8 const&    lowColor     = Rgba8::BLACK,
                              Rgba8 const&    highColor    = Rgba8::WHITE,
                              float           specialValue = 999.f,
                              Rgba8 const&    specialColor = Rgba8::RED) const;

    // private:
    IntVec2 m_dimensions   = IntVec2::ZERO;
    float*  m_values       = nullptr;
    float   m_lowestValue  = FLT_MAX;
    float   m_highestValue = -FLT_MAX;
};
