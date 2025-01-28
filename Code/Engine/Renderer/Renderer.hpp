//----------------------------------------------------------------------------------------------------
// Renderer.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#define DX_SAFE_RELEASE
#include <d3d11.h>
#include <vector>

#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Game/EngineBuildPreferences.hpp"

//----------------------------------------------------------------------------------------------------
class BitmapFont;
struct IntVec2;
class Window;

//----------------------------------------------------------------------------------------------------
enum class BlendMode
{
    ALPHA,
    ADDITIVE,
};

//----------------------------------------------------------------------------------------------------
struct RenderConfig
{
    Window* m_window = nullptr;
};

//----------------------------------------------------------------------------------------------------
class Renderer
{
public:
    explicit Renderer(RenderConfig const& render_config);

    void Startup();
    void BeginFrame();
    void EndFrame() const;
    void Shutdown();

    void ClearScreen(Rgba8 const& clearColor);
    void BeginCamera(Camera const& camera);
    void EndCamera(Camera const& camera);
    void DrawVertexArray(int numVertexes, Vertex_PCU const* vertexes);

    void BindTexture(Texture const* texture);
    void DrawTexturedQuad(AABB2 const& bounds, Texture const* texture, Rgba8 const& tint, float uniformScaleXY, float rotationDegreesAboutZ);

    Texture*    CreateOrGetTextureFromFile(char const* imageFilePath);
    BitmapFont* CreateOrGetBitmapFontFromFile(char const* bitmapFontFilePathWithNoExtension);
    void        SetBlendMode(BlendMode mode);

private:
    Texture*    GetTextureForFileName(char const* imageFilePath) const;
    BitmapFont* GetBitMapFontForFileName(const char* bitmapFontFilePathWithNoExtension) const;
    // void        CreateRenderingContext();
    Texture* CreateTextureFromFile(char const* imageFilePath);
    Texture* CreateTextureFromData(char const* name, IntVec2 const& dimensions, int bytesPerTexel, uint8_t const* texelData);

    RenderConfig             m_config;
    void*                    m_apiRenderingContext = nullptr;
    std::vector<Texture*>    m_loadedTextures;
    std::vector<BitmapFont*> m_loadedFonts;

protected:
    // Create variables to store DirectX state.
    ID3D11RasterizerState*  m_rasterizerState  = nullptr;
    ID3D11RenderTargetView* m_renderTargetView = nullptr;
    ID3D11Device*           m_device           = nullptr;
    ID3D11DeviceContext*    m_deviceContext    = nullptr;
    IDXGISwapChain*         m_swapChain        = nullptr;

#if defined(ENGINE_DEBUG_RENDER)
    void* m_dxgiDebug       = nullptr;
    void* m_dxgiDebugModule = nullptr;
#endif
};
