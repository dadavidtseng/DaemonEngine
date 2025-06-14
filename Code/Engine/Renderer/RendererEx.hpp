//----------------------------------------------------------------------------------------------------
// Renderer.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once

#include <vector>

#include "Engine/Core/Rgba8.hpp"
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
class WindowEx;
struct IntVec2;
struct ID3D11RasterizerState;
struct ID3D11RenderTargetView;
struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGISwapChain;
struct ID3D11BlendState;
struct IWICImagingFactory;

#define DX_SAFE_RELEASE(dxObject) \
if ((dxObject) != nullptr) {    \
dxObject->Release();      \
dxObject = nullptr;       \
}

#if defined(OPAQUE)
#undef OPAQUE
#endif


//----------------------------------------------------------------------------------------------------
class RendererEx
{
public:


    //----------------------------------------------------------------------------------------------------
    enum class eSamplerMode : int8_t
    {
        POINT_CLAMP,
        BILINEAR_CLAMP,
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
    struct sRenderExConfig
    {
        WindowEx* m_window = nullptr;
    };

    explicit RendererEx(sRenderExConfig const& config);

    static int k_lightConstantSlot;
    static int k_cameraConstantSlot;
    static int k_modelConstantsSlot;


    void CreateDeviceAndSwapChain();
    void CreateRenderTargetView();
    void CreateDepthStencilTextureAndView();
    
    void CreateSamplerState();
    void Startup();
    void BeginFrame() const;
    void EndFrame() const;
    void Shutdown();

    void ClearScreen(Rgba8 const& clearColor) const;
    // void BeginCamera(Camera const& camera) const;
    void EndCamera(Camera const& camera);

    void SetSamplerMode(eSamplerMode mode);



    void    RenderViewportToWindow(WindowEx const& window) const;
    void    RenderTestTexture();
    void    RenderWindows(std::vector<WindowEx*> const& windows);
    void    RenderWindows(WindowEx& windows);
    void    UpdateWindows(std::vector<WindowEx*> const& windows);
    void    UpdateWindows(WindowEx& windows);
    HRESULT CreateSceneRenderTexture();
    HRESULT CreateStagingTexture();
    HRESULT CreateTestTexture(const wchar_t* imageFile = nullptr);
    HRESULT CreateShaders();
    HRESULT CreateVertexBuffer();
    HRESULT CreateSampler();
    HRESULT LoadImageFromFile(const wchar_t* filename, ID3D11Texture2D** texture, ID3D11ShaderResourceView** srv) const;
    void    DebugSaveSceneTexture();

private:

    sRenderExConfig m_config;

protected:
    // Create variables to store DirectX state.
    IDXGISwapChain*         m_swapChain        = nullptr;
    ID3D11Device*           m_device           = nullptr;
    ID3D11DeviceContext*    m_deviceContext    = nullptr;
    ID3D11RenderTargetView* m_renderTargetView = nullptr;

    ID3D11Texture2D*          m_sceneTexture            = nullptr;
    ID3D11RenderTargetView*   m_sceneRenderTargetView   = nullptr;
    ID3D11ShaderResourceView* m_sceneShaderResourceView = nullptr;

    ID3D11Texture2D*          m_stagingTexture         = nullptr;
    ID3D11Texture2D*          m_testTexture            = nullptr;
    ID3D11ShaderResourceView* m_testShaderResourceView = nullptr;


    int virtualScreenWidth;
    int virtualScreenHeight;

    BITMAPINFO        bitmapInfo;
    std::vector<BYTE> pixelData;
    UINT              sceneWidth  = 1920;
    UINT              sceneHeight = 1080;

    ID3D11VertexShader* vertexShader = nullptr;
    ID3D11PixelShader*  pixelShader  = nullptr;
    ID3D11Buffer*       vertexBuffer = nullptr;
    ID3D11Buffer*       indexBuffer  = nullptr;
    ID3D11InputLayout*  inputLayout  = nullptr;
    ID3D11SamplerState* sampler      = nullptr;

    IWICImagingFactory* m_wicFactory = nullptr;

    //----------------------------------------------------------------------------------------------------



    ID3D11Texture2D*         m_depthStencilTexture                                     = nullptr;
    ID3D11DepthStencilView*  m_depthStencilDSV                                         = nullptr;

    ID3D11DepthStencilState* m_depthStencilState                                       = nullptr;


    eSamplerMode        m_desiredSamplerMode                                   = eSamplerMode::POINT_CLAMP;
    ID3D11SamplerState* m_samplerState                                         = nullptr;
    ID3D11SamplerState* m_samplerStates[static_cast<int>(eSamplerMode::COUNT)] = {};

    ID3D11RasterizerState* m_rasterizerState                                            = nullptr;


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
