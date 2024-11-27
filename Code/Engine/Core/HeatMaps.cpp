//----------------------------------------------------------------------------------------------------
// HeatMaps.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Core/HeatMaps.hpp"

#include <cmath>

#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/MathUtils.hpp"

//----------------------------------------------------------------------------------------------------
TileHeatMap::TileHeatMap(IntVec2 const& dimensions, float const initialValue)
    : m_dimensions(dimensions)
{
    int const tileNums = GetTileNums();
    m_values           = new float[tileNums];

    for (int i = 0; i < tileNums; i++)
    {
        m_values[i] = initialValue;
    }
}

//----------------------------------------------------------------------------------------------------
FloatRange TileHeatMap::GetRangeOfValuesExcludingSpecial(float const specialValueToIgnore) const
{
    FloatRange      rangeOfNonSpecialValues(FLT_MAX, -FLT_MAX);
    int const       tileNums = GetTileNums();
    constexpr float epsilon  = 1e-6f;


    for (int i = 0; i < tileNums; i++)
    {
        float const value = m_values[i];

        if (std::fabs(value - specialValueToIgnore) > epsilon)
            rangeOfNonSpecialValues.StretchToIncludeValue(m_values[i]);
    }

    return rangeOfNonSpecialValues;
}


void TileHeatMap::SetAllValues(int const value) const
{
    int const tileNums = GetTileNums();

    for (int i = 0; i < tileNums; i++)
    {
        m_values[i] = value;
    }
}

//----------------------------------------------------------------------------------------------------
bool TileHeatMap::IsOutOfBounds(int const tileIndex) const
{
    int const tileNums = GetTileNums();

    return tileIndex < 0 || tileIndex >= tileNums;
}
bool TileHeatMap::IsOutOfBounds(int const tileX, int const tileY) const
{
    return
        tileX < 0 || tileX >= m_dimensions.x ||
        tileY < 0 || tileY >= m_dimensions.y;
}

//----------------------------------------------------------------------------------------------------
bool TileHeatMap::IsOutOfBounds(IntVec2 const& tileCoords) const
{
    return
        tileCoords.x < 0 || tileCoords.x >= m_dimensions.x ||
        tileCoords.y < 0 || tileCoords.y >= m_dimensions.y;
}

int TileHeatMap::GetTileIndex(IntVec2 const& tileCoords) const
{
    return tileCoords.y * m_dimensions.x + tileCoords.x;
}

int TileHeatMap::GetTileIndex(int const tileX, int const tileY) const
{
    return tileY * m_dimensions.x + tileX;
}

void TileHeatMap::SetValueAtCoords(IntVec2 const& tileCoords, float x) const
{
    if (IsOutOfBounds(tileCoords))
        ERROR_AND_DIE("tileCoords is out of bound")

    int const tileIndex = GetTileIndex(tileCoords);

    m_values[tileIndex] = x;
}

void TileHeatMap::SetValueAtIndex(int const tileIndex, float const value) const
{
    if (IsOutOfBounds(tileIndex))
        ERROR_AND_DIE("tileIndex is out of bounds")

    m_values[tileIndex] = value;
}
float TileHeatMap::GetValueAtCoords(int const tileX, int const tileY) const
{
    IntVec2 const tileCoords(tileX, tileY);

    if (IsOutOfBounds(tileCoords))
        ERROR_AND_DIE("tileCoords is out of bounds")

    int const tileIndex = GetTileIndex(tileCoords);

    return m_values[tileIndex];
}
float TileHeatMap::GetValueAtCoords(IntVec2 tileCoords) const
{
    if (IsOutOfBounds(tileCoords))
        ERROR_AND_DIE("tileCoords is out of bounds")

    int const tileIndex = GetTileIndex(tileCoords);

    return m_values[tileIndex];
}

void TileHeatMap::AddVertsForDebugDraw(VertexList&       verts,
                                       AABB2 const&      totalBounds,
                                       FloatRange const& valueRange,
                                       Rgba8 const&      lowColor,
                                       Rgba8 const&      highColor,
                                       float const       specialValue,
                                       Rgba8 const&      specialColor) const
{
    // Do some math?

    for (int tileY = 0; tileY < m_dimensions.y; tileY++)
    {
        for (int tileX = 0; tileX < m_dimensions.x; tileX++)
        {
            int   tileIndex           = tileX + (tileY * m_dimensions.x);
            float value               = m_values[tileIndex];
            float fractionWithinRange = GetFractionWithinRange(value, valueRange.m_min, valueRange.m_max);
// RangeMapClamped()
            Rgba8 color = Interpolate(lowColor, highColor, fractionWithinRange);

            if ((int) value == (int) specialValue)
            {
                color = specialColor;
            }

            // calculate physical quad bounds(vertex min/max corners)
            float outMinX = RangeMapClamped(static_cast<float>(tileX), 0.f, (float) m_dimensions.x, totalBounds.m_mins.x, totalBounds.m_maxs.x);
            float outMinY = RangeMapClamped(static_cast<float>(tileY), 0.f, (float) m_dimensions.y, totalBounds.m_mins.y, totalBounds.m_maxs.y);
            float outMaxX = RangeMapClamped(static_cast<float>(tileX + 1), 0.f, (float) m_dimensions.x, totalBounds.m_mins.x, totalBounds.m_maxs.x);
            float outMaxY = RangeMapClamped(static_cast<float>(tileY + 1), 0.f, (float) m_dimensions.y, totalBounds.m_mins.y, totalBounds.m_maxs.y);

            AABB2 tileBounds(outMinX, outMinY, outMaxX, outMaxY);
            // printf("(%d), (%d,%d,%d,%d)\n", tileIndex, color.r, color.g, color.b, color.a);
            AddVertsForAABB2D(verts, tileBounds, color);
        }
    }

}
