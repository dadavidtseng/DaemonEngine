//----------------------------------------------------------------------------------------------------
// BitmapFont.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Renderer/VertexUtils.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"

#include <cstdint>
#include <map>
#include <vector>

//-Forward-Declaration--------------------------------------------------------------------------------
class Texture;

//----------------------------------------------------------------------------------------------------
enum class eTextBoxMode : int8_t
{
    SHRINK_TO_FIT,
    OVERRUN
};

//----------------------------------------------------------------------------------------------------
enum class eFontTier : int8_t
{
    TIER_1,     // Fixed-width 16x16 grid (SD1 style)
    TIER_2,     // Proportional auto-width (pixel scanning)
    TIER_3,     // BMFont metadata with kerning
    TIER_4,     // SDF threshold shader
    TIER_5,     // Custom VertexFont + effects shader
};

//----------------------------------------------------------------------------------------------------
struct sGlyphData
{
    int   m_id        = 0;      // Character code point (ASCII/Unicode)
    float m_uvMinsX   = 0.f;   // Texture UV left
    float m_uvMinsY   = 0.f;   // Texture UV bottom
    float m_uvMaxsX   = 0.f;   // Texture UV right
    float m_uvMaxsY   = 0.f;   // Texture UV top
    float m_width     = 0.f;   // Glyph width in pixels
    float m_height    = 0.f;   // Glyph height in pixels
    float m_xOffset   = 0.f;   // X offset when drawing
    float m_yOffset   = 0.f;   // Y offset when drawing
    float m_xAdvance  = 0.f;   // Cursor advance after this glyph
    int   m_page      = 0;     // Texture page index
};

class BitmapFont
{
    friend class Renderer;  // Only the Renderer can create new BitmapFont objects!
    friend class FontLoader;  // FontLoader can also create BitmapFont objects

public:
    ~BitmapFont();  // Destructor to clean up owned texture

    Texture const& GetTexture() const;
    void           AddVertsForText2D(VertexList_PCU& verts, String const& text, Vec2 const& textMins, float cellHeight, Rgba8 const& tint = Rgba8::WHITE, float cellAspectRatio = 1.f) const;
    void           AddVertsForTextInBox2D(VertexList_PCU& verts, String const& text, AABB2 const& box, float cellHeight, Rgba8 const& tint = Rgba8::WHITE, float cellAspectRatio = 1.f, Vec2 const& alignment = Vec2::ZERO, eTextBoxMode mode = eTextBoxMode::SHRINK_TO_FIT, int maxGlyphsToDraw = INT_MAX) const;
    void           AddVertsForText3DAtOriginXForward(VertexList_PCU& verts, String const& text, float cellHeight, Rgba8 const& tint = Rgba8::WHITE, float cellAspectRatio = 1.f, Vec2 const& alignment = Vec2(0.5f, 0.5f), int maxGlyphsToDraw = INT_MAX) const;

    float GetTextWidth(float cellHeight, String const& text, float cellAspectRatio = 1.f) const;

    eFontTier GetFontTier() const;
    bool      IsSDF() const;

    // Tier 5: VertexFont output overloads
    void AddVertsForText2D(VertexList_Font& verts, String const& text, Vec2 const& textMins,
        float cellHeight, Rgba8 const& tint = Rgba8::WHITE, float cellAspectRatio = 1.f) const;
    void AddVertsForTextInBox2D(VertexList_Font& verts, String const& text, AABB2 const& box,
        float cellHeight, Rgba8 const& tint = Rgba8::WHITE, float cellAspectRatio = 1.f,
        Vec2 const& alignment = Vec2::ZERO, eTextBoxMode mode = eTextBoxMode::SHRINK_TO_FIT,
        int maxGlyphsToDraw = INT_MAX) const;

    // Leak tracking - static counters
    static int  GetAliveCount();
    static int  GetTotalCreated();
    static int  GetTotalDeleted();
    static void ReportLeakStatus();

private:
    BitmapFont(char const* fontFilePathNameWithNoExtension, Texture const& fontTexture, IntVec2 const& spriteCoords);

    // New constructor that takes ownership of the texture
    BitmapFont(char const* fontFilePathNameWithNoExtension, Texture* fontTexture, IntVec2 const& spriteCoords, bool ownsTexture);

protected:
    float           GetGlyphAspect(int glyphUnicode) const;
    float           GetKerningAmount(int firstChar, int secondChar) const;
    sGlyphData const* GetGlyphData(int glyphUnicode) const;

public:
    void            ComputeAutoWidths(class Image const& image);
    bool            ParseBMFontFile(String const& fntFilePath);

protected:
    String      m_fontFilePathNameWithNoExtension;
    SpriteSheet m_fontGlyphsSpriteSheet;
    float       m_fontDefaultAspect = 1.f;

    Texture* m_ownedTexture = nullptr;

    // Tiered font data (Tier 2+)
    eFontTier                          m_fontTier       = eFontTier::TIER_1;
    std::map<int, sGlyphData>           m_glyphData;
    std::map<uint64_t, float>          m_kerningPairs;
    float                              m_lineHeight     = 0.f;
    float                              m_base           = 0.f;
    int                                m_scaleW         = 0;
    int                                m_scaleH         = 0;
    std::vector<Texture const*>        m_pageTextures;
    bool                               m_isSDF          = false;

    // Leak tracking
    static int s_totalCreated;
    static int s_totalDeleted;
};
