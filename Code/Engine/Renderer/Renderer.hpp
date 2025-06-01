//----------------------------------------------------------------------------------------------------
// Renderer.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once

#include <vector>

#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Renderer/IndexBuffer.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Game/EngineBuildPreferences.hpp"

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
enum class eVertexType : int8_t
{
    VERTEX_PCU,
    VERTEX_PCUTBN,
    COUNT
};

//----------------------------------------------------------------------------------------------------
enum class eBlendMode : int8_t
{
    OPAQUE,
    ALPHA,
    ADDITIVE,
    COUNT
};

//----------------------------------------------------------------------------------------------------
enum class eDepthMode : int8_t
{
    DISABLED,
    READ_ONLY_ALWAYS,
    READ_ONLY_LESS_EQUAL,
    READ_WRITE_LESS_EQUAL,
    COUNT
};

//----------------------------------------------------------------------------------------------------
enum class eSamplerMode : int8_t
{
    POINT_CLAMP,
    BILINEAR_CLAMP,
    COUNT
};

//----------------------------------------------------------------------------------------------------
enum class eRasterizerMode : int8_t
{
    SOLID_CULL_NONE,
    SOLID_CULL_BACK,
    WIREFRAME_CULL_NONE,
    WIREFRAME_CULL_BACK,
    COUNT
};

//----------------------------------------------------------------------------------------------------
struct sCameraConstants
{
    Mat44 WorldToCameraTransform;       // View transform
    Mat44 CameraToRenderTransform;      // Non-standard transform from game to DirectX conventions;
    Mat44 RenderToClipTransform;        // Projection transform
};

//----------------------------------------------------------------------------------------------------
struct sLightConstants
{
    float SunDirection[3];
    float SunIntensity;
    float AmbientIntensity;
    float padding[3];
};

//----------------------------------------------------------------------------------------------------
struct sModelConstants
{
    Mat44 ModelToWorldTransform;
    float ModelColor[4];
};

//----------------------------------------------------------------------------------------------------
struct sRenderConfig
{
    Window* m_window = nullptr;
};

//----------------------------------------------------------------------------------------------------
class Renderer
{
public:
    explicit Renderer(sRenderConfig const& config);

    static int k_lightConstantSlot;
    static int k_cameraConstantSlot;
    static int k_modelConstantsSlot;

    void Startup();
    void BeginFrame() const;
    void EndFrame() const;
    void Shutdown();

    void ClearScreen(Rgba8 const& clearColor) const;
    void BeginCamera(Camera const& camera) const;
    void EndCamera(Camera const& camera);

    void DrawVertexArray(int numVertexes, Vertex_PCU const* vertexes);
    void DrawVertexArray(int numVertexes, Vertex_PCUTBN const* vertexes);
    void DrawVertexArray(VertexList_PCU const& verts);
    void DrawVertexArray(VertexList_PCUTBN const& verts);
    void DrawVertexArray(VertexList_PCU const& verts, std::vector<unsigned int> const& indexes);
    void DrawVertexArray(VertexList_PCUTBN const& verts, std::vector<unsigned int> const& indexes);

    void BindShader(Shader const* shader) const;
    void BindTexture(Texture const* texture) const;
    void DrawTexturedQuad(AABB2 const& bounds, Texture const* texture, Rgba8 const& tint, float uniformScaleXY, float rotationDegreesAboutZ);

    Image       CreateImageFromFile(char const* imageFilePath);
    BitmapFont* CreateOrGetBitmapFontFromFile(char const* bitmapFontFilePathWithNoExtension);
    Shader*     CreateOrGetShaderFromFile(char const* shaderFilePath, eVertexType vertexType = eVertexType::VERTEX_PCU);
    Texture*    CreateOrGetTextureFromFile(char const* imageFilePath);

    void            SetBlendMode(eBlendMode mode);
    void            SetDepthMode(eDepthMode mode);
    void            SetSamplerMode(eSamplerMode mode);
    void            SetRasterizerMode(eRasterizerMode mode);
    void            SetLightConstants(Vec3 const& sunDirection, float sunIntensity, float ambientIntensity) const;
    void            SetModelConstants(Mat44 const& modelToWorldTransform = Mat44(), Rgba8 const& modelColor = Rgba8::WHITE) const;
    ConstantBuffer* CreateConstantBuffer(unsigned int size) const;
    IndexBuffer*    CreateIndexBuffer(unsigned int size, unsigned int stride) const;
    VertexBuffer*   CreateVertexBuffer(unsigned int size, unsigned int stride) const;

private:
    Texture*    GetTextureForFileName(char const* imageFilePath) const;
    BitmapFont* GetBitMapFontForFileName(const char* bitmapFontFilePathWithNoExtension) const;
    Shader*     GetShaderForFileName(char const* shaderFilePath) const;

    Texture* CreateTextureFromFile(char const* imageFilePath);
    Texture* CreateTextureFromData(char const* name, IntVec2 const& dimensions, int bytesPerTexel, uint8_t const* texelData);
    Texture* CreateTextureFromImage(Image const& image);

    Shader* CreateShader(char const* shaderName, char const* shaderSource, eVertexType vertexType = eVertexType::VERTEX_PCU);
    Shader* CreateShader(char const* shaderName, eVertexType vertexType = eVertexType::VERTEX_PCU);
    bool    CompileShaderToByteCode(std::vector<unsigned char>& out_byteCode, char const* name, char const* source, char const* entryPoint, char const* target);

    void DrawVertexBuffer(VertexBuffer const* vbo, unsigned int vertexCount);
    void DrawIndexedVertexBuffer(VertexBuffer const* vbo, IndexBuffer const* ibo, unsigned int indexCount);

    void CopyCPUToGPU(void const* data, unsigned int size, ConstantBuffer* cbo) const;
    void CopyCPUToGPU(void const* data, unsigned int size, IndexBuffer* ibo) const;
    void CopyCPUToGPU(void const* data, unsigned int size, VertexBuffer* vbo) const;
    void BindConstantBuffer(int slot, ConstantBuffer const* cbo) const;
    void BindIndexBuffer(IndexBuffer const* ibo) const;
    void BindVertexBuffer(VertexBuffer const* vbo) const;
    void SetStatesIfChanged();

    sRenderConfig m_config;

protected:
    // Create variables to store DirectX state.
    IDXGISwapChain*         m_swapChain        = nullptr;
    ID3D11Device*           m_device           = nullptr;
    ID3D11DeviceContext*    m_deviceContext    = nullptr;
    ID3D11RenderTargetView* m_renderTargetView = nullptr;

    eBlendMode        m_desiredBlendMode                                 = eBlendMode::ALPHA;
    ID3D11BlendState* m_blendState                                       = nullptr;
    ID3D11BlendState* m_blendStates[static_cast<int>(eBlendMode::COUNT)] = {};

    ID3D11Texture2D*         m_depthStencilTexture                                     = nullptr;
    ID3D11DepthStencilView*  m_depthStencilDSV                                         = nullptr;
    eDepthMode               m_desiredDepthMode                                        = eDepthMode::READ_WRITE_LESS_EQUAL;
    ID3D11DepthStencilState* m_depthStencilState                                       = nullptr;
    ID3D11DepthStencilState* m_depthStencilStates[static_cast<int>(eDepthMode::COUNT)] = {};

    eSamplerMode        m_desiredSamplerMode                                   = eSamplerMode::POINT_CLAMP;
    ID3D11SamplerState* m_samplerState                                         = nullptr;
    ID3D11SamplerState* m_samplerStates[static_cast<int>(eSamplerMode::COUNT)] = {};

    eRasterizerMode        m_desiredRasterizerMode                                      = eRasterizerMode::SOLID_CULL_BACK;
    ID3D11RasterizerState* m_rasterizerState                                            = nullptr;
    ID3D11RasterizerState* m_rasterizerStates[static_cast<int>(eRasterizerMode::COUNT)] = {};

    VertexBuffer*   m_immediateVBO_PCU    = nullptr;
    VertexBuffer*   m_immediateVBO_PCUTBN = nullptr;
    IndexBuffer*    m_immediateIBO        = nullptr;
    ConstantBuffer* m_cameraCBO           = nullptr;
    ConstantBuffer* m_lightCBO            = nullptr;
    ConstantBuffer* m_modelCBO            = nullptr;

    Texture* m_defaultTexture = nullptr;
    Shader*  m_defaultShader  = nullptr;
    Shader*  m_currentShader  = nullptr;

    std::vector<BitmapFont*> m_loadedFonts;
    std::vector<Shader*>     m_loadedShaders;
    std::vector<Texture*>    m_loadedTextures;

#if defined(ENGINE_DEBUG_RENDER)
    void* m_dxgiDebug       = nullptr;
    void* m_dxgiDebugModule = nullptr;
#endif
};
