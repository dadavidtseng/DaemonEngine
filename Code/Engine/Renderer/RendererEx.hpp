//----------------------------------------------------------------------------------------------------
// Renderer.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include "Engine/Platform/WindowEx.hpp"

//-Forward-Declaration--------------------------------------------------------------------------------
struct ID3D11Texture2D;
struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGISwapChain;
struct ID3D11RenderTargetView;
struct ID3D11VertexShader;
struct ID3D11PixelShader;
struct ID3D11Buffer;
struct ID3D11InputLayout;
struct ID3D11SamplerState;
struct ID3D11ShaderResourceView;
struct IWICImagingFactory;
struct DriftParams;

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
    RendererEx();
    ~RendererEx();
    HRESULT Startup();
    void    EndFrame();
    HRESULT CreateDeviceAndSwapChain();
    HRESULT CreateSceneRenderTexture();
    HRESULT CreateStagingTexture();
    HRESULT CreateShaders();
    HRESULT CreateVertexBuffer();
    HRESULT CreateSampler();


    void Render();
    void RenderSceneTextureToMainWindow();
    void ReadStagingTextureToPixelData();

    void    RenderTexture(Texture* texture);
    HRESULT CreateWindowSwapChain(WindowEx& window);
    void    UpdateWindows(std::vector<WindowEx>& windows);

private:
    void     RenderViewportToWindow(WindowEx const& window) const;
    void     RenderViewportToWindowDX11(const WindowEx& window) const;
    void     Cleanup();
    Texture* CreateOrGetTextureFromFile(char const* imageFilePath);
    Texture* GetTextureForFileName(char const* imageFilePath) const;
    Texture* CreateTextureFromFile(char const* imageFilePath);
    Texture* CreateTextureFromImage(Image const& image);

    HRESULT ResizeWindowSwapChain(WindowEx& window);

    ID3D11Device*           m_device                         = nullptr;
    ID3D11DeviceContext*    m_deviceContext                  = nullptr;
    IDXGISwapChain*         m_mainSwapChain                  = nullptr;
    ID3D11RenderTargetView* m_mainBackBufferRenderTargetView = nullptr;

    Texture*                m_sceneTexture          = nullptr;
    Texture*                m_stagingTexture        = nullptr;
    ID3D11RenderTargetView* m_sceneRenderTargetView = nullptr;

    ID3D11VertexShader* vertexShader = nullptr;
    ID3D11PixelShader*  pixelShader  = nullptr;
    ID3D11Buffer*       vertexBuffer = nullptr;
    ID3D11Buffer*       indexBuffer  = nullptr;
    ID3D11InputLayout*  inputLayout  = nullptr;
    ID3D11SamplerState* sampler      = nullptr;

    int sceneWidth = 1920, sceneHeight = 1080;

    BITMAPINFO        m_bitmapInfo;     // The BITMAPINFO structure defines the dimensions and color information for a DIB.
    std::vector<BYTE> m_pixelData;

    VertexList_PCU        m_vertexList;
    std::vector<Texture*> m_loadedTextures;
    Texture*              m_defaultTexture = nullptr;
};
