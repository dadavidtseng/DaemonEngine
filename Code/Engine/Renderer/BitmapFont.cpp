//----------------------------------------------------------------------------------------------------
// BitmapFont.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Renderer/BitmapFont.hpp"

#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Math/AABB2.hpp"

//----------------------------------------------------------------------------------------------------
BitmapFont::BitmapFont(char const* fontFilePathNameWithNoExtension, Texture const& fontTexture, IntVec2 const& spriteCoords)
    : m_fontFilePathNameWithNoExtension(fontFilePathNameWithNoExtension)
      , m_fontGlyphsSpriteSheet(fontTexture, spriteCoords)
{
}

//----------------------------------------------------------------------------------------------------
Texture const& BitmapFont::GetTexture() const
{
    return m_fontGlyphsSpriteSheet.GetTexture();
}

//----------------------------------------------------------------------------------------------------
void BitmapFont::AddVertsForText2D(VertexList&   verts,
                                   Vec2 const&   textMins,
                                   float const   cellHeight,
                                   String const& text,
                                   Rgba8 const&  tint,
                                   float const   cellAspectScale) const
{
    Vec2 currentPosition = textMins; // Create a local copy to modify

    for (char const& c : text)
    {
        int const   glyphIndex  = static_cast<unsigned char>(c);
        AABB2       uvs         = m_fontGlyphsSpriteSheet.GetSpriteUVs(glyphIndex);
        float const glyphAspect = GetGlyphAspect(glyphIndex);
        Vec2 const  glyphSize(cellHeight * glyphAspect * cellAspectScale, cellHeight);

        AddVertsForAABB2D(verts, AABB2(currentPosition, currentPosition + glyphSize), tint, uvs.m_mins, uvs.m_maxs);

        currentPosition.x += glyphSize.x;
    }
}

//----------------------------------------------------------------------------------------------------
void BitmapFont::AddVertsForTextInBox2D(VertexList&       verts,
                                        String const&     text,
                                        AABB2 const&      box,
                                        float             cellHeight,
                                        Rgba8 const&      tint,
                                        float        cellAspectScale,
                                        Vec2 const&       alignment,
                                        TextBoxMode const mode,
                                        int const         maxGlyphsToDraw) const
{
    // 1. Split text string on delimiter '\n' and store them in StingList.
    StringList const lines = SplitStringOnDelimiter(text, '\n');

    // 2. Calculate the totalTextHeight and initialize the maxLineWidth.
    float const totalTextHeight = cellHeight * static_cast<float>(lines.size());
    float       maxLineWidth    = 0.f;

    // 3. Update the maxLineWidth by for looping all lines.
    for (String const& line : lines)
    {
        float lineWidth = GetTextWidth(cellHeight, line, cellAspectScale);
        maxLineWidth    = std::max(maxLineWidth, lineWidth);  // 更新最大行寬
    }

    // 如果是 SHRINK_TO_FIT 模式，則計算縮放因子以使文字適應邊界框
    float scaleFactor = 1.f;
    if (mode == SHRINK_TO_FIT)
    {
        float const horizontalScale = box.GetDimensions().x / maxLineWidth;
        float const verticalScale   = box.GetDimensions().y / totalTextHeight;
        scaleFactor                 = std::min(horizontalScale, verticalScale); // 選擇較小的比例
    }

    // 根據縮放比例調整字元的高度和寬高比例
    cellHeight *= scaleFactor;
    // cellAspectScale *= scaleFactor; // If you want to apply scaling here as well, uncomment this line

    // 計算最終文字區塊的寬度和高度
    float const finalTextWidth  = maxLineWidth * scaleFactor;
    float const finalTextHeight = totalTextHeight * scaleFactor;

    // 根據對齊方式計算文字區塊的初始位置
    float const offsetX = (box.GetDimensions().x - finalTextWidth) * alignment.x;
    float const offsetY = (box.GetDimensions().y - finalTextHeight) * alignment.y;

    // 文字的起始位置
    Vec2 startPos = box.m_mins + Vec2(offsetX + alignment.x, offsetY + cellHeight * ((float)lines.size() - 1.f));

    // 設置當前文字位置為起始位置
    Vec2 currentPos = startPos;
    int  glyphCount = 0;

    // 遍歷每行文字並將頂點加入 vertexArray
    for (String const& line : lines)
    {
        if (line.empty())  // 忽略空行
            continue;

        // 計算當前行的寬度
        float const lineWidth = GetTextWidth(cellHeight, line, cellAspectScale);

        // 根據水平對齊方式計算該行的起始 X 位置
        float const lineOffsetX = (box.GetDimensions().x - lineWidth) * alignment.x;
        currentPos.x            = box.m_mins.x + lineOffsetX;

        // 將當前行的每個字元添加到頂點數組中
        for (char const& c : line)
        {
            if (glyphCount >= maxGlyphsToDraw)  // 如果超過最大顯示字元數，則停止渲染
                break;

            // 根據字元獲取字形的 UV 坐標
            int   glyphIndex  = static_cast<unsigned char>(c);
            AABB2 uvs         = m_fontGlyphsSpriteSheet.GetSpriteUVs(glyphIndex);
            float glyphAspect = GetGlyphAspect(glyphIndex);  // 獲取字形的寬高比例
            Vec2  glyphSize(cellHeight * glyphAspect * cellAspectScale, cellHeight);  // 計算字形的寬度和高度

            // 將字形的頂點數據添加到 vertexArray
            AddVertsForAABB2D(verts, AABB2(currentPos, currentPos + glyphSize), tint, uvs.m_mins, uvs.m_maxs);

            // 更新當前字形的 x 位置，準備渲染下一個字形
            currentPos.x += glyphSize.x;

            glyphCount++;  // 增加已渲染字形的計數
        }

        // 移動到下一行，y坐標減少
        currentPos.y -= cellHeight;
    }
}

//----------------------------------------------------------------------------------------------------
void BitmapFont::AddVertsForText3DAtOriginXForward(VertexList&   verts,
                                                   float         cellHeight,
                                                   String const& text,
                                                   Rgba8 const&  tine,
                                                   float         cellAspect,
                                                   Vec2 const&   alignment,
                                                   int           maxGlyphsToDraw)
{
}

//----------------------------------------------------------------------------------------------------
float BitmapFont::GetTextWidth(float const   cellHeight,
                               String const& text,
                               float const   cellAspectScale) const
{
    float totalWidth = 0.f;

    for (char const& c : text)
    {
        int const   glyphIndex  = static_cast<unsigned char>(c);
        float const glyphAspect = GetGlyphAspect(glyphIndex);

        totalWidth += cellHeight * glyphAspect * cellAspectScale;
    }

    return totalWidth;
}

//----------------------------------------------------------------------------------------------------
float BitmapFont::GetGlyphAspect(int const glyphUnicode) const
{
    UNUSED(glyphUnicode)
    return m_fontDefaultAspect;
}
