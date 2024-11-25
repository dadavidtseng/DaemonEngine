//----------------------------------------------------------------------------------------------------
// HeatMaps.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include <vector>

#include "VertexUtils.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Math/IntVec2.hpp"

struct IntVec2;
class TileHeatMap
{
public:
    TileHeatMap(IntVec2 const& dimensions, float initialValue);

    int        GetTileNums() const { return m_dimensions.x * m_dimensions.y; }
    int        GetTileIndex(IntVec2 const& tileCoords) const;
    int        GetTileIndex(int tileX, int tileY) const;
    void       SetValueAtIndex(int tileIndex, float value) const;
    float      GetValueAtCoords(int tileX, int tileY) const;
    FloatRange GetRangeOfValuesExcludingSpecial(float specialValueToIgnore) const;

    bool IsOutOfBounds(int tileIndex) const;
    bool IsOutOfBounds(int tileX, int tileY) const;
    bool IsOutOfBounds(IntVec2 const& tileCoords) const;

    void SetAllValues(float value) const;
    void SetValueAtCoords(const IntVec2& tileCoords, float x) const;
    void SetValueAtCoordsIfSmallerThanCurrent(int tileX, int tileY, float value) const;
    void AddVertsForDebugDraw(VertexList& verts, const AABB2& totalBounds, const FloatRange& valueRange, Rgba8 lowColor, Rgba8 highColor, float specialValue, Rgba8 specialColor);

    IntVec2 m_dimensions   = IntVec2::ZERO;
    float*  m_values       = nullptr;
    float   m_lowestValue  = FLT_MAX;
    float   m_highestValue = -FLT_MAX;
};
