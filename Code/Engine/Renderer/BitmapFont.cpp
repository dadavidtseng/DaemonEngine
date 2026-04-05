//----------------------------------------------------------------------------------------------------
// BitmapFont.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Renderer/BitmapFont.hpp"

#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Renderer/Image.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Renderer/Vertex_Font.hpp"
#include "Engine/Resource/ResourceSubsystem.hpp"

//----------------------------------------------------------------------------------------------------
// Static member initialization
int BitmapFont::s_totalCreated = 0;
int BitmapFont::s_totalDeleted = 0;

//----------------------------------------------------------------------------------------------------
BitmapFont::BitmapFont(char const*    fontFilePathNameWithNoExtension,
                       Texture const& fontTexture,
                       IntVec2 const& spriteCoords)
    : m_fontFilePathNameWithNoExtension(fontFilePathNameWithNoExtension),
      m_fontGlyphsSpriteSheet(fontTexture, spriteCoords)
{
    s_totalCreated++;
    DebuggerPrintf("[BitmapFont] Constructor: Created font #%d '%s' (this=%p), Alive=%d\n",
                   s_totalCreated, fontFilePathNameWithNoExtension, this, GetAliveCount());
}

//----------------------------------------------------------------------------------------------------
BitmapFont::BitmapFont(char const* fontFilePathNameWithNoExtension, Texture* fontTexture, IntVec2 const& spriteCoords, bool ownsTexture)
    : m_fontFilePathNameWithNoExtension(fontFilePathNameWithNoExtension)
      , m_fontGlyphsSpriteSheet(*fontTexture, spriteCoords)
      , m_ownedTexture(ownsTexture ? fontTexture : nullptr)
{
    s_totalCreated++;
    DebuggerPrintf("[BitmapFont] Constructor (owning): Created font #%d '%s' (this=%p), ownsTexture=%d, Alive=%d\n",
                   s_totalCreated, fontFilePathNameWithNoExtension, this, ownsTexture, GetAliveCount());
}

//----------------------------------------------------------------------------------------------------
BitmapFont::~BitmapFont()
{
    s_totalDeleted++;
    // Don't call DebuggerPrintf during destruction - logging system may be shut down

    // Delete owned texture if we own it
    if (m_ownedTexture)
    {
        delete m_ownedTexture;
        m_ownedTexture = nullptr;
    }
}

//----------------------------------------------------------------------------------------------------
int BitmapFont::GetAliveCount()
{
    return s_totalCreated - s_totalDeleted;
}

//----------------------------------------------------------------------------------------------------
int BitmapFont::GetTotalCreated()
{
    return s_totalCreated;
}

//----------------------------------------------------------------------------------------------------
int BitmapFont::GetTotalDeleted()
{
    return s_totalDeleted;
}

//----------------------------------------------------------------------------------------------------
void BitmapFont::ReportLeakStatus()
{
    int alive = GetAliveCount();
    DebuggerPrintf("========================================\n");
    DebuggerPrintf("[BitmapFont] LEAK REPORT:\n");
    DebuggerPrintf("[BitmapFont]   Total Created: %d\n", s_totalCreated);
    DebuggerPrintf("[BitmapFont]   Total Deleted: %d\n", s_totalDeleted);
    DebuggerPrintf("[BitmapFont]   Still Alive:   %d\n", alive);
    if (alive > 0)
    {
        DebuggerPrintf("[BitmapFont]   *** LEAK DETECTED: %d fonts not deleted! ***\n", alive);
    }
    else
    {
        DebuggerPrintf("[BitmapFont]   No leaks detected.\n");
    }
    DebuggerPrintf("========================================\n");
}

//----------------------------------------------------------------------------------------------------
Texture const& BitmapFont::GetTexture() const
{
    // For Tier 3+, prefer page texture from BMFont metadata if available
    if (!m_pageTextures.empty() && m_pageTextures[0] != nullptr)
    {
        return *m_pageTextures[0];
    }
    return m_fontGlyphsSpriteSheet.GetTexture();
}

//----------------------------------------------------------------------------------------------------
void BitmapFont::AddVertsForText2D(VertexList_PCU& verts,
                                   String const&   text,
                                   Vec2 const&     textMins,
                                   float const     cellHeight,
                                   Rgba8 const&    tint,
                                   float const     cellAspectRatio) const
{
    Vec2 currentPosition = textMins;

    int prevChar = -1;

    for (int i = 0; i < static_cast<int>(text.size()); ++i)
    {
        int const glyphIndex = static_cast<unsigned char>(text[i]);

        // Apply kerning between adjacent characters (Tier 3+)
        if (prevChar >= 0 && m_fontTier >= eFontTier::TIER_3)
        {
            float const kern  = GetKerningAmount(prevChar, glyphIndex);
            float const scale = (m_lineHeight > 0.f) ? (cellHeight / m_lineHeight) : 1.f;
            currentPosition.x += kern * scale;
        }

        sGlyphData const* glyph = GetGlyphData(glyphIndex);

        if (glyph && m_fontTier >= eFontTier::TIER_2)
        {
            // Tier 2+: use GlyphData UVs and metrics
            float const scale = (m_lineHeight > 0.f) ? (cellHeight / m_lineHeight) : 1.f;

            Vec2 const uvMins(glyph->m_uvMinsX, glyph->m_uvMinsY);
            Vec2 const uvMaxs(glyph->m_uvMaxsX, glyph->m_uvMaxsY);

            float const drawW = glyph->m_width * scale * cellAspectRatio;
            float const drawH = glyph->m_height * scale;
            float const offX  = glyph->m_xOffset * scale * cellAspectRatio;
            float const offY  = (m_lineHeight - glyph->m_yOffset - glyph->m_height) * scale; // Flip Y offset

            Vec2 const glyphMins(currentPosition.x + offX, currentPosition.y + offY);
            Vec2 const glyphMaxs(glyphMins.x + drawW, glyphMins.y + drawH);

            AddVertsForAABB2D(verts, AABB2(glyphMins, glyphMaxs), tint, uvMins, uvMaxs);

            currentPosition.x += glyph->m_xAdvance * scale * cellAspectRatio;
        }
        else
        {
            // Tier 1: original SpriteSheet path
            AABB2       uvs         = m_fontGlyphsSpriteSheet.GetSpriteUVs(glyphIndex);
            float const glyphAspect = GetGlyphAspect(glyphIndex);
            Vec2 const  glyphSize(cellHeight * glyphAspect * cellAspectRatio, cellHeight);

            AddVertsForAABB2D(verts, AABB2(currentPosition, currentPosition + glyphSize), tint, uvs.m_mins, uvs.m_maxs);

            currentPosition.x += glyphSize.x;
        }

        prevChar = glyphIndex;
    }
}

//----------------------------------------------------------------------------------------------------
void BitmapFont::AddVertsForText2D(VertexList_Font& verts,
                                   String const&    text,
                                   Vec2 const&      textMins,
                                   float const      cellHeight,
                                   Rgba8 const&     tint,
                                   float const      cellAspectRatio) const
{
    // First pass: compute total text width for textPosition normalization
    float const totalWidth = GetTextWidth(cellHeight, text, cellAspectRatio);
    if (totalWidth <= 0.f)
    {
        return;
    }

    Vec2  currentPosition = textMins;
    int   prevChar        = -1;
    float cursorX         = 0.f; // Track cursor offset from textMins for textPosition

    for (int i = 0; i < static_cast<int>(text.size()); ++i)
    {
        int const glyphIndex = static_cast<unsigned char>(text[i]);

        // Apply kerning (Tier 3+)
        if (prevChar >= 0 && m_fontTier >= eFontTier::TIER_3)
        {
            float const kern  = GetKerningAmount(prevChar, glyphIndex);
            float const scale = (m_lineHeight > 0.f) ? (cellHeight / m_lineHeight) : 1.f;
            currentPosition.x += kern * scale;
            cursorX += kern * scale;
        }

        sGlyphData const* glyph = GetGlyphData(glyphIndex);

        Vec2  uvMins, uvMaxs;
        float drawW, drawH, offX, offY, advance;

        if (glyph && m_fontTier >= eFontTier::TIER_2)
        {
            float const scale = (m_lineHeight > 0.f) ? (cellHeight / m_lineHeight) : 1.f;
            uvMins  = Vec2(glyph->m_uvMinsX, glyph->m_uvMinsY);
            uvMaxs  = Vec2(glyph->m_uvMaxsX, glyph->m_uvMaxsY);
            drawW   = glyph->m_width * scale * cellAspectRatio;
            drawH   = glyph->m_height * scale;
            offX    = glyph->m_xOffset * scale * cellAspectRatio;
            offY    = (m_lineHeight - glyph->m_yOffset - glyph->m_height) * scale;
            advance = glyph->m_xAdvance * scale * cellAspectRatio;
        }
        else
        {
            AABB2 const spriteUVs  = m_fontGlyphsSpriteSheet.GetSpriteUVs(glyphIndex);
            float const glyphAspect = GetGlyphAspect(glyphIndex);
            uvMins  = spriteUVs.m_mins;
            uvMaxs  = spriteUVs.m_maxs;
            drawW   = cellHeight * glyphAspect * cellAspectRatio;
            drawH   = cellHeight;
            offX    = 0.f;
            offY    = 0.f;
            advance = drawW;
        }

        Vec2 const glyphMins(currentPosition.x + offX, currentPosition.y + offY);
        Vec2 const glyphMaxs(glyphMins.x + drawW, glyphMins.y + drawH);

        // Normalized text position for this glyph
        float const textPosX = (totalWidth > 0.f) ? (cursorX / totalWidth) : 0.f;

        // 6 vertices per glyph (two triangles)
        // Bottom-left, Bottom-right, Top-right, Bottom-left, Top-right, Top-left
        verts.emplace_back(Vec3(glyphMins.x, glyphMins.y, 0.f), tint, Vec2(uvMins.x, uvMins.y), Vec2(0.f, 0.f), Vec2(textPosX, 0.f), i, 0.f);
        verts.emplace_back(Vec3(glyphMaxs.x, glyphMins.y, 0.f), tint, Vec2(uvMaxs.x, uvMins.y), Vec2(1.f, 0.f), Vec2(textPosX, 0.f), i, 0.f);
        verts.emplace_back(Vec3(glyphMaxs.x, glyphMaxs.y, 0.f), tint, Vec2(uvMaxs.x, uvMaxs.y), Vec2(1.f, 1.f), Vec2(textPosX, 1.f), i, 0.f);
        verts.emplace_back(Vec3(glyphMins.x, glyphMins.y, 0.f), tint, Vec2(uvMins.x, uvMins.y), Vec2(0.f, 0.f), Vec2(textPosX, 0.f), i, 0.f);
        verts.emplace_back(Vec3(glyphMaxs.x, glyphMaxs.y, 0.f), tint, Vec2(uvMaxs.x, uvMaxs.y), Vec2(1.f, 1.f), Vec2(textPosX, 1.f), i, 0.f);
        verts.emplace_back(Vec3(glyphMins.x, glyphMaxs.y, 0.f), tint, Vec2(uvMins.x, uvMaxs.y), Vec2(0.f, 1.f), Vec2(textPosX, 1.f), i, 0.f);

        currentPosition.x += advance;
        cursorX += advance;
        prevChar = glyphIndex;
    }
}

//----------------------------------------------------------------------------------------------------
void BitmapFont::AddVertsForTextInBox2D(VertexList_Font&   verts,
                                        String const&      text,
                                        AABB2 const&       box,
                                        float              cellHeight,
                                        Rgba8 const&       tint,
                                        float const        cellAspectRatio,
                                        Vec2 const&        alignment,
                                        eTextBoxMode const mode,
                                        int const          maxGlyphsToDraw) const
{
    // Simplified: render text at box mins with alignment offset
    // Reuse the PCU version's line-splitting and alignment logic by converting to PCU first,
    // or implement a simplified single-line version for demo purposes.

    float const textWidth = GetTextWidth(cellHeight, text, cellAspectRatio);
    float const boxWidth  = box.m_maxs.x - box.m_mins.x;
    float const boxHeight = box.m_maxs.y - box.m_mins.y;

    float usedCellHeight = cellHeight;

    if (mode == eTextBoxMode::SHRINK_TO_FIT)
    {
        if (textWidth > boxWidth && textWidth > 0.f)
        {
            usedCellHeight *= (boxWidth / textWidth);
        }
        if (usedCellHeight > boxHeight && boxHeight > 0.f)
        {
            usedCellHeight = boxHeight;
        }
    }

    float const usedTextWidth = GetTextWidth(usedCellHeight, text, cellAspectRatio);
    float const offsetX       = (boxWidth - usedTextWidth) * alignment.x;
    float const offsetY       = (boxHeight - usedCellHeight) * alignment.y;

    Vec2 const textMins(box.m_mins.x + offsetX, box.m_mins.y + offsetY);

    // Apply glyph limit
    String const clampedText = (maxGlyphsToDraw >= 0 && maxGlyphsToDraw < static_cast<int>(text.size()))
                                   ? text.substr(0, maxGlyphsToDraw)
                                   : text;

    AddVertsForText2D(verts, clampedText, textMins, usedCellHeight, tint, cellAspectRatio);
}

//----------------------------------------------------------------------------------------------------
void BitmapFont::AddVertsForTextInBox2D(VertexList_PCU&    verts,
                                        String const&      text,
                                        AABB2 const&       box,
                                        float              cellHeight,
                                        Rgba8 const&       tint,
                                        float const        cellAspectRatio,
                                        Vec2 const&        alignment,
                                        eTextBoxMode const mode,
                                        int const          maxGlyphsToDraw) const
{
    // 1. Split text string on delimiter '\n' and store them in StingList.
    StringList const lines = SplitStringOnDelimiter(text, '\n');

    // 2. Initialize the maxLineWidth, and calculate the totalLineHeight.
    float       maxLineWidth    = 0.f;
    float const totalLineHeight = cellHeight * static_cast<float>(lines.size());

    // 3. Update the maxLineWidth by for-looping all lines.
    for (String const& line : lines)
    {
        float lineWidth = GetTextWidth(cellHeight, line, cellAspectRatio);
        maxLineWidth    = (std::max)(maxLineWidth, lineWidth);
    }

    // 4. If eTextBoxMode is set to SHRINK_TO_FIT,
    float scaleFactor = 1.f;

    if (mode == eTextBoxMode::SHRINK_TO_FIT)
    {
        // Get the horizontalScale and verticalScale, and set scaleFactor to whichever is smaller.
        float const horizontalScale = box.GetDimensions().x / maxLineWidth;
        float const verticalScale   = box.GetDimensions().y / totalLineHeight;
        scaleFactor                 = (std::min)(horizontalScale, verticalScale);
    }

    // 5. Apply the scaleFactor to cellHeight, maxLineWidth, and totalLineHeight.
    cellHeight *= scaleFactor;
    float const finalTextWidth  = maxLineWidth * scaleFactor;
    float const finalTextHeight = totalLineHeight * scaleFactor;

    // 6. Calculate the adjustmentX and adjustmentY based on alignment.
    float const adjustmentX = (box.GetDimensions().x - finalTextWidth) * alignment.x;
    float const adjustmentY = (box.GetDimensions().y - finalTextHeight) * alignment.y;

    // 7. Calculate the startPosition of all the lines ( StringList ), which is bottomLeft ( box.m_mins ) plus adjustment.
    Vec2 const startPosition = box.m_mins + Vec2(adjustmentX, adjustmentY + cellHeight * (static_cast<float>(lines.size()) - 1.f));

    // 8. Initialize the currentPosition of all the lines ( StringList ) and glyphCount.
    Vec2 currentPosition = startPosition;
    int  glyphCount      = 0;

    // 9. Stores vertices into VertList.
    for (String const& line : lines)
    {
        // 10. Skip the line if empty.
        if (line.empty())
        {
            continue;
        }

        // 11. Adjust the currentPosition similarly to how the totalLines box is adjusted above.
        float const lineWidth       = GetTextWidth(cellHeight, line, cellAspectRatio);
        float const lineAdjustmentX = (box.GetDimensions().x - lineWidth) * alignment.x;
        currentPosition.x           = box.m_mins.x + lineAdjustmentX;

        // 12. Stores each char into VertList.
        int prevChar = -1;

        for (int ci = 0; ci < static_cast<int>(line.size()); ++ci)
        {
            // 13. If glyphCount is larger than maxGlyphsToDraw, stop rendering this line.
            if (glyphCount >= maxGlyphsToDraw)
            {
                break;
            }

            int const glyphIndex = static_cast<unsigned char>(line[ci]);

            // Apply kerning (Tier 3+)
            if (prevChar >= 0 && m_fontTier >= eFontTier::TIER_3)
            {
                float const kern  = GetKerningAmount(prevChar, glyphIndex);
                float const scale = (m_lineHeight > 0.f) ? (cellHeight / m_lineHeight) : 1.f;
                currentPosition.x += kern * scale;
            }

            sGlyphData const* glyph = GetGlyphData(glyphIndex);

            if (glyph && m_fontTier >= eFontTier::TIER_2)
            {
                // Tier 2+: use GlyphData UVs and metrics
                float const scale = (m_lineHeight > 0.f) ? (cellHeight / m_lineHeight) : 1.f;

                Vec2 const uvMins(glyph->m_uvMinsX, glyph->m_uvMinsY);
                Vec2 const uvMaxs(glyph->m_uvMaxsX, glyph->m_uvMaxsY);

                float const drawW = glyph->m_width * scale * cellAspectRatio;
                float const drawH = glyph->m_height * scale;
                float const offX  = glyph->m_xOffset * scale * cellAspectRatio;
                float const offY  = (m_lineHeight - glyph->m_yOffset - glyph->m_height) * scale;

                Vec2 const glyphMins(currentPosition.x + offX, currentPosition.y + offY);
                Vec2 const glyphMaxs(glyphMins.x + drawW, glyphMins.y + drawH);

                AddVertsForAABB2D(verts, AABB2(glyphMins, glyphMaxs), tint, uvMins, uvMaxs);

                currentPosition.x += glyph->m_xAdvance * scale * cellAspectRatio;
            }
            else
            {
                // Tier 1: original SpriteSheet path
                AABB2       UVs         = m_fontGlyphsSpriteSheet.GetSpriteUVs(glyphIndex);
                float const glyphAspect = GetGlyphAspect(glyphIndex);
                Vec2        glyphSize(cellHeight * glyphAspect * cellAspectRatio, cellHeight);

                AddVertsForAABB2D(verts, AABB2(currentPosition, currentPosition + glyphSize), tint, UVs.m_mins, UVs.m_maxs);

                currentPosition.x += glyphSize.x;
            }

            prevChar = glyphIndex;
            glyphCount++;
        }

        // 17. Update the char's currentPosition.y and move downward to next line.
        currentPosition.y -= cellHeight;
    }
}

//----------------------------------------------------------------------------------------------------
void BitmapFont::AddVertsForText3DAtOriginXForward(VertexList_PCU& verts,
                                                   String const&   text,
                                                   float const     cellHeight,
                                                   Rgba8 const&    tint,
                                                   float const     cellAspectRatio,
                                                   Vec2 const&     alignment,
                                                   int const       maxGlyphsToDraw) const
{
    float const textWidth = GetTextWidth(cellHeight, text, cellAspectRatio);

    AABB2 const box = AABB2(Vec2::ZERO, Vec2(textWidth, cellHeight));

    AddVertsForTextInBox2D(verts, text, box, cellHeight, tint, cellAspectRatio, alignment, eTextBoxMode::OVERRUN, maxGlyphsToDraw);

    Mat44 transform;

    transform.SetIJKT3D(Vec3::Y_BASIS, Vec3::Z_BASIS, Vec3::X_BASIS, Vec3(0.f, -textWidth * (1.f - alignment.x), -cellHeight * (1.f - alignment.y)));

    TransformVertexArray3D(verts, transform);
}

//----------------------------------------------------------------------------------------------------
float BitmapFont::GetTextWidth(float const   cellHeight,
                               String const& text,
                               float const   cellAspectRatio) const
{
    float totalWidth = 0.f;
    int   prevChar   = -1;

    for (int i = 0; i < static_cast<int>(text.size()); ++i)
    {
        int const glyphIndex = static_cast<unsigned char>(text[i]);

        // Apply kerning (Tier 3+)
        if (prevChar >= 0 && m_fontTier >= eFontTier::TIER_3)
        {
            float const kern  = GetKerningAmount(prevChar, glyphIndex);
            float const scale = (m_lineHeight > 0.f) ? (cellHeight / m_lineHeight) : 1.f;
            totalWidth += kern * scale;
        }

        sGlyphData const* glyph = GetGlyphData(glyphIndex);

        if (glyph && m_fontTier >= eFontTier::TIER_2)
        {
            float const scale = (m_lineHeight > 0.f) ? (cellHeight / m_lineHeight) : 1.f;
            totalWidth += glyph->m_xAdvance * scale * cellAspectRatio;
        }
        else
        {
            float const glyphAspect = GetGlyphAspect(glyphIndex);
            totalWidth += cellHeight * glyphAspect * cellAspectRatio;
        }

        prevChar = glyphIndex;
    }

    return totalWidth;
}

//----------------------------------------------------------------------------------------------------
float BitmapFont::GetGlyphAspect(int const glyphUnicode) const
{
    sGlyphData const* glyph = GetGlyphData(glyphUnicode);
    if (glyph && glyph->m_height > 0.f)
    {
        return glyph->m_width / glyph->m_height;
    }
    return m_fontDefaultAspect;
}

//----------------------------------------------------------------------------------------------------
eFontTier BitmapFont::GetFontTier() const
{
    return m_fontTier;
}

//----------------------------------------------------------------------------------------------------
bool BitmapFont::IsSDF() const
{
    return m_isSDF;
}

//----------------------------------------------------------------------------------------------------
float BitmapFont::GetKerningAmount(int const firstChar, int const secondChar) const
{
    uint64_t const key = (static_cast<uint64_t>(firstChar) << 32) | static_cast<uint64_t>(secondChar);
    auto const     it  = m_kerningPairs.find(key);
    if (it != m_kerningPairs.end())
    {
        return it->second;
    }
    return 0.f;
}

//----------------------------------------------------------------------------------------------------
sGlyphData const* BitmapFont::GetGlyphData(int const glyphUnicode) const
{
    auto const it = m_glyphData.find(glyphUnicode);
    if (it != m_glyphData.end())
    {
        return &it->second;
    }
    return nullptr;
}

//----------------------------------------------------------------------------------------------------
void BitmapFont::ComputeAutoWidths(Image const& image)
{
    IntVec2 const imageDims = image.GetDimensions();
    int const     cellW     = imageDims.x / 16;
    int const     cellH     = imageDims.y / 16;

    if (cellW <= 0 || cellH <= 0)
    {
        return;
    }

    for (int glyphIndex = 0; glyphIndex < 256; ++glyphIndex)
    {
        int const gridCol  = glyphIndex % 16;
        int const gridRow  = glyphIndex / 16;
        int const cellMinX = gridCol * cellW;
        int const cellMinY = gridRow * cellH;

        // Scan for leftmost and rightmost non-transparent columns
        int leftmost  = cellW;
        int rightmost = -1;

        for (int x = 0; x < cellW; ++x)
        {
            for (int y = 0; y < cellH; ++y)
            {
                Rgba8 const texel = image.GetTexelColor(IntVec2(cellMinX + x, cellMinY + y));
                if (texel.a > 0)
                {
                    if (x < leftmost)  leftmost  = x;
                    if (x > rightmost) rightmost = x;
                    break;
                }
            }
        }

        sGlyphData glyph;
        glyph.m_id     = glyphIndex;
        glyph.m_height = static_cast<float>(cellH);
        glyph.m_page   = 0;

        if (rightmost >= leftmost)
        {
            int const pad        = 1;
            int const tightLeft  = (leftmost > pad) ? (leftmost - pad) : 0;
            int const tightRight = (rightmost + pad < cellW - 1) ? (rightmost + pad) : (cellW - 1);
            float const pixelW   = static_cast<float>(tightRight - tightLeft + 1);

            glyph.m_width    = pixelW;
            glyph.m_xOffset  = 0.f;
            glyph.m_yOffset  = 0.f;
            glyph.m_xAdvance = pixelW + 1.f;

            glyph.m_uvMinsX = static_cast<float>(cellMinX + tightLeft) / static_cast<float>(imageDims.x);
            glyph.m_uvMaxsX = static_cast<float>(cellMinX + tightRight + 1) / static_cast<float>(imageDims.x);
            glyph.m_uvMaxsY = 1.f - static_cast<float>(cellMinY) / static_cast<float>(imageDims.y);
            glyph.m_uvMinsY = 1.f - static_cast<float>(cellMinY + cellH) / static_cast<float>(imageDims.y);
        }
        else
        {
            float const halfW = static_cast<float>(cellW) * 0.5f;
            glyph.m_width    = halfW;
            glyph.m_xOffset  = 0.f;
            glyph.m_yOffset  = 0.f;
            glyph.m_xAdvance = halfW + 1.f;

            glyph.m_uvMinsX = static_cast<float>(cellMinX) / static_cast<float>(imageDims.x);
            glyph.m_uvMaxsX = static_cast<float>(cellMinX + cellW) / static_cast<float>(imageDims.x);
            glyph.m_uvMaxsY = 1.f - static_cast<float>(cellMinY) / static_cast<float>(imageDims.y);
            glyph.m_uvMinsY = 1.f - static_cast<float>(cellMinY + cellH) / static_cast<float>(imageDims.y);
        }

        m_glyphData[glyphIndex] = glyph;
    }

    m_lineHeight = static_cast<float>(cellH);
    m_fontTier   = eFontTier::TIER_2;
}

//----------------------------------------------------------------------------------------------------
// Helper: parse a key=value pair from a BMFont token (handles quoted values)
//----------------------------------------------------------------------------------------------------
static bool ParseBMFontKeyValue(String const& token, String& out_key, String& out_value)
{
    size_t const eqPos = token.find('=');
    if (eqPos == String::npos || eqPos == 0)
    {
        return false;
    }

    out_key   = token.substr(0, eqPos);
    out_value = token.substr(eqPos + 1);

    // Strip surrounding quotes if present
    if (out_value.size() >= 2 && out_value.front() == '"' && out_value.back() == '"')
    {
        out_value = out_value.substr(1, out_value.size() - 2);
    }

    return true;
}

//----------------------------------------------------------------------------------------------------
// Tokenize a BMFont line respecting quoted strings (e.g., face="Times New Roman")
//----------------------------------------------------------------------------------------------------
static StringList TokenizeBMFontLine(String const& line)
{
    StringList tokens;
    String     current;
    bool       inQuotes = false;

    for (size_t i = 0; i < line.size(); ++i)
    {
        char const c = line[i];

        if (c == '"')
        {
            inQuotes = !inQuotes;
            current += c;
        }
        else if (c == ' ' && !inQuotes)
        {
            if (!current.empty())
            {
                tokens.push_back(current);
                current.clear();
            }
        }
        else
        {
            current += c;
        }
    }

    if (!current.empty())
    {
        tokens.push_back(current);
    }

    return tokens;
}

//----------------------------------------------------------------------------------------------------
bool BitmapFont::ParseBMFontFile(String const& fntFilePath)
{
    String fileContents;
    if (!FileReadToString(fileContents, fntFilePath))
    {
        return false;
    }

    // Extract directory from fntFilePath for resolving page texture paths
    String fontDir;
    size_t const lastSlash = fntFilePath.find_last_of("/\\");
    if (lastSlash != String::npos)
    {
        fontDir = fntFilePath.substr(0, lastSlash + 1);
    }

    // Parse padding for SDF detection
    int paddingUp    = 0;
    int paddingRight = 0;
    int paddingDown  = 0;
    int paddingLeft  = 0;

    StringList lines = SplitStringOnDelimiter(fileContents, '\n');

    for (String const& rawLine : lines)
    {
        // Trim trailing \r
        String line = rawLine;
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }

        if (line.empty())
        {
            continue;
        }

        StringList tokens = TokenizeBMFontLine(line);
        if (tokens.empty())
        {
            continue;
        }

        String const& lineType = tokens[0];

        // Build key-value map from remaining tokens
        std::map<String, String> kvMap;
        for (size_t i = 1; i < tokens.size(); ++i)
        {
            String key, value;
            if (ParseBMFontKeyValue(tokens[i], key, value))
            {
                kvMap[key] = value;
            }
        }

        if (lineType == "info")
        {
            // Parse padding for SDF detection
            auto const itPadding = kvMap.find("padding");
            if (itPadding != kvMap.end())
            {
                StringList padParts = SplitStringOnDelimiter(itPadding->second, ',');
                if (padParts.size() >= 4)
                {
                    paddingUp    = atoi(padParts[0].c_str());
                    paddingRight = atoi(padParts[1].c_str());
                    paddingDown  = atoi(padParts[2].c_str());
                    paddingLeft  = atoi(padParts[3].c_str());
                }
            }
        }
        else if (lineType == "common")
        {
            auto const itLH = kvMap.find("lineHeight");
            auto const itB  = kvMap.find("base");
            auto const itSW = kvMap.find("scaleW");
            auto const itSH = kvMap.find("scaleH");

            if (itLH != kvMap.end()) m_lineHeight = static_cast<float>(atoi(itLH->second.c_str()));
            if (itB  != kvMap.end()) m_base       = static_cast<float>(atoi(itB->second.c_str()));
            if (itSW != kvMap.end()) m_scaleW     = atoi(itSW->second.c_str());
            if (itSH != kvMap.end()) m_scaleH     = atoi(itSH->second.c_str());
        }
        else if (lineType == "page")
        {
            auto const itFile = kvMap.find("file");
            if (itFile != kvMap.end())
            {
                String const pageTexturePath = fontDir + itFile->second;
                Texture* pageTexture = g_resourceSubsystem->CreateOrGetTextureFromFile(pageTexturePath);
                if (pageTexture)
                {
                    m_pageTextures.push_back(pageTexture);
                }
                else
                {
                    ERROR_RECOVERABLE(Stringf("BMFont: Failed to load page texture '%s'", pageTexturePath.c_str()));
                    m_pageTextures.push_back(nullptr);
                }
            }
        }
        else if (lineType == "char")
        {
            if (m_scaleW <= 0 || m_scaleH <= 0)
            {
                continue; // Can't compute UVs without atlas dimensions
            }

            sGlyphData glyph;

            auto const itId  = kvMap.find("id");
            auto const itX   = kvMap.find("x");
            auto const itY   = kvMap.find("y");
            auto const itW   = kvMap.find("width");
            auto const itH   = kvMap.find("height");
            auto const itXO  = kvMap.find("xoffset");
            auto const itYO  = kvMap.find("yoffset");
            auto const itXA  = kvMap.find("xadvance");
            auto const itP   = kvMap.find("page");

            if (itId == kvMap.end()) continue;

            glyph.m_id       = atoi(itId->second.c_str());
            int const x      = (itX  != kvMap.end()) ? atoi(itX->second.c_str())  : 0;
            int const y      = (itY  != kvMap.end()) ? atoi(itY->second.c_str())  : 0;
            int const w      = (itW  != kvMap.end()) ? atoi(itW->second.c_str())  : 0;
            int const h      = (itH  != kvMap.end()) ? atoi(itH->second.c_str())  : 0;
            glyph.m_xOffset  = (itXO != kvMap.end()) ? static_cast<float>(atoi(itXO->second.c_str())) : 0.f;
            glyph.m_yOffset  = (itYO != kvMap.end()) ? static_cast<float>(atoi(itYO->second.c_str())) : 0.f;
            glyph.m_xAdvance = (itXA != kvMap.end()) ? static_cast<float>(atoi(itXA->second.c_str())) : 0.f;
            glyph.m_page     = (itP  != kvMap.end()) ? atoi(itP->second.c_str())  : 0;

            glyph.m_width  = static_cast<float>(w);
            glyph.m_height = static_cast<float>(h);

            // Compute UVs — flip Y for DirectX convention (image Y=0 is top, UV Y=0 is bottom)
            float const sw = static_cast<float>(m_scaleW);
            float const sh = static_cast<float>(m_scaleH);
            glyph.m_uvMinsX = static_cast<float>(x) / sw;
            glyph.m_uvMaxsX = static_cast<float>(x + w) / sw;
            glyph.m_uvMaxsY = 1.f - static_cast<float>(y) / sh;
            glyph.m_uvMinsY = 1.f - static_cast<float>(y + h) / sh;

            m_glyphData[glyph.m_id] = glyph;
        }
        else if (lineType == "kerning")
        {
            auto const itFirst  = kvMap.find("first");
            auto const itSecond = kvMap.find("second");
            auto const itAmount = kvMap.find("amount");

            if (itFirst != kvMap.end() && itSecond != kvMap.end() && itAmount != kvMap.end())
            {
                int const   first  = atoi(itFirst->second.c_str());
                int const   second = atoi(itSecond->second.c_str());
                float const amount = static_cast<float>(atoi(itAmount->second.c_str()));

                uint64_t const key = (static_cast<uint64_t>(first) << 32) | static_cast<uint64_t>(second);
                m_kerningPairs[key] = amount;
            }
        }
    }

    // Determine font tier based on SDF detection
    if (paddingUp >= 3 && paddingRight >= 3 && paddingDown >= 3 && paddingLeft >= 3)
    {
        m_isSDF    = true;
        m_fontTier = eFontTier::TIER_4;
    }
    else
    {
        m_fontTier = eFontTier::TIER_3;
    }

    DebuggerPrintf("Info: Parsed BMFont '%s' — %zu glyphs, %zu kerning pairs, tier %d%s\n",
                   fntFilePath.c_str(), m_glyphData.size(), m_kerningPairs.size(),
                   static_cast<int>(m_fontTier), m_isSDF ? " (SDF)" : "");

    return true;
}
