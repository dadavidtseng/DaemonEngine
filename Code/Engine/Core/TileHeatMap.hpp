//-----------------------------------------------------------------------------------------------
// Time.hpp
//
#pragma once
#include <vector>

#include "Vertex_PCU.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Math/IntVec2.hpp"

struct IntVec2;
class TileHeatMap
{
public:
    // TileHeatMap(IntVec2 const& dimensions, float initialValue);
    // ~TileHeatMap()=default;
    // // void SetValueAtIndex(int tileIndex, float value);
    // // float GetValueAtCoords( int tileX, int tileY)const;
    // FloatRange GetRangeOfValuesExcludingSpecial(float specialValueToIgnore)const;
    // void       SetAllValues(float value);
    // bool       IsOutOfBounds(IntVec2 const & tileCoords);
    // void       SetValueAtCoords(const IntVec2& tileCoords, float x);
    // float      GetValueAtCoords(int tileX, int tileY);
    // void SetValueAtCoordsIfSmallerThanCurrent(int tileX, int tileY, float value);
    // void AddVertsForDebugDraw(std::vector<Vertex_PCU>& verts, AABB2 totalBounds, FloatRange valueRange, Rgba8 lowColor, Rgba8 highColor, float specialValue, Rgba8 specialColor);

    IntVec2 m_dimensions =IntVec2::ZERO;
    float* m_values = nullptr;
    float m_lowestValue = FLT_MAX;
    float m_highestValue = FLT_MAX;
};
