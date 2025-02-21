//----------------------------------------------------------------------------------------------------
// Renderer.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once

#include <vector>

#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Game/EngineBuildPreferences.hpp"

class ModelBuffer;
//-Forward-Declaration--------------------------------------------------------------------------------
class BitmapFont;
class ConstantBuffer;
class Image;
class Shader;
class VertexBuffer;
class Window;
struct IntVec2;
struct ID3D11RasterizerState;
struct ID3D11RenderTargetView;
struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGISwapChain;
struct ID3D11BlendState;

#define DX_SAFE_RELEASE(dxObject) \
if ((dxObject) != nullptr) {    \
dxObject->Release();      \
dxObject = nullptr;       \
}

#if defined(OPAQUE)
#undef OPAQUE
#endif

//----------------------------------------------------------------------------------------------------
enum class DepthMode
{
    DISABLED,
    READ_ONLY_ALWAYS,
    READ_ONLY_LESS_EQUAL,
    READ_WRITE_LESS_EQUAL,
    COUNT
};

//----------------------------------------------------------------------------------------------------
enum class RasterizerMode
{
    SOLID_CULL_NONE,
    SOLID_CULL_BACK,
    WIREFRAME_CULL_NONE,
    WIREFRAME_CULL_BACK,
    COUNT
};

//----------------------------------------------------------------------------------------------------
enum class SamplerMode
{
    POINT_CLAMP,
    BILINEAR_CLAMP,
    COUNT
};

//----------------------------------------------------------------------------------------------------
enum class BlendMode
{
    OPAQUE,
    ALPHA,
    ADDITIVE,
    COUNT
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
    void BeginFrame() const;
    void EndFrame() const;
    void Shutdown();

    void ClearScreen(Rgba8 const& clearColor) const;
    void BeginCamera(Camera const& camera) const;
    void EndCamera(Camera const& camera);
    void DrawVertexArray(int numVertexes, Vertex_PCU const* vertexes);

    void BindTexture(Texture const* texture) const;
    void DrawTexturedQuad(AABB2 const& bounds, Texture const* texture, Rgba8 const& tint, float uniformScaleXY, float rotationDegreesAboutZ);

    Texture*    CreateOrGetTextureFromFile(char const* imageFilePath);
    BitmapFont* CreateOrGetBitmapFontFromFile(char const* bitmapFontFilePathWithNoExtension);

    void SetBlendMode(BlendMode mode);
    void SetModelConstants(Mat44 const& modelToWorldTransform = Mat44(), Rgba8 const& modelColor = Rgba8::WHITE) const;

private:
    Texture*    GetTextureForFileName(char const* imageFilePath) const;
    BitmapFont* GetBitMapFontForFileName(const char* bitmapFontFilePathWithNoExtension) const;
    Texture*    CreateTextureFromFile(char const* imageFilePath);
    Texture*    CreateTextureFromData(char const* name, IntVec2 const& dimensions, int bytesPerTexel, uint8_t const* texelData);
    Texture*    CreateTextureFromImage(Image const& image);
    Image       CreateImageFromFile(char const* imageFilePath);

    Shader* CreateShader(char const* shaderName, char const* shaderSource);
    Shader* CreateShader(char const* shaderName);
    bool    CompileShaderToByteCode(std::vector<unsigned char>& out_byteCode, char const* name, char const* source, char const* entryPoint, char const* target);
    void    BindShader(Shader const* shader) const;
    void    DrawVertexBuffer(VertexBuffer const* vbo, unsigned int vertexCount) const;

    VertexBuffer*   CreateVertexBuffer(unsigned int size, unsigned int stride) const;
    void            CopyCPUToGPU(void const* data, unsigned int size, VertexBuffer* vbo) const;
    void            BindVertexBuffer(VertexBuffer const* vbo) const;
    ConstantBuffer* CreateConstantBuffer(unsigned int size) const;
    ModelBuffer*    CreateModelBuffer(unsigned int size) const;
    void            CopyCPUToGPU(void const* data, unsigned int size, ConstantBuffer* cbo) const;
    void            CopyCPUToGPU(void const* data, unsigned int size, ModelBuffer* cbo) const;
    void            BindConstantBuffer(int slot, ConstantBuffer const* cbo) const;
    void            BindModelBuffer(int slot, ModelBuffer const* cbo) const;
    void            SetStatesIfChanged();
    void            SetSamplerMode(SamplerMode mode);
    void            SetRasterizerMode(RasterizerMode mode);
    void            SetDepthMode(DepthMode mode);

    RenderConfig m_config;
    // void*                    m_apiRenderingContext = nullptr;

protected:
    // Create variables to store DirectX state.
    ID3D11RenderTargetView*  m_renderTargetView                         = nullptr;
    ID3D11Device*            m_device                                   = nullptr;
    ID3D11DeviceContext*     m_deviceContext                            = nullptr;
    IDXGISwapChain*          m_swapChain                                = nullptr;
    Shader*                  m_currentShader                            = nullptr;
    VertexBuffer*            m_immediateVBO                             = nullptr;
    Shader*                  m_defaultShader                            = nullptr;
    ConstantBuffer*          m_cameraCBO                                = nullptr;
    ModelBuffer*             m_modelCBO                                 = nullptr;
    ID3D11BlendState*        m_blendState                               = nullptr;
    BlendMode                m_desiredBlendMode                         = BlendMode::ALPHA;
    ID3D11BlendState*        m_blendStates[(int)(BlendMode::COUNT)]     = {};
    Texture*                 m_defaultTexture                           = nullptr;
    SamplerMode              m_desiredSamplerMode                       = SamplerMode::POINT_CLAMP;
    ID3D11SamplerState*      m_samplerState                             = nullptr;
    ID3D11SamplerState*      m_samplerStates[(int)(SamplerMode::COUNT)] = {};
    std::vector<Shader*>     m_loadedShaders;
    std::vector<Texture*>    m_loadedTextures;
    std::vector<BitmapFont*> m_loadedFonts;
    RasterizerMode           m_desiredRasterizerMode                          = RasterizerMode::SOLID_CULL_BACK;
    ID3D11RasterizerState*   m_rasterizerState                                = nullptr;
    ID3D11RasterizerState*   m_rasterizerStates[(int)(RasterizerMode::COUNT)] = {};
    ID3D11Texture2D*         m_depthStencilTexture                            = nullptr;
    ID3D11DepthStencilView*  m_depthStencilDSV                                = nullptr;
    DepthMode                m_desiredDepthMode                               = DepthMode::READ_WRITE_LESS_EQUAL;
    ID3D11DepthStencilState* m_depthStencilState                              = nullptr;
    ID3D11DepthStencilState* m_depthStencilStates[(int)(DepthMode::COUNT)]    = {};

#if defined(ENGINE_DEBUG_RENDER)
    void* m_dxgiDebug       = nullptr;
    void* m_dxgiDebugModule = nullptr;
#endif
};
