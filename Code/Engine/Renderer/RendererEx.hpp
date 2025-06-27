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
    void    Startup();
    HRESULT Initialize();
    HRESULT LoadImageFromFile(wchar_t const* filename, ID3D11Texture2D** texture, ID3D11ShaderResourceView** srv) const;
    void    SetWindowDriftParams(HWND const hwnd, DriftParams const& params);
    void    StartDragging(HWND const hwnd, POINT const& mousePos);
    void    StopDragging(HWND hwnd);
    void    UpdateDragging(HWND const hwnd, POINT const& mousePos) const;
    void    UpdateWindowDrift(WindowEx& window) const;
    // HRESULT AddWindow(HWND const& hwnd);
    void    UpdateWindowPosition(WindowEx& window) const;
    void    Render(std::vector<WindowEx> windows);
    HRESULT CreateDeviceAndSwapChain();
    HRESULT CreateSceneRenderTexture();
    HRESULT CreateStagingTexture();
    HRESULT CreateTestTexture(const wchar_t* imageFile = nullptr);
    HRESULT CreateShaders();
    HRESULT CreateVertexBuffer();
    HRESULT CreateSampler();

    float GetSceneWidth();
    float GetSceneHeight();

private:
    void                    RenderTestTexture() const;
    void                    UpdateWindows(std::vector<WindowEx>& windows);
    void                    RenderViewportToWindow(WindowEx const& window) const;
    void                    Cleanup();
    ID3D11Device*           m_device                         = nullptr;
    ID3D11DeviceContext*    m_deviceContext                  = nullptr;
    IDXGISwapChain*         m_mainSwapChain                  = nullptr;
    ID3D11RenderTargetView* m_mainBackBufferRenderTargetView = nullptr;

    ID3D11Texture2D*          m_sceneTexture            = nullptr;
    ID3D11RenderTargetView*   m_sceneRenderTargetView   = nullptr;
    ID3D11ShaderResourceView* m_sceneShaderResourceView = nullptr;

    ID3D11Texture2D* m_stagingTexture = nullptr;

    ID3D11Texture2D*          m_testTexture            = nullptr;
    ID3D11ShaderResourceView* m_testShaderResourceView = nullptr;

    ID3D11VertexShader* vertexShader = nullptr;
    ID3D11PixelShader*  pixelShader  = nullptr;
    ID3D11Buffer*       vertexBuffer = nullptr;
    ID3D11Buffer*       indexBuffer  = nullptr;
    ID3D11InputLayout*  inputLayout  = nullptr;
    ID3D11SamplerState* sampler      = nullptr;


    float sceneWidth = 1920, sceneHeight = 1080;
    HWND  mainWindow = nullptr;

    BITMAPINFO        m_bitmapInfo;     // The BITMAPINFO structure defines the dimensions and color information for a DIB.
    std::vector<BYTE> pixelData;


    IWICImagingFactory* m_wicFactory = nullptr;
};
