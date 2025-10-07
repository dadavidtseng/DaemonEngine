//----------------------------------------------------------------------------------------------------
// BitmapFont.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Renderer/VertexUtils.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"

//-Forward-Declaration--------------------------------------------------------------------------------
class Texture;

//----------------------------------------------------------------------------------------------------
enum class eTextBoxMode : int8_t
{
    SHRINK_TO_FIT,
    OVERRUN
};

//----------------------------------------------------------------------------------------------------
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
    float GetGlyphAspect(int glyphUnicode) const;   // For now this will always return m_fontDefaultAspect

    String      m_fontFilePathNameWithNoExtension;
    SpriteSheet m_fontGlyphsSpriteSheet;
    float       m_fontDefaultAspect = 1.f;  // For basic (tier 1) fonts, set this to the aspect of the sprite sheet texture

    Texture* m_ownedTexture = nullptr;  // Texture owned by this BitmapFont (nullptr if not owned)

    // Leak tracking
    static int s_totalCreated;
    static int s_totalDeleted;
};
