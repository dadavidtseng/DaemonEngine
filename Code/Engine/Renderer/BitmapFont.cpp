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
                                   String const& text,
                                   Vec2 const&   textMins,
                                   float const   cellHeight,
                                   Rgba8 const&  tint,
                                   float const   cellAspectRatio) const
{
    Vec2 currentPosition = textMins; // Create a local copy to modify

    for (char const& c : text)
    {
        int const   glyphIndex  = static_cast<unsigned char>(c);
        AABB2       uvs         = m_fontGlyphsSpriteSheet.GetSpriteUVs(glyphIndex);
        float const glyphAspect = GetGlyphAspect(glyphIndex);
        Vec2 const  glyphSize(cellHeight * glyphAspect * cellAspectRatio, cellHeight);

        AddVertsForAABB2D(verts, AABB2(currentPosition, currentPosition + glyphSize), tint, uvs.m_mins, uvs.m_maxs);

        currentPosition.x += glyphSize.x;
    }
}

//----------------------------------------------------------------------------------------------------
void BitmapFont::AddVertsForTextInBox2D(VertexList&        verts,
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
        maxLineWidth    = std::max(maxLineWidth, lineWidth);
    }

    // 4. If eTextBoxMode is set to SHRINK_TO_FIT,
    float scaleFactor = 1.f;

    if (mode == SHRINK_TO_FIT)
    {
        // Get the horizontalScale and verticalScale, and set scaleFactor to whichever is smaller.
        float const horizontalScale = box.GetDimensions().x / maxLineWidth;
        float const verticalScale   = box.GetDimensions().y / totalLineHeight;
        scaleFactor                 = std::min(horizontalScale, verticalScale);
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
        for (char const& c : line)
        {
            // 13. If glyphCount is larger than maxGlyphsToDraw, stop rendering this line.
            if (glyphCount >= maxGlyphsToDraw)
            {
                break;
            }

            // 14. Calculate the glyphSize by getting its glyphAspect and UVs. ( Aspect ratio = Width / Height )
            int const   glyphIndex  = static_cast<unsigned char>(c);
            AABB2       UVs         = m_fontGlyphsSpriteSheet.GetSpriteUVs(glyphIndex);
            float const glyphAspect = GetGlyphAspect(glyphIndex);
            Vec2        glyphSize(cellHeight * glyphAspect * cellAspectRatio, cellHeight);

            // 15. Calculate the char's AABB2 box based on the currentPosition and glyphSize, then add vertices using AddVertsForAABB2D.
            AddVertsForAABB2D(verts, AABB2(currentPosition, currentPosition + glyphSize), tint, UVs.m_mins, UVs.m_maxs);

            // 16. Update the char's currentPosition.x and move rightward to next char, then update the glyphCount.
            currentPosition.x += glyphSize.x;
            glyphCount++;
        }

        // 17. Update the char's currentPosition.y and move downward to next line.
        currentPosition.y -= cellHeight;
    }
}

//----------------------------------------------------------------------------------------------------
void BitmapFont::AddVertsForText3DAtOriginXForward(VertexList&   verts,
                                                   String const& text,
                                                   float const   cellHeight,
                                                   Rgba8 const&  tint,
                                                   float const   cellAspectRatio,
                                                   Vec2 const&   alignment,
                                                   int const     maxGlyphsToDraw)
{
    float const textWidth = GetTextWidth(cellHeight, text, cellAspectRatio);

    AABB2 const box = AABB2(Vec2::ZERO, Vec2(textWidth, cellHeight));

    AddVertsForTextInBox2D(verts, text, box, cellHeight, tint, cellAspectRatio, alignment, OVERRUN, maxGlyphsToDraw);

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

    for (char const& c : text)
    {
        int const   glyphIndex  = static_cast<unsigned char>(c);
        float const glyphAspect = GetGlyphAspect(glyphIndex);

        totalWidth += cellHeight * glyphAspect * cellAspectRatio;
    }

    return totalWidth;
}

//----------------------------------------------------------------------------------------------------
float BitmapFont::GetGlyphAspect(int const glyphUnicode) const
{
    UNUSED(glyphUnicode)
    return m_fontDefaultAspect;
}
