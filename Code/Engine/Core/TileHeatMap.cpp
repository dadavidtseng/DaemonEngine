//-----------------------------------------------------------------------------------------------
#include "Engine/Core/TileHeatMap.hpp"

#include "ErrorWarningAssert.hpp"
#include "Rgba8.hpp"
#include "VertexUtils.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/MathUtils.hpp"

// TileHeatMap::TileHeatMap(IntVec2 const& dimensions, float initialValue)
// {
//     int numTiles = dimensions.x * dimensions.y;
//     m_values     = new float[numTiles];
//
//     for (int i = 0; i < numTiles; i++)
//     {
//         m_values[i] = initialValue;
//     }
// }
// FloatRange TileHeatMap::GetRangeOfValuesExcludingSpecial(float specialValueToIgnore) const
// {
//     FloatRange rangeOfNonSpecialValues(FLT_MAX, -FLT_MAX);
//     int        numTiles = GetNumTiles();
//     for (int i = 0; i < numTiles; i++)
//     {
//         float value = m_values[i];
//         if (value != specialValueToIgnore)
//             rangeOfNonSpecialValues.StretchToIncludeValue(m_values[i]);
//     }
//     return rangeOfNonSpecialValues;
// }
// void TileHeatMap::SetAllValues(float value)
// {
//     int numTiles = m_dimensions.x * m_dimensions.y;
//     for (int i = 0; i < numTiles; i++)
//     {
//         m_values[i] = value;
//     }
//     
// }
// bool TileHeatMap::IsOutOfBounds(const IntVec2& tileCoords)
// {
//     return
//     tileCoords.x >= 0 &&
// }
// void TileHeatMap::SetValueAtCoords(const IntVec2& tileCoords, float x)
// {
//     
//     if (IsOutOfBounds(tileCoords))
//     {
//         return;
//     }
//
//     int tileIndex = tiledCoords.x;
// }
// void TileHeatMap::SetValueAtCoordsIfSmallerThanCurrent(int tileX, int tileY, float value)
// {
// int tileIndex = tileX * m_dimensions.y + tileY;
//     float currentvalue = m_values[tileIndex];
//     if (value <= currentvalue)
//     {
//         currentvalue = value;
//     }
//     
// }
// void TileHeatMap::SetValueAtIndex(int tileIndex, float value)
// {
//     // GUARANTEE_OR_DIE(tileIndex>=0&&tileIndex<GetNumTiles());
//     // m_values[tileIndex] = value;
// }

// void TileHeatMap::AddVertsForDebugDraw(std::vector<Vertex_PCU>& verts,
//                                        AABB2                    totalBounds,
//                                        FloatRange               valueRange,
//                                        Rgba8                    lowColor,
//                                        Rgba8                    highColor,
//                                        float                    specialValue,
//                                        Rgba8                    specialColor)
// {
//     // Do some math?
//
//     for (int tileY = 0; tileY < m_dimensions.y; tileY++)
//     {
//         for (int tileX = 0; tileX < m_dimensions.x; tileX++)
//         {
//             int   tileIndex           = tileX + (tileY * m_dimensions.x);
//             float value               = m_values[tileIndex];
//             float fractionWithinRange = GetFractionWithinRange(value, valueRange.min, valueRange.max);
//             Rgba8 color               = Interpolate(lowColor, highColor, fractionWithinRange);
//
//             if (value == specialValue)
//             {
//                 color = specialColor;
//             }

// calculate physical quad bounds (vertex min/max corners)
// float outMinX = RangeMap(static_cast<float>(tileX), 0.f, (float) m_dimensions.x, totalBounds.m_mins.x, totalBounds.m_maxs.x);
// float outMinY = RangeMap(static_cast<float>(tileY), 0.f, (float) m_dimensions.y, totalBounds.m_mins.y, totalBounds.m_maxs.y);
// float outMaxX = RangeMap(static_cast<float>(tileX+1), 0.f, (float) m_dimensions.x, totalBounds.m_mins.x, totalBounds.m_maxs.x);
// float outMaxY = RangeMap(static_cast<float>(tileY+1), 0.f, (float) m_dimensions.y, totalBounds.m_mins.y, totalBounds.m_maxs.y);
// AABB2 tileBounds(outMinX, outMinY, outMaxX, outMaxY);

//             AddVertsForAABB2D(verts, tileBounds, color);
//             
//         }
//     }
//
// }
