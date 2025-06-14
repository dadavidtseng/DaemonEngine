//----------------------------------------------------------------------------------------------------
// RendererEx.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Renderer/RendererEx.hpp"

#include <comdef.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <dxgi.h>
#include <wincodec.h>

#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/Image.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Platform/WindowEx.hpp"
#include "Engine/Renderer/ConstantBuffer.hpp"
#include "Engine/Renderer/Shader.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"
#include "ThirdParty/stb/stb_image.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "windowscodecs.lib")

using namespace DirectX;

// Define ENGINE_DEBUG_RENDERER if this is a Debug configuration and link debug libraries if it is.
#if defined(_DEBUG)
#define ENGINE_DEBUG_RENDER
#endif

#if defined(ENGINE_DEBUG_RENDER)
#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")
#endif

// 簡單的頂點結構
struct Vertex
{
    XMFLOAT3 m_position;
    XMFLOAT2 m_texCoord;
};

//----------------------------------------------------------------------------------------------------
RendererEx::RendererEx(sRenderExConfig const& config)
{
    m_config = config;

    virtualScreenWidth  = GetSystemMetrics(SM_CXSCREEN);
    virtualScreenHeight = GetSystemMetrics(SM_CYSCREEN);

    ZeroMemory(&bitmapInfo, sizeof(BITMAPINFO));
    bitmapInfo.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biWidth       = sceneWidth;
    bitmapInfo.bmiHeader.biHeight      = -static_cast<LONG>(sceneHeight);
    bitmapInfo.bmiHeader.biPlanes      = 1;
    bitmapInfo.bmiHeader.biBitCount    = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;

    pixelData.resize(sceneWidth * sceneHeight * 4);
}



//----------------------------------------------------------------------------------------------------
/// @brief
/// Create device and swap chain.
void RendererEx::CreateDeviceAndSwapChain()
{
    WindowEx const*      windowEx      = m_config.m_window;
    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferCount          = 1;
    swapChainDesc.BufferDesc.Width     = sceneWidth;
    swapChainDesc.BufferDesc.Height    = sceneHeight;
    swapChainDesc.BufferDesc.Format    = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage          = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.OutputWindow         = (HWND)windowEx->GetWindowHandle();
    swapChainDesc.SampleDesc.Count     = 1;
    swapChainDesc.Windowed             = TRUE;

    HRESULT const hr = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        D3D11_CREATE_DEVICE_DEBUG,
        nullptr,
        0,
        D3D11_SDK_VERSION,
        &swapChainDesc,
        &m_swapChain,
        &m_device,
        nullptr,
        &m_deviceContext
    );

    if (FAILED(hr))ERROR_AND_DIE("Could not create D3D 11 device and swap chain.")
}

//----------------------------------------------------------------------------------------------------
/// @brief
/// Get back buffer texture.
void RendererEx::CreateRenderTargetView()
{
    ID3D11Texture2D* backBuffer;

    HRESULT hr = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer));
    if (FAILED(hr)) ERROR_AND_DIE("Could not get swap chain buffer.")

    hr = m_device->CreateRenderTargetView(backBuffer, nullptr, &m_renderTargetView);
    if (FAILED(hr)) ERROR_AND_DIE("Could create render target view for swap chain buffer.")

    backBuffer->Release();
}

//----------------------------------------------------------------------------------------------------
/// @brief
/// Create depth stencil texture and view.
void RendererEx::CreateDepthStencilTextureAndView()
{
    D3D11_TEXTURE2D_DESC depthTextureDesc = {};
    depthTextureDesc.Width                = sceneWidth;
    depthTextureDesc.Height               = sceneHeight;
    depthTextureDesc.MipLevels            = 1;
    depthTextureDesc.ArraySize            = 1;
    depthTextureDesc.Usage                = D3D11_USAGE_DEFAULT;
    depthTextureDesc.Format               = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthTextureDesc.BindFlags            = D3D11_BIND_DEPTH_STENCIL;
    depthTextureDesc.SampleDesc.Count     = 1;

    HRESULT hr = m_device->CreateTexture2D(&depthTextureDesc, nullptr, &m_depthStencilTexture);
    if (FAILED(hr)) ERROR_AND_DIE("Could not create texture for depth stencil.")

    hr = m_device->CreateDepthStencilView(m_depthStencilTexture, nullptr, &m_depthStencilDSV);
    if (FAILED(hr)) ERROR_AND_DIE("Could not create depth stencil view.")
}



void RendererEx::CreateSamplerState()
{
    //-Create sampler states--------------------------------------------------------------------------
    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter             = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU           = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressV           = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressW           = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.ComparisonFunc     = D3D11_COMPARISON_NEVER;
    samplerDesc.MinLOD             = 0;
    samplerDesc.MaxLOD             = D3D11_FLOAT32_MAX;

    HRESULT hr = m_device->CreateSamplerState(&samplerDesc, &m_samplerStates[static_cast<int>(eSamplerMode::POINT_CLAMP)]);

    if (FAILED(hr)) ERROR_AND_DIE("CreateSamplerState for SamplerMode::POINT_CLAMP failed.")

    samplerDesc.Filter   = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;

    hr = m_device->CreateSamplerState(&samplerDesc, &m_samplerStates[static_cast<int>(eSamplerMode::BILINEAR_CLAMP)]);
    if (FAILED(hr)) ERROR_AND_DIE("CreateSamplerState for SamplerMode::BILINEAR_CLAMP failed.")
}



//----------------------------------------------------------------------------------------------------
void RendererEx::Startup()
{
    // Create a local DXGI_SWAP_CHAIN_DESC variable and set its values as follows.
    unsigned int deviceFlags = 0;

#if defined(ENGINE_DEBUG_RENDER)
    deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    // Create the DXGI debug module in Startup, before creating the device and swap chain.
#if defined(ENGINE_DEBUG_RENDER)
    m_dxgiDebugModule = static_cast<void*>(LoadLibraryA("dxgidebug.dll"));

    if (!m_dxgiDebugModule)
    {
        ERROR_AND_DIE("Could not load dxgidebug.dll.")
    }

    typedef HRESULT (WINAPI*GetDebugModuleCB)(REFIID, void**);
    ((GetDebugModuleCB)GetProcAddress(static_cast<HMODULE>(m_dxgiDebugModule), "DXGIGetDebugInterface"))
        (__uuidof(IDXGIDebug), &m_dxgiDebug);

    if (!m_dxgiDebug)
    {
        ERROR_AND_DIE("Could not load debug module.")
    }
#endif

    //-Create device and swap chain-------------------------------------------------------------------
    // DXGI_SWAP_CHAIN_DESC swapChainDesc = {};

    // swapChainDesc.BufferDesc.Width  = window->GetClientDimensions().x;
    // swapChainDesc.BufferDesc.Height = window->GetClientDimensions().y;
    // swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    // swapChainDesc.SampleDesc.Count  = 1;
    // swapChainDesc.BufferUsage       = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    // swapChainDesc.BufferCount       = 2;
    // swapChainDesc.OutputWindow      = static_cast<HWND>(window->GetWindowHandle());
    // swapChainDesc.Windowed          = true;
    // swapChainDesc.SwapEffect        = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    //
    // HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr,
    //                                            D3D_DRIVER_TYPE_HARDWARE,
    //                                            nullptr,
    //                                            deviceFlags,
    //                                            nullptr,
    //                                            0,
    //                                            D3D11_SDK_VERSION,
    //                                            &swapChainDesc,
    //                                            &m_swapChain,
    //                                            &m_device,
    //                                            nullptr,
    //                                            &m_deviceContext);


    CreateDeviceAndSwapChain();
    CreateRenderTargetView();
    // CreateBlendState();
    // CreateDepthStencilTextureAndView();
    // CreateDepthStencilState();
    CreateSamplerState();
    // CreateRasterizerState();
    // SetDepthMode(eDepthMode::READ_WRITE_LESS_EQUAL);
    SetSamplerMode(eSamplerMode::POINT_CLAMP);


    // m_immediateVBO_PCU    = CreateVertexBuffer(sizeof(Vertex_PCU), sizeof(Vertex_PCU));
    // m_immediateVBO_PCUTBN = CreateVertexBuffer(sizeof(Vertex_PCUTBN), sizeof(Vertex_PCUTBN));
    // m_immediateIBO        = CreateIndexBuffer(sizeof(Vertex_PCU), sizeof(Vertex_PCU));
    // m_lightCBO            = CreateConstantBuffer(sizeof(sLightConstants));
    // m_cameraCBO           = CreateConstantBuffer(sizeof(sCameraConstants));
    // m_modelCBO            = CreateConstantBuffer(sizeof(sModelConstants));
    //
    // //------------------------------------------------------------------------------------------------
    // // Initialize m_defaultTexture to a 2x2 white image
    // Image const defaultImage(IntVec2(2, 2), Rgba8::WHITE);
    // m_defaultTexture         = CreateTextureFromImage(defaultImage);
    // m_defaultTexture->m_name = "Default";
    // m_defaultShader          = CreateOrGetShaderFromFile("Data/Shaders/Default", eVertexType::VERTEX_PCU);
    // m_currentShader          = CreateOrGetShaderFromFile("Data/Shaders/Default", eVertexType::VERTEX_PCU);
    // // m_currentShader = CreateShader("Default", DEFAULT_SHADER_SOURCE);
    //
    // BindShader(m_currentShader);
    // BindTexture(m_defaultTexture);

    // m_sceneTexture   = m_defaultTexture;
    // m_testTexture   = m_defaultTexture;
    // m_stagingTexture = m_defaultTexture;
    HRESULT hr = CoCreateInstance(
        CLSID_WICImagingFactory,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_IWICImagingFactory,
        reinterpret_cast<void**>(&m_wicFactory)
    );
    if (FAILED(hr))
    {
        // WIC 初始化失敗，但繼續執行（使用程序生成紋理）
        m_wicFactory = nullptr;
    }
    hr = CreateSceneRenderTexture();
    if (FAILED(hr)) return;

    hr = CreateStagingTexture();
    if (FAILED(hr)) return;
    // m_testTexture = CreateOrGetTextureFromFile("Data/Images/Butler.png");
    hr = CreateTestTexture(L"C:/p4/Personal/SD/FirstMultipleWindows/Run/Data/Images/Butler.png");
    if (FAILED(hr)) return;
    hr = CreateShaders();
    if (FAILED(hr)) return;

    hr = CreateVertexBuffer();
    if (FAILED(hr)) return;

    hr = CreateSampler();
    if (FAILED(hr)) return;
}


//----------------------------------------------------------------------------------------------------
void RendererEx::BeginFrame() const
{
    // TODO: BindDefaultRenderTarget();
    // This code needs to run every frame and should be in your Render function.
    // m_config.m_window->m_renderTargetView[0] = m_renderTargetView;
    // m_config.m_window->m_renderTargetView[1] = m_renderTargetView;
    // m_deviceContext->OMSetRenderTargets(1, &m_renderTargetView, nullptr);
    m_deviceContext->OMSetRenderTargets(1, &m_renderTargetView, m_depthStencilDSV);
    // m_deviceContext->OMSetRenderTargets(1, &m_config.m_window->m_renderTargetView[0], m_depthStencilDSV);
    // m_deviceContext->OMSetRenderTargets(1, &m_config.m_window->m_renderTargetView[1], m_depthStencilDSV);
}

//----------------------------------------------------------------------------------------------------
void RendererEx::EndFrame() const
{
    HRESULT const hr = m_swapChain->Present(0, 0);

    if (hr == DXGI_ERROR_DEVICE_REMOVED ||
        hr == DXGI_ERROR_DEVICE_RESET)
    {
        ERROR_AND_DIE("Device has been lost, application will now terminate.")
    }
}

//----------------------------------------------------------------------------------------------------
void RendererEx::Shutdown()
{
    for (int i = 0; i < static_cast<int>(m_loadedShaders.size()); ++i)
    {
        if (m_loadedShaders[i] != nullptr)
        {
            delete m_loadedShaders[i];
            m_loadedShaders[i] = nullptr;
        }
    }

    for (int i = 0; i < static_cast<int>(m_loadedTextures.size()); ++i)
    {
        if (m_loadedTextures[i] != nullptr)
        {
            delete m_loadedTextures[i];
            m_loadedTextures[i] = nullptr;
        }
    }

    m_loadedFonts.clear();
    m_loadedShaders.clear();
    m_loadedTextures.clear();

    if (m_modelCBO != nullptr)
    {
        delete m_modelCBO;
        m_modelCBO = nullptr;
    }

    if (m_lightCBO != nullptr)
    {
        delete m_lightCBO;
        m_lightCBO = nullptr;
    }

    if (m_cameraCBO != nullptr)
    {
        delete m_cameraCBO;
        m_cameraCBO = nullptr;
    }

    if (m_immediateIBO != nullptr)
    {
        delete m_immediateIBO;
        m_immediateIBO = nullptr;
    }

    if (m_immediateVBO_PCUTBN != nullptr)
    {
        delete m_immediateVBO_PCUTBN;
        m_immediateVBO_PCUTBN = nullptr;
    }

    if (m_immediateVBO_PCU != nullptr)
    {
        delete m_immediateVBO_PCU;
        m_immediateVBO_PCU = nullptr;
    }



    // Release all sampler states
    for (int i = 0; i < static_cast<int>(eSamplerMode::COUNT); ++i)
    {
        DX_SAFE_RELEASE(m_samplerStates[i])
    }


    DX_SAFE_RELEASE(m_depthStencilDSV)
    DX_SAFE_RELEASE(m_depthStencilTexture)


    DX_SAFE_RELEASE(m_renderTargetView)
    DX_SAFE_RELEASE(m_swapChain)
    DX_SAFE_RELEASE(m_deviceContext)
    DX_SAFE_RELEASE(m_device)


    // Report error leaks and release debug module
#if defined(ENGINE_DEBUG_RENDER)
    if (m_dxgiDebug)
    {
        constexpr DXGI_DEBUG_RLO_FLAGS flags = static_cast<DXGI_DEBUG_RLO_FLAGS>(DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL);
        static_cast<IDXGIDebug*>(m_dxgiDebug)->ReportLiveObjects(DXGI_DEBUG_ALL, flags);

        static_cast<IDXGIDebug*>(m_dxgiDebug)->Release();
        m_dxgiDebug = nullptr;
    }

    if (m_dxgiDebugModule)
    {
        FreeLibrary(static_cast<HMODULE>(m_dxgiDebugModule));
        m_dxgiDebugModule = nullptr;
    }
#endif
    if (m_wicFactory)
    {
        m_wicFactory->Release();
        m_wicFactory = nullptr;
    }

    DX_SAFE_RELEASE(m_testShaderResourceView);
    DX_SAFE_RELEASE(m_testTexture);
    DX_SAFE_RELEASE(m_sceneTexture);
    DX_SAFE_RELEASE(m_sceneShaderResourceView);
    DX_SAFE_RELEASE(m_sceneRenderTargetView);
    DX_SAFE_RELEASE(vertexShader);
    DX_SAFE_RELEASE(pixelShader);
    DX_SAFE_RELEASE(vertexBuffer);
    DX_SAFE_RELEASE(indexBuffer);
    DX_SAFE_RELEASE(inputLayout);
    DX_SAFE_RELEASE(sampler);
}

//----------------------------------------------------------------------------------------------------
void RendererEx::ClearScreen(Rgba8 const& clearColor) const
{
    // Clear the screen
    float colorAsFloats[4];
    clearColor.GetAsFloats(colorAsFloats);
    m_deviceContext->ClearRenderTargetView(m_renderTargetView, colorAsFloats);
    m_deviceContext->ClearDepthStencilView(m_depthStencilDSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

//----------------------------------------------------------------------------------------------------
void RendererEx::EndCamera(Camera const& camera)
{
    UNUSED(camera)
}



//----------------------------------------------------------------------------------------------------
void RendererEx::SetSamplerMode(eSamplerMode const mode)
{
    m_desiredSamplerMode = mode;
}

void RendererEx::RenderViewportToWindow(WindowEx const& window) const
{
    if (!window.m_displayContext) return;

    // Debug output to verify viewport values
    // printf("Window viewport: x=%.3f, y=%.3f, w=%.3f, h=%.3f\n",
    //        window.viewportX, window.viewportY, window.viewportWidth, window.viewportHeight);

    // Calculate source region in scene texture
    int srcX      = (int)round(window.viewportX * sceneWidth);
    int srcY      = (int)round(window.viewportY * sceneHeight);
    int srcWidth  = (int)round(window.viewportWidth * sceneWidth);
    int srcHeight = (int)round(window.viewportHeight * sceneHeight);

    // Clamp to valid bounds
    srcX      = max(0, min(srcX, (int)sceneWidth - 1));
    srcY      = max(0, min(srcY, (int)sceneHeight - 1));
    srcWidth  = min(srcWidth, (int)sceneWidth - srcX);
    srcHeight = min(srcHeight, (int)sceneHeight - srcY);

    if (srcWidth <= 0 || srcHeight <= 0)
    {
        // Fill with debug color if viewport is invalid
        RECT clientRect;
        GetClientRect((HWND)window.GetWindowHandle(), &clientRect);
        HBRUSH debugBrush = CreateSolidBrush(RGB(255, 0, 0)); // Red for debug
        FillRect((HDC)window.m_displayContext, &clientRect, debugBrush);
        DeleteObject(debugBrush);
        return;
    }

    // Create window-specific pixel buffer
    std::vector<BYTE> windowPixels(srcWidth * srcHeight * 4);

    // Copy pixels from scene buffer with proper bounds checking
    for (int y = 0; y < srcHeight; y++)
    {
        int srcRowStart = ((srcY + y) * sceneWidth + srcX) * 4;
        int dstRowStart = (y * srcWidth) * 4;

        // Additional bounds check
        if (srcRowStart >= 0 && srcRowStart < (int)(pixelData.size() - srcWidth * 4))
        {
            memcpy(&windowPixels[dstRowStart], &pixelData[srcRowStart], srcWidth * 4);
        }
    }

    // Setup bitmap info for this window
    BITMAPINFO localBitmapInfo         = bitmapInfo;
    localBitmapInfo.bmiHeader.biWidth  = srcWidth;
    localBitmapInfo.bmiHeader.biHeight = -srcHeight; // Negative for top-down DIB

    // Render to window with scaling
    int result = StretchDIBits(
        (HDC)window.GetDisplayContext(),
        0, 0,                          // Destination position
        window.width, window.height,   // Destination size (scale to window)
        0, 0,                          // Source position
        srcWidth, srcHeight,           // Source size
        windowPixels.data(),           // Pixel data
        &localBitmapInfo,              // Bitmap info
        DIB_RGB_COLORS,                // Color usage
        SRCCOPY                        // Raster operation
    );

    // Debug: Check if StretchDIBits succeeded
    if (result == GDI_ERROR)
    {
        DWORD error = GetLastError();
        printf("StretchDIBits failed with error: %lu\n", error);
    }
}

void RendererEx::RenderTestTexture()
{
    // BindShader(m_defaultShader);
    // BindTexture(m_testTexture);
    // SetSamplerMode(eSamplerMode::POINT_CLAMP);
    // // m_deviceContext->PSSetShaderResources(0, 1, &m_testShaderResourceView);
    // // m_deviceContext->PSSetSamplers(0, 1, &sampler);
    //
    // UINT stride = sizeof(Vertex_PCU);
    // UINT offset = 0;
    // m_deviceContext->IASetVertexBuffers(0, 1, &m_immediateVBO_PCU->m_buffer, &stride, &offset);
    // m_deviceContext->IASetIndexBuffer(m_immediateIBO->m_buffer, DXGI_FORMAT_R32_UINT, 0);
    // m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    //
    // m_deviceContext->DrawIndexed(6, 0, 0);
    m_deviceContext->VSSetShader(vertexShader, nullptr, 0);
    m_deviceContext->PSSetShader(pixelShader, nullptr, 0);
    m_deviceContext->IASetInputLayout(inputLayout);

    m_deviceContext->PSSetShaderResources(0, 1, &m_testShaderResourceView);
    m_deviceContext->PSSetSamplers(0, 1, &sampler);

    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    m_deviceContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
    m_deviceContext->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
    m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    m_deviceContext->DrawIndexed(6, 0, 0);
}

void RendererEx::RenderWindows(std::vector<WindowEx*> const& windows)
{
    if (!m_sceneRenderTargetView || !m_deviceContext) return;

    // 更新窗口漂移
    for (WindowEx* window : windows)
    {
        // UpdateWindowDrift(window);
        window->UpdateWindowPosition(sceneWidth, sceneHeight, virtualScreenWidth, virtualScreenHeight);
    }

    // 設置渲染目標為場景紋理
    m_deviceContext->OMSetRenderTargets(1, &m_sceneRenderTargetView, nullptr);

    D3D11_VIEWPORT viewport = {};
    viewport.Width          = (FLOAT)sceneWidth;
    viewport.Height         = (FLOAT)sceneHeight;
    viewport.MinDepth       = 0.0f;
    viewport.MaxDepth       = 1.0f;
    m_deviceContext->RSSetViewports(1, &viewport);

    float clearColor[4] = {0.2f, 0.3f, 0.4f, 1.0f};
    m_deviceContext->ClearRenderTargetView(m_sceneRenderTargetView, clearColor);

    RenderTestTexture();
    m_deviceContext->CopyResource(m_stagingTexture, m_sceneTexture);    // ID3D11DeviceContext::CopyResource(destination, source)
    m_deviceContext->Flush();

    UpdateWindows(windows);
}

void RendererEx::RenderWindows(WindowEx& windows)
{
    if (!m_sceneRenderTargetView || !m_deviceContext) return;

    // 更新窗口漂移

        // UpdateWindowDrift(window);
        windows.UpdateWindowPosition(sceneWidth, sceneHeight, virtualScreenWidth, virtualScreenHeight);


    // 設置渲染目標為場景紋理
    m_deviceContext->OMSetRenderTargets(1, &m_sceneRenderTargetView, nullptr);

    D3D11_VIEWPORT viewport = {};
    viewport.Width          = (FLOAT)sceneWidth;
    viewport.Height         = (FLOAT)sceneHeight;
    viewport.MinDepth       = 0.0f;
    viewport.MaxDepth       = 1.0f;
    m_deviceContext->RSSetViewports(1, &viewport);

    float clearColor[4] = {0.2f, 0.3f, 0.4f, 1.0f};
    m_deviceContext->ClearRenderTargetView(m_sceneRenderTargetView, clearColor);

    RenderTestTexture();
    m_deviceContext->CopyResource(m_stagingTexture, m_sceneTexture);    // ID3D11DeviceContext::CopyResource(destination, source)
    m_deviceContext->Flush();

    UpdateWindows(windows);
}

void RendererEx::UpdateWindows(std::vector<WindowEx*> const& windows)
{
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    HRESULT const            hr = m_deviceContext->Map(m_stagingTexture, 0, D3D11_MAP_READ, 0, &mappedResource);
    if (FAILED(hr)) return;

    BYTE const* sourceData = static_cast<BYTE*>(mappedResource.pData);
    for (UINT y = 0; y < sceneHeight; y++)
    {
        memcpy(&pixelData[y * sceneWidth * 4],
               &sourceData[y * mappedResource.RowPitch],
               sceneWidth * 4);
    }

    m_deviceContext->Unmap(m_stagingTexture, 0);

    for (WindowEx* window : windows)
    {
        if (window->needsUpdate)
        {
            RenderViewportToWindow(*window);
            window->needsUpdate = false;
        }
    }
}

void RendererEx::UpdateWindows(WindowEx& windows)
{
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    HRESULT const            hr = m_deviceContext->Map(m_stagingTexture, 0, D3D11_MAP_READ, 0, &mappedResource);
    if (FAILED(hr)) return;

    BYTE const* sourceData = static_cast<BYTE*>(mappedResource.pData);
    for (UINT y = 0; y < sceneHeight; y++)
    {
        memcpy(&pixelData[y * sceneWidth * 4],
               &sourceData[y * mappedResource.RowPitch],
               sceneWidth * 4);
    }

    m_deviceContext->Unmap(m_stagingTexture, 0);


    if (windows.needsUpdate)
    {
        RenderViewportToWindow(windows);
        windows.needsUpdate = false;
    }
}

HRESULT RendererEx::CreateSceneRenderTexture()
{
    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width                = sceneWidth;
    texDesc.Height               = sceneHeight;
    texDesc.MipLevels            = 1;
    texDesc.ArraySize            = 1;
    texDesc.Format               = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count     = 1;
    texDesc.Usage                = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags            = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    HRESULT hr = m_device->CreateTexture2D(&texDesc, nullptr, &m_sceneTexture);
    if (FAILED(hr)) return hr;

    hr = m_device->CreateRenderTargetView(m_sceneTexture, nullptr, &m_sceneRenderTargetView);
    if (FAILED(hr)) return hr;

    hr = m_device->CreateShaderResourceView(m_sceneTexture, nullptr, &m_sceneShaderResourceView);
    if (FAILED(hr)) return hr;

    return S_OK;
}

HRESULT RendererEx::CreateStagingTexture()
{
    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width                = sceneWidth;
    texDesc.Height               = sceneHeight;
    texDesc.MipLevels            = 1;
    texDesc.ArraySize            = 1;
    texDesc.Format               = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count     = 1;
    texDesc.Usage                = D3D11_USAGE_STAGING;
    texDesc.CPUAccessFlags       = D3D11_CPU_ACCESS_READ;
    texDesc.BindFlags            = 0;

    return m_device->CreateTexture2D(&texDesc, nullptr, &m_stagingTexture);
}

HRESULT RendererEx::CreateTestTexture(const wchar_t* imageFile)
{
    if (imageFile)
    {
        // 嘗試從檔案載入
        HRESULT hr = LoadImageFromFile(imageFile, &m_testTexture, &m_testShaderResourceView);
        if (SUCCEEDED(hr))
        {
            return hr;
        }
        // 如果載入失敗，回到程序生成紋理
    }

    const UINT texWidth  = 512;
    const UINT texHeight = 512;
    //
    std::vector<UINT> textureData(texWidth * texHeight);
    //
    for (UINT y = 0; y < texHeight; y++)
    {
        for (UINT x = 0; x < texWidth; x++)
        {
            UINT index = y * texWidth + x;

            float fx = (float)x / texWidth;
            float fy = (float)y / texHeight;

            UINT r = (UINT)(fx * 255);
            UINT g = (UINT)(fy * 255);
            UINT b = (UINT)((1.0f - fx) * 255);

            if (((x / 32) + (y / 32)) % 2 == 0)
            {
                r = min(255, r + 50);
                g = min(255, g + 50);
                b = min(255, b + 50);
            }

            for (int i = 0; i < 5; i++)
            {
                float centerX  = (i % 3) * texWidth / 3.0f + texWidth / 6.0f;
                float centerY  = (i / 3) * texHeight / 3.0f + texHeight / 6.0f;
                float distance = sqrt((x - centerX) * (x - centerX) + (y - centerY) * (y - centerY));

                if (distance < 30.0f)
                {
                    r = (i * 50) % 256;
                    g = (i * 80) % 256;
                    b = (i * 120) % 256;
                }
            }

            textureData[index] = 0xFF000000 | (b << 16) | (g << 8) | r;
        }
    }

    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width                = texWidth;
    texDesc.Height               = texHeight;
    texDesc.MipLevels            = 1;
    texDesc.ArraySize            = 1;
    texDesc.Format               = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count     = 1;
    texDesc.Usage                = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags            = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem                = textureData.data();
    initData.SysMemPitch            = texWidth * 4;

    HRESULT hr = m_device->CreateTexture2D(&texDesc, &initData, &m_testTexture);
    if (FAILED(hr)) return hr;

    hr = m_device->CreateShaderResourceView(m_testTexture, nullptr, &m_testShaderResourceView);
    return hr;
}

HRESULT RendererEx::CreateShaders()
{
    const char* vsSource = R"(
            struct VS_INPUT {
                float3 pos : POSITION;
                float2 tex : TEXCOORD0;
            };

            struct VS_OUTPUT {
                float4 pos : SV_POSITION;
                float2 tex : TEXCOORD0;
            };

            VS_OUTPUT main(VS_INPUT input) {
                VS_OUTPUT output;
                output.pos = float4(input.pos, 1.0f);
                output.tex = input.tex;
                return output;
            }
        )";

    const char* psSource = R"(
            Texture2D tex : register(t0);
            SamplerState sam : register(s0);

            struct PS_INPUT {
                float4 pos : SV_POSITION;
                float2 tex : TEXCOORD0;
            };

            float4 main(PS_INPUT input) : SV_TARGET {
                return tex.Sample(sam, input.tex);
            }
        )";

    ID3DBlob* vsBlob    = nullptr;
    ID3DBlob* psBlob    = nullptr;
    ID3DBlob* errorBlob = nullptr;

    HRESULT hr = D3DCompile(vsSource, strlen(vsSource), nullptr, nullptr, nullptr,
                            "main", "vs_4_0", 0, 0, &vsBlob, &errorBlob);
    if (FAILED(hr))
    {
        if (errorBlob) errorBlob->Release();
        return hr;
    }

    hr = m_device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
                                      nullptr, &vertexShader);
    if (FAILED(hr))
    {
        vsBlob->Release();
        return hr;
    }

    D3D11_INPUT_ELEMENT_DESC layout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };

    hr = m_device->CreateInputLayout(layout, 2, vsBlob->GetBufferPointer(),
                                     vsBlob->GetBufferSize(), &inputLayout);
    vsBlob->Release();
    if (FAILED(hr)) return hr;

    hr = D3DCompile(psSource, strlen(psSource), nullptr, nullptr, nullptr,
                    "main", "ps_4_0", 0, 0, &psBlob, &errorBlob);
    if (FAILED(hr))
    {
        if (errorBlob) errorBlob->Release();
        return hr;
    }

    hr = m_device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(),
                                     nullptr, &pixelShader);
    psBlob->Release();

    return hr;
}

HRESULT RendererEx::CreateVertexBuffer()
{
    Vertex vertices[] = {
        {XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT2(0.0f, 1.0f)},
        {XMFLOAT3(-1.0f, 1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f)},
        {XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT2(1.0f, 0.0f)},
        {XMFLOAT3(1.0f, -1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f)}
    };

    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.Usage             = D3D11_USAGE_DEFAULT;
    bufferDesc.ByteWidth         = sizeof(vertices);
    bufferDesc.BindFlags         = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem                = vertices;

    HRESULT hr = m_device->CreateBuffer(&bufferDesc, &initData, &vertexBuffer);
    if (FAILED(hr)) return hr;

    UINT indices[] = {0, 1, 2, 0, 2, 3};

    bufferDesc.ByteWidth = sizeof(indices);
    bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    initData.pSysMem     = indices;

    return m_device->CreateBuffer(&bufferDesc, &initData, &indexBuffer);
}

HRESULT RendererEx::CreateSampler()
{
    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter             = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU           = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressV           = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressW           = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.ComparisonFunc     = D3D11_COMPARISON_NEVER;
    samplerDesc.MinLOD             = 0;
    samplerDesc.MaxLOD             = D3D11_FLOAT32_MAX;

    return m_device->CreateSamplerState(&samplerDesc, &sampler);
}

HRESULT RendererEx::LoadImageFromFile(const wchar_t*             filename,
                                      ID3D11Texture2D**          texture,
                                      ID3D11ShaderResourceView** srv) const
{
    if (!m_wicFactory) return E_FAIL;

    IWICBitmapDecoder*     decoder   = nullptr;
    IWICBitmapFrameDecode* frame     = nullptr;
    IWICFormatConverter*   converter = nullptr;

    HRESULT hr = m_wicFactory->CreateDecoderFromFilename(
        filename, nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &decoder);
    if (FAILED(hr)) return hr;

    hr = decoder->GetFrame(0, &frame);
    if (FAILED(hr))
    {
        decoder->Release();
        return hr;
    }

    hr = m_wicFactory->CreateFormatConverter(&converter);
    if (FAILED(hr))
    {
        frame->Release();
        decoder->Release();
        return hr;
    }

    hr = converter->Initialize(frame, GUID_WICPixelFormat32bppBGRA,
                               WICBitmapDitherTypeNone, nullptr, 0.0,
                               WICBitmapPaletteTypeCustom);
    if (FAILED(hr))
    {
        converter->Release();
        frame->Release();
        decoder->Release();
        return hr;
    }

    UINT width, height;
    converter->GetSize(&width, &height);

    // 創建像素數據緩衝區
    std::vector<BYTE> pixels(width * height * 4);
    hr = converter->CopyPixels(nullptr, width * 4, pixels.size(), pixels.data());

    if (SUCCEEDED(hr))
    {
        // 創建 D3D11 紋理
        D3D11_TEXTURE2D_DESC texDesc = {};
        texDesc.Width                = width;
        texDesc.Height               = height;
        texDesc.MipLevels            = 1;
        texDesc.ArraySize            = 1;
        texDesc.Format               = DXGI_FORMAT_R8G8B8A8_UNORM;
        texDesc.SampleDesc.Count     = 1;
        texDesc.Usage                = D3D11_USAGE_DEFAULT;
        texDesc.BindFlags            = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA initData = {};
        initData.pSysMem                = pixels.data();
        initData.SysMemPitch            = width * 4;

        hr = m_device->CreateTexture2D(&texDesc, &initData, texture);
        if (SUCCEEDED(hr))
        {
            hr = m_device->CreateShaderResourceView(*texture, nullptr, srv);
        }
    }

    converter->Release();
    frame->Release();
    decoder->Release();
    return hr;
}

void RendererEx::DebugSaveSceneTexture()
{
    // This method can help debug what's actually in your scene texture
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    HRESULT                  hr = m_deviceContext->Map(m_stagingTexture, 0, D3D11_MAP_READ, 0, &mappedResource);

    if (SUCCEEDED(hr))
    {
        // Save first few pixels to debug output
        BYTE* pixels = static_cast<BYTE*>(mappedResource.pData);
        printf("Scene texture first pixels: ");
        for (int i = 0; i < 16; i += 4)
        {
            printf("(%d,%d,%d,%d) ", pixels[i], pixels[i + 1], pixels[i + 2], pixels[i + 3]);
        }
        printf("\n");

        m_deviceContext->Unmap(m_stagingTexture, 0);
    }
}
