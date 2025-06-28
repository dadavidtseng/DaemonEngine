//----------------------------------------------------------------------------------------------------
// RendererEx.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Renderer/RendererEx.hpp"


#include <d3d11.h>
#include <d3dcompiler.h>
// #include <wincodec.h>
#include <vector>

#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/Image.hpp"
#include "ThirdParty/stb/stb_image.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
// #pragma comment(lib, "windowscodecs.lib")

RendererEx::RendererEx()
{
    ZeroMemory(&m_bitmapInfo, sizeof(BITMAPINFO));
    m_bitmapInfo.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    m_bitmapInfo.bmiHeader.biWidth       = sceneWidth;
    m_bitmapInfo.bmiHeader.biHeight      = static_cast<LONG>(sceneHeight);
    m_bitmapInfo.bmiHeader.biPlanes      = 1;
    m_bitmapInfo.bmiHeader.biBitCount    = 32;
    m_bitmapInfo.bmiHeader.biCompression = BI_RGB;

    pixelData.resize(sceneWidth * sceneHeight * 4);
}

RendererEx::~RendererEx()
{
    Cleanup();
}

void RendererEx::Startup()
{
}

HRESULT RendererEx::Initialize()
{
    CoInitialize(nullptr);
    mainWindow = CreateWindowEx(
        NULL,
        L"STATIC",
        L"Hidden",
        WS_POPUP,
        0, 0, 1, 1,
        nullptr,
        nullptr,
        GetModuleHandle(nullptr),
        nullptr
    );
    // // 初始化 WIC 工廠
    // HRESULT hr = CoCreateInstance(
    //     CLSID_WICImagingFactory,
    //     nullptr,
    //     CLSCTX_INPROC_SERVER,
    //     IID_IWICImagingFactory,
    //     reinterpret_cast<void**>(&m_wicFactory)
    // );

    // if (FAILED(hr))
    // {
    //     // WIC 初始化失敗，但繼續執行（使用程序生成紋理）
    //     m_wicFactory = nullptr;
    // }

    HRESULT hr = CreateDeviceAndSwapChain();
    if (FAILED(hr)) return hr;

    hr = CreateSceneRenderTexture();
    if (FAILED(hr)) return hr;

    hr = CreateStagingTexture();
    if (FAILED(hr)) return hr;

    // hr = CreateTestTexture(L"Data/Images/Windowkill.png");
    // if (FAILED(hr)) return hr;

    // Image const defaultImage(IntVec2(2, 2), Rgba8::WHITE);
    m_defaultTexture         = CreateOrGetTextureFromFile("Data/Images/Butler.png");
    m_defaultTexture->m_name = "Default";

    // hr = CreateTestTexture2(L"Data/Images/Butler.png");
    // if (FAILED(hr)) return hr;
    //
    hr = CreateShaders();
    if (FAILED(hr)) return hr;

    hr = CreateVertexBuffer();
    if (FAILED(hr)) return hr;

    hr = CreateSampler();
    if (FAILED(hr)) return hr;

    return S_OK;
}

// HRESULT RendererEx::LoadImageFromFile(const wchar_t* filename, ID3D11Texture2D** texture, ID3D11ShaderResourceView** srv) const
// {
//     if (!m_wicFactory) return E_FAIL;
//
//     IWICBitmapDecoder*     decoder   = nullptr;
//     IWICBitmapFrameDecode* frame     = nullptr;
//     IWICFormatConverter*   converter = nullptr;
//
//     HRESULT hr = m_wicFactory->CreateDecoderFromFilename(
//         filename, nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &decoder);
//     if (FAILED(hr)) return hr;
//
//     hr = decoder->GetFrame(0, &frame);
//     if (FAILED(hr))
//     {
//         decoder->Release();
//         return hr;
//     }
//
//     hr = m_wicFactory->CreateFormatConverter(&converter);
//     if (FAILED(hr))
//     {
//         frame->Release();
//         decoder->Release();
//         return hr;
//     }
//
//     hr = converter->Initialize(frame, GUID_WICPixelFormat32bppBGRA,
//                                WICBitmapDitherTypeNone, nullptr, 0.0,
//                                WICBitmapPaletteTypeCustom);
//     if (FAILED(hr))
//     {
//         converter->Release();
//         frame->Release();
//         decoder->Release();
//         return hr;
//     }
//
//     UINT width, height;
//     converter->GetSize(&width, &height);
//
//     // 創建像素數據緩衝區
//     std::vector<BYTE> pixels(width * height * 4);
//     hr = converter->CopyPixels(nullptr, width * 4, (UINT)pixels.size(), pixels.data());
//
//     if (SUCCEEDED(hr))
//     {
//         // 創建 D3D11 紋理
//         D3D11_TEXTURE2D_DESC texDesc = {};
//         texDesc.Width                = width;
//         texDesc.Height               = height;
//         texDesc.MipLevels            = 1;
//         texDesc.ArraySize            = 1;
//         texDesc.Format               = DXGI_FORMAT_R8G8B8A8_UNORM;
//         texDesc.SampleDesc.Count     = 1;
//         texDesc.Usage                = D3D11_USAGE_DEFAULT;
//         texDesc.BindFlags            = D3D11_BIND_SHADER_RESOURCE;
//
//         D3D11_SUBRESOURCE_DATA initData = {};
//         initData.pSysMem                = pixels.data();
//         initData.SysMemPitch            = width * 4;
//
//         hr = m_device->CreateTexture2D(&texDesc, &initData, texture);
//         if (SUCCEEDED(hr))
//         {
//             hr = m_device->CreateShaderResourceView(*texture, nullptr, srv);
//         }
//     }
//
//     converter->Release();
//     frame->Release();
//     decoder->Release();
//     return hr;
// }

void RendererEx::SetWindowDriftParams(HWND const hwnd, const DriftParams& params)
{
    // for (WindowEx& window : windows)
    // {
    //     if ((HWND)window.m_windowHandle == hwnd)
    //     {
    //         window.drift = params;
    //         break;
    //     }
    // }
}

void RendererEx::StartDragging(HWND const hwnd, POINT const& mousePos)
{
    // for (WindowEx& window : windows)
    // {
    //     if ((HWND)window.m_windowHandle == hwnd)
    //     {
    //         window.isDragging = true;
    //         RECT rect;
    //         GetWindowRect(hwnd, &rect);
    //         window.dragOffset.x = mousePos.x - rect.left;
    //         window.dragOffset.y = mousePos.y - rect.top;
    //         // 拖拽時停止漂移
    //         window.drift.velocityX = 0;
    //         window.drift.velocityY = 0;
    //         break;
    //     }
    // }
}

void RendererEx::StopDragging(HWND hwnd)
{
    // for (WindowEx& window : windows)
    // {
    //     if ((HWND)window.m_windowHandle == hwnd)
    //     {
    //         window.isDragging = false;
    //         // 可以在這裡給一個初始速度來模擬拋擲效果
    //         std::uniform_real_distribution<float> throwDist(-100.0f, 100.0f);
    //         window.drift.velocityX = throwDist(window.rng);
    //         window.drift.velocityY = throwDist(window.rng);
    //         break;
    //     }
    // }
}

void RendererEx::UpdateDragging(HWND const hwnd, POINT const& mousePos) const
{
    // /// https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setwindowpos
    // for (WindowEx const& window : windows)
    // {
    //     if ((HWND)window.m_windowHandle == hwnd && window.isDragging)
    //     {
    //         int newX = mousePos.x - window.dragOffset.x;
    //         int newY = mousePos.y - window.dragOffset.y;
    //         SetWindowPos(hwnd, nullptr, newX, newY, 0, 0,
    //                      SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    //         break;
    //     }
    // }
}

void RendererEx::UpdateWindowDrift(WindowEx& window) const
{
    // if (window.isDragging) return; // 拖拽時不漂移
    //
    // // auto  currentTime     = std::chrono::steady_clock::now();
    // // float deltaTime       = std::chrono::duration<float>(currentTime - window.lastUpdateTime).count();
    // // window.lastUpdateTime = currentTime;
    // //
    // // // if (deltaTime > 0.1f) deltaTime = 0.1f; // 限制最大 delta time
    // // // 更嚴格的 delta time 控制
    // // if (deltaTime > 0.016f) deltaTime = 0.016f; // 限制為 60fps
    // // if (deltaTime < 0.001f) return; // 太小的變化直接忽略
    //
    // RECT windowRect;
    // GetWindowRect((HWND)window.m_windowHandle, &windowRect);
    //
    // int currentX = windowRect.left;
    // int currentY = windowRect.top;
    //
    // // 重力效果
    // if (window.drift.enableGravity)
    // {
    //     window.drift.velocityY += window.drift.acceleration * deltaTime;
    // }
    //
    // // 隨機漂移
    // if (window.drift.enableWander)
    // {
    //     window.drift.velocityX += window.wanderDist(window.rng) * window.drift.wanderStrength * deltaTime;
    //     window.drift.velocityY += window.wanderDist(window.rng) * window.drift.wanderStrength * deltaTime;
    // }
    //
    // // 速度限制
    // float const currentSpeed = sqrt(window.drift.velocityX * window.drift.velocityX +
    //     window.drift.velocityY * window.drift.velocityY);
    // if (currentSpeed > window.drift.targetVelocity)
    // {
    //     float const scale = window.drift.targetVelocity / currentSpeed;
    //     window.drift.velocityX *= scale;
    //     window.drift.velocityY *= scale;
    // }
    //
    // // 阻力
    // window.drift.velocityX *= window.drift.drag;
    // window.drift.velocityY *= window.drift.drag;
    //
    // // 計算新位置
    // int newX = currentX + static_cast<int>(window.drift.velocityX * deltaTime);
    // int newY = currentY + static_cast<int>(window.drift.velocityY * deltaTime);
    //
    // // 邊界碰撞檢測和反彈
    // RECT clientRect;
    // GetClientRect((HWND)window.m_windowHandle, &clientRect);
    // int const windowWidth  = clientRect.right - clientRect.left;
    // int const windowHeight = clientRect.bottom - clientRect.top;
    //
    // bool bounced = false;
    //
    // // 左右邊界
    // if (newX < 0)
    // {
    //     newX                   = 0;
    //     window.drift.velocityX = -window.drift.velocityX * window.drift.bounceEnergy;
    //     bounced                = true;
    // }
    // else if (newX + windowWidth > virtualScreenWidth)
    // {
    //     newX                   = virtualScreenWidth - windowWidth;
    //     window.drift.velocityX = -window.drift.velocityX * window.drift.bounceEnergy;
    //     bounced                = true;
    // }
    //
    // // 上下邊界
    // if (newY < 0)
    // {
    //     newY                   = 0;
    //     window.drift.velocityY = -window.drift.velocityY * window.drift.bounceEnergy;
    //     bounced                = true;
    // }
    // else if (newY + windowHeight > virtualScreenHeight)
    // {
    //     newY                   = virtualScreenHeight - windowHeight;
    //     window.drift.velocityY = -window.drift.velocityY * window.drift.bounceEnergy;
    //     bounced                = true;
    // }
    //
    // // 反彈時添加一些隨機性
    // if (bounced)
    // {
    //     std::uniform_real_distribution<float> bounceDist(-30.0f, 30.0f);
    //     window.drift.velocityX += bounceDist(window.rng);
    //     window.drift.velocityY += bounceDist(window.rng);
    // }
    //
    // // 移動窗口
    // if (newX != currentX || newY != currentY)
    // {
    //     SetWindowPos((HWND)window.m_windowHandle, nullptr, newX, newY, 0, 0,
    //                  SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    // }
}

// HRESULT RendererEx::AddWindow(HWND const& hwnd)
// {
//     WindowEx window;
//     window.m_windowHandle   = hwnd;
//     window.m_displayContext = GetDC(hwnd);
//     window.needsUpdate      = true;
//
//     // UpdateWindowPosition(window);
//     windows.push_back(window);
//     return S_OK;
// }

void RendererEx::UpdateWindowPosition(WindowEx& window) const
{
    // RECT windowRect;
    // GetWindowRect((HWND)window.m_windowHandle, &windowRect);
    //
    // if (memcmp(&windowRect, &window.lastRect, sizeof(RECT)) != 0)
    // {
    //     window.lastRect.left   = windowRect.left;
    //     window.lastRect.top    = windowRect.top;
    //     window.lastRect.right  = windowRect.right;
    //     window.lastRect.bottom = windowRect.bottom;
    //     window.needsUpdate     = true;
    //
    //     RECT clientRect;
    //     GetClientRect((HWND)window.m_windowHandle, &clientRect);
    //     window.width  = clientRect.right - clientRect.left;
    //     window.height = clientRect.bottom - clientRect.top;
    //
    //     window.viewportX      = (float)windowRect.left / (float)virtualScreenWidth;
    //     window.viewportY      = (float)windowRect.top / (float)virtualScreenHeight;
    //     window.viewportWidth  = (float)window.width / (float)virtualScreenWidth;
    //     window.viewportHeight = (float)window.height / (float)virtualScreenHeight;
    //
    //     // 確保座標對齊到像素邊界
    //     float pixelAlignX = 1.0f / (float)sceneWidth;
    //     float pixelAlignY = 1.0f / (float)sceneHeight;
    //
    //     window.viewportX      = floor(window.viewportX / pixelAlignX) * pixelAlignX;
    //     window.viewportY      = floor(window.viewportY / pixelAlignY) * pixelAlignY;
    //     window.viewportWidth  = ceil(window.viewportWidth / pixelAlignX) * pixelAlignX;
    //     window.viewportHeight = ceil(window.viewportHeight / pixelAlignY) * pixelAlignY;
    //
    //     window.viewportX      = max(0.0f, min(1.0f, window.viewportX));
    //     window.viewportY      = max(0.0f, min(1.0f, window.viewportY));
    //     window.viewportWidth  = max(0.0f, min(1.0f - window.viewportX, window.viewportWidth));
    //     window.viewportHeight = max(0.0f, min(1.0f - window.viewportY, window.viewportHeight));
    // }
}

void RendererEx::Render(std::vector<WindowEx> windows)
{
    if (!m_sceneRenderTargetView || !m_deviceContext) return;

    // // 更新窗口漂移
    // for (WindowEx& window : windows)
    // {
    //     UpdateWindowDrift(window);
    //     UpdateWindowPosition(window);
    // }

    // 設置渲染目標為場景紋理
    m_deviceContext->OMSetRenderTargets(1, &m_sceneRenderTargetView, nullptr);

    D3D11_VIEWPORT viewport = {};
    viewport.Width          = (FLOAT)sceneWidth;
    viewport.Height         = (FLOAT)sceneHeight;
    viewport.MinDepth       = 0.0f;
    viewport.MaxDepth       = 1.0f;
    m_deviceContext->RSSetViewports(1, &viewport);

    float clearColor[4] = {0.1f, 0.1f, 0.2f, 1.0f};
    m_deviceContext->ClearRenderTargetView(m_sceneRenderTargetView, clearColor);

    RenderTestTexture();
    m_deviceContext->CopyResource(m_stagingTexture, m_sceneTexture);    // ID3D11DeviceContext::CopyResource(destination, source)
    UpdateWindows(windows);
}

HRESULT RendererEx::CreateDeviceAndSwapChain()
{
    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferCount          = 1;
    swapChainDesc.BufferDesc.Width     = 1;
    swapChainDesc.BufferDesc.Height    = 1;
    swapChainDesc.BufferDesc.Format    = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage          = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.OutputWindow         = mainWindow;
    swapChainDesc.SampleDesc.Count     = 1;
    swapChainDesc.Windowed             = TRUE;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        D3D11_CREATE_DEVICE_DEBUG,
        nullptr,
        0,
        D3D11_SDK_VERSION,
        &swapChainDesc,
        &m_mainSwapChain,
        &m_device,
        nullptr,
        &m_deviceContext
    );

    if (FAILED(hr)) return hr;

    ID3D11Texture2D* backBuffer;
    hr = m_mainSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
    if (FAILED(hr)) return hr;

    hr = m_device->CreateRenderTargetView(backBuffer, nullptr, &m_mainBackBufferRenderTargetView);
    backBuffer->Release();

    return hr;
}

HRESULT RendererEx::CreateSceneRenderTexture()
{
    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width                = sceneWidth;
    texDesc.Height               = sceneHeight;
    texDesc.MipLevels            = 1;
    texDesc.ArraySize            = 1;
    texDesc.Format               = DXGI_FORMAT_B8G8R8A8_UNORM;
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
    texDesc.Format               = DXGI_FORMAT_B8G8R8A8_UNORM;
    texDesc.SampleDesc.Count     = 1;
    texDesc.Usage                = D3D11_USAGE_STAGING;
    texDesc.CPUAccessFlags       = D3D11_CPU_ACCESS_READ;

    return m_device->CreateTexture2D(&texDesc, nullptr, &m_stagingTexture);
}

// HRESULT RendererEx::CreateTestTexture(const wchar_t* imageFile)
// {
//     if (imageFile)
//     {
//         // 嘗試從檔案載入
//         HRESULT hr = LoadImageFromFile(imageFile, &m_testTexture, &m_testShaderResourceView);
//         if (SUCCEEDED(hr))
//         {
//             return hr;
//         }
//         // 如果載入失敗，回到程序生成紋理
//     }
//
//     const UINT texWidth  = 512;
//     const UINT texHeight = 512;
//     //
//     std::vector<UINT> textureData(texWidth * texHeight);
//
//     for (UINT y = 0; y < texHeight; y++)
//     {
//         for (UINT x = 0; x < texWidth; x++)
//         {
//             UINT index = y * texWidth + x;
//
//             float fx = (float)x / texWidth;
//             float fy = (float)y / texHeight;
//
//             UINT r = (UINT)(fx * 255);
//             UINT g = (UINT)(fy * 255);
//             UINT b = (UINT)((1.0f - fx) * 255);
//
//             if (((x / 32) + (y / 32)) % 2 == 0)
//             {
//                 r = min(255, r + 50);
//                 g = min(255, g + 50);
//                 b = min(255, b + 50);
//             }
//
//             for (int i = 0; i < 5; i++)
//             {
//                 float centerX  = (i % 3) * texWidth / 3.0f + texWidth / 6.0f;
//                 float centerY  = (i / 3) * texHeight / 3.0f + texHeight / 6.0f;
//                 float distance = sqrt((x - centerX) * (x - centerX) + (y - centerY) * (y - centerY));
//
//                 if (distance < 30.0f)
//                 {
//                     r = (i * 50) % 256;
//                     g = (i * 80) % 256;
//                     b = (i * 120) % 256;
//                 }
//             }
//
//             textureData[index] = 0xFF000000 | (b << 16) | (g << 8) | r;
//         }
//     }
//
//     D3D11_TEXTURE2D_DESC texDesc = {};
//     texDesc.Width                = texWidth;
//     texDesc.Height               = texHeight;
//     texDesc.MipLevels            = 1;
//     texDesc.ArraySize            = 1;
//     texDesc.Format               = DXGI_FORMAT_R8G8B8A8_UNORM;
//     texDesc.SampleDesc.Count     = 1;
//     texDesc.Usage                = D3D11_USAGE_DEFAULT;
//     texDesc.BindFlags            = D3D11_BIND_SHADER_RESOURCE;
//
//     D3D11_SUBRESOURCE_DATA initData = {};
//     initData.pSysMem                = textureData.data();
//     initData.SysMemPitch            = texWidth * 4;
//
//     HRESULT hr = m_device->CreateTexture2D(&texDesc, &initData, &m_testTexture);
//     if (FAILED(hr)) return hr;
//
//     hr = m_device->CreateShaderResourceView(m_testTexture, nullptr, &m_testShaderResourceView);
//     return hr;
// }

// HRESULT RendererEx::CreateTestTexture2(const wchar_t* imageFile)
// {
//     if (imageFile)
//     {
//         // 嘗試從檔案載入
//         HRESULT hr = LoadImageFromFile(imageFile, &m_defaultTexture->m_texture, &m_testShaderResourceView);
//         if (SUCCEEDED(hr))
//         {
//             return hr;
//         }
//     }
//
//     D3D11_TEXTURE2D_DESC   texDesc;
//     D3D11_SUBRESOURCE_DATA initData;
//
//     HRESULT hr = m_device->CreateTexture2D(&texDesc, &initData, &m_defaultTexture->m_texture);
//     if (FAILED(hr)) return hr;
//
//     hr = m_device->CreateShaderResourceView(m_defaultTexture->m_texture, nullptr, &m_testShaderResourceView);
//     return hr;
// }

HRESULT RendererEx::CreateShaders()
{
    const char* vsSource = R"(
        struct VS_INPUT
        {
            float3 pos : VERTEX_POSITION;
            float4 a_color : VERTEX_COLOR;
            float2 tex : VERTEX_UVTEXCOORDS;
        };

        struct VS_OUTPUT
        {
            float4 pos : SV_POSITION;
            float4 color : COLOR0;
            float2 tex : TEXCOORD0;
        };

        VS_OUTPUT main(VS_INPUT input)
        {
            VS_OUTPUT output;
            output.pos = float4(input.pos, 1.0f);
            output.color = input.a_color;
            output.tex = input.tex;
            return output;
        }
        )";

    const char* psSource = R"(
        Texture2D tex : register(t0);
        SamplerState sam : register(s0);

        struct PS_INPUT
        {
            float4 pos : SV_POSITION;
            float4 color : COLOR0;
            float2 tex : TEXCOORD0;
        };

        float4 main(PS_INPUT input) : SV_TARGET
        {
            float4 texColor = tex.Sample(sam, input.tex);
    return texColor * input.color;     // 紋理顏色 × 頂點顏色
        }
        )";

    ID3DBlob* vsBlob    = nullptr;
    ID3DBlob* psBlob    = nullptr;
    ID3DBlob* errorBlob = nullptr;

    HRESULT hr = D3DCompile(vsSource, strlen(vsSource), nullptr, nullptr, nullptr,
                            "main", "vs_5_0", 0, 0, &vsBlob, &errorBlob);
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

    // Create a local array of input element descriptions that defines the vertex layout.
    D3D11_INPUT_ELEMENT_DESC inputElementDesc[3];
    UINT                     numElements = 3;

    inputElementDesc[0] = {"VERTEX_POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0};
    inputElementDesc[1] = {"VERTEX_COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0};
    inputElementDesc[2] = {"VERTEX_UVTEXCOORDS", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0};


    hr = m_device->CreateInputLayout(inputElementDesc, numElements, vsBlob->GetBufferPointer(),
                                     vsBlob->GetBufferSize(), &inputLayout);
    vsBlob->Release();
    if (FAILED(hr)) return hr;

    hr = D3DCompile(psSource, strlen(psSource), nullptr, nullptr, nullptr,
                    "main", "ps_5_0", 0, 0, &psBlob, &errorBlob);
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
    m_vertexList.emplace_back(Vec3(-1.f, -1.f, 0.f), Rgba8(255, 255, 255, 255), Vec2(1, 0));
    m_vertexList.emplace_back(Vec3(-1.f, 1.f, 0.f), Rgba8(255, 255, 255, 255), Vec2(1, 1));
    m_vertexList.emplace_back(Vec3(1.f, 1.f, 0.f), Rgba8(255, 255, 255, 255), Vec2(0, 1));
    m_vertexList.emplace_back(Vec3(1.f, -1.f, 0.f), Rgba8(255, 255, 255, 255), Vec2(0, 0));


    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.Usage             = D3D11_USAGE_DEFAULT;
    bufferDesc.ByteWidth         = m_vertexList.size() * sizeof(Vertex_PCU);
    bufferDesc.BindFlags         = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem                = m_vertexList.data();

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

float RendererEx::GetSceneWidth()
{
    return sceneWidth;
}

float RendererEx::GetSceneHeight()
{
    return sceneHeight;
}

void RendererEx::RenderTestTexture() const
{
    m_deviceContext->VSSetShader(vertexShader, nullptr, 0);
    m_deviceContext->PSSetShader(pixelShader, nullptr, 0);
    m_deviceContext->IASetInputLayout(inputLayout);

    m_deviceContext->PSSetShaderResources(0, 1, &m_defaultTexture->m_shaderResourceView);
    m_deviceContext->PSSetSamplers(0, 1, &sampler);

    UINT stride = sizeof(Vertex_PCU);
    UINT offset = 0;
    m_deviceContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
    m_deviceContext->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
    m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    m_deviceContext->DrawIndexed(6, 0, 0);
}

void RendererEx::UpdateWindows(std::vector<WindowEx>& windows)
{
    // bool needsUpdate = false;
    // for (const auto& window : windows)
    // {
    //     if (window.needsUpdate)
    //     {
    //         needsUpdate = true;
    //         break;
    //     }
    // }
    //
    // if (!needsUpdate) return;

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

    for (WindowEx& window : windows)
    {
        if (window.needsUpdate)
        {
            RenderViewportToWindow(window);
            window.needsUpdate = false;
        }
    }
}

void RendererEx::RenderViewportToWindow(WindowEx const& window) const
{
    if (!window.m_displayContext) return;

    // 計算在場景紋理中的區域
    int srcX      = (int)round(window.viewportX * sceneWidth);
    int srcY      = (int)round(window.viewportY * sceneHeight);
    int srcWidth  = (int)round(window.viewportWidth * sceneWidth);
    int srcHeight = (int)round(window.viewportHeight * sceneHeight);

    // 確保不超出邊界
    srcX      = max(0, min(srcX, (int)sceneWidth - 1));
    srcY      = max(0, min(srcY, (int)sceneHeight - 1));
    srcWidth  = min(srcWidth, (int)sceneWidth - srcX);
    srcHeight = min(srcHeight, (int)sceneHeight - srcY);

    if (srcWidth <= 0 || srcHeight <= 0) return;

    // 創建臨時的 DIB 數據
    std::vector<BYTE> windowPixels(srcWidth * srcHeight * 4);

    for (int y = 0; y < srcHeight; y++)
    {
        int srcRowIndex = (srcY + y) * sceneWidth + srcX;
        int dstRowIndex = y * srcWidth;

        memcpy(&windowPixels[dstRowIndex * 4],
               &pixelData[srcRowIndex * 4],
               srcWidth * 4);
    }

    // 設置 DIB 信息
    BITMAPINFO localBitmapInfo         = m_bitmapInfo;
    localBitmapInfo.bmiHeader.biWidth  = srcWidth;
    localBitmapInfo.bmiHeader.biHeight = -srcHeight;

    // 使用 StretchDIBits 來縮放顯示
    StretchDIBits(
        (HDC)window.m_displayContext,
        0, 0,                          // 目標位置
        window.width, window.height,   // 目標大小 (縮放到窗口大小)
        0, 0,                          // 源起始位置
        srcWidth, srcHeight,           // 源大小
        windowPixels.data(),           // 像素數據
        &localBitmapInfo,              // DIB 信息
        DIB_RGB_COLORS,                // 顏色模式
        SRCCOPY                        // 複製模式
    );
}

void RendererEx::Cleanup()
{
    // for (WindowEx& window : windows)
    // {
    //     if (window.m_displayContext) ReleaseDC((HWND)window.m_windowHandle, (HDC)window.m_displayContext);
    // }

    // 釋放所有 D3D11 和相關對象
    if (sampler)
    {
        sampler->Release();
        sampler = nullptr;
    }
    if (inputLayout)
    {
        inputLayout->Release();
        inputLayout = nullptr;
    }
    if (indexBuffer)
    {
        indexBuffer->Release();
        indexBuffer = nullptr;
    }
    if (vertexBuffer)
    {
        vertexBuffer->Release();
        vertexBuffer = nullptr;
    }
    if (pixelShader)
    {
        pixelShader->Release();
        pixelShader = nullptr;
    }
    if (vertexShader)
    {
        vertexShader->Release();
        vertexShader = nullptr;
    }
    // if (m_testShaderResourceView)
    // {
    //     m_testShaderResourceView->Release();
    //     m_testShaderResourceView = nullptr;
    // }
    // if (m_testTexture)
    // {
    //     m_testTexture->Release();
    //     m_testTexture = nullptr;
    // }
    if (m_stagingTexture)
    {
        m_stagingTexture->Release();
        m_stagingTexture = nullptr;
    }
    if (m_sceneShaderResourceView)
    {
        m_sceneShaderResourceView->Release();
        m_sceneShaderResourceView = nullptr;
    }
    if (m_sceneRenderTargetView)
    {
        m_sceneRenderTargetView->Release();
        m_sceneRenderTargetView = nullptr;
    }
    if (m_sceneTexture)
    {
        m_sceneTexture->Release();
        m_sceneTexture = nullptr;
    }
    if (m_mainBackBufferRenderTargetView)
    {
        m_mainBackBufferRenderTargetView->Release();
        m_mainBackBufferRenderTargetView = nullptr;
    }

    // 重要：在釋放 device 之前先釋放 context
    if (m_deviceContext)
    {
        m_deviceContext->ClearState();  // 清除所有綁定的資源
        m_deviceContext->Flush();       // 確保所有命令執行完畢
        m_deviceContext->Release();
        m_deviceContext = nullptr;
    }

    if (m_mainSwapChain)
    {
        m_mainSwapChain->Release();
        m_mainSwapChain = nullptr;
    }

    // // **這個是你缺少的：釋放 WIC 工廠**
    // if (m_wicFactory)
    // {
    //     m_wicFactory->Release();
    //     m_wicFactory = nullptr;
    // }

    // 最後釋放 device
    if (m_device)
    {
        m_device->Release();
        m_device = nullptr;
    }
}

//----------------------------------------------------------------------------------------------------
Texture* RendererEx::CreateOrGetTextureFromFile(char const* imageFilePath)
{
    // See if we already have this texture previously loaded
    if (Texture* existingTexture = GetTextureForFileName(imageFilePath))
    {
        return existingTexture;
    }

    // Never seen this texture before!  Let's load it.
    Texture* newTexture = CreateTextureFromFile(imageFilePath);

    return newTexture;
}

//----------------------------------------------------------------------------------------------------
Texture* RendererEx::GetTextureForFileName(char const* imageFilePath) const
{
    for (Texture* texture : m_loadedTextures)
    {
        if (texture && !strcmp(texture->m_name.c_str(), imageFilePath))
        {
            return texture;
        }
    }

    return nullptr;
}

//----------------------------------------------------------------------------------------------------
Texture* RendererEx::CreateTextureFromFile(char const* imageFilePath)
{
    IntVec2 dimensions    = IntVec2::ZERO; // This will be filled in for us to indicate image width & height
    int     bytesPerTexel = 0;
    // This will be filled in for us to indicate how many color components the image had (e.g. 3=RGB=24bit, 4=RGBA=32bit)
    int constexpr numComponentsRequested = 0; // don't care; we support 3 (24-bit RGB) or 4 (32-bit RGBA)

    // Load (and decompress) the image RGB(A) bytes from a file on disk into a memory buffer (array of bytes)
    stbi_set_flip_vertically_on_load(1); // We prefer uvTexCoords has origin (0,0) at BOTTOM LEFT
    unsigned char* texelData = stbi_load(imageFilePath,
                                         &dimensions.x,
                                         &dimensions.y,
                                         &bytesPerTexel,
                                         numComponentsRequested);

    GUARANTEE_OR_DIE(texelData, Stringf("Failed to load image \"%s\"", imageFilePath))

    Image const fileImage(imageFilePath);
    Texture*    newTexture = CreateTextureFromImage(fileImage);

    // Free the raw image texel data now that we've sent a copy of it down to the GPU to be stored in video memory
    stbi_image_free(texelData);

    //m_loadedTextures.push_back(newTexture);
    return newTexture;
}

//----------------------------------------------------------------------------------------------------
Texture* RendererEx::CreateTextureFromImage(Image const& image)
{
    Texture* newTexture      = new Texture();
    newTexture->m_name       = image.GetImageFilePath();
    newTexture->m_dimensions = image.GetDimensions();

    D3D11_TEXTURE2D_DESC textureDesc = {};
    textureDesc.Width                = image.GetDimensions().x;
    textureDesc.Height               = image.GetDimensions().y;
    textureDesc.MipLevels            = 1;
    textureDesc.ArraySize            = 1;
    textureDesc.Format               = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.SampleDesc.Count     = 1;
    textureDesc.Usage                = D3D11_USAGE_IMMUTABLE;
    textureDesc.BindFlags            = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA textureData;
    textureData.pSysMem     = image.GetRawData();
    textureData.SysMemPitch = 4 * image.GetDimensions().x;

    HRESULT hr = m_device->CreateTexture2D(&textureDesc, &textureData, &newTexture->m_texture);

    if (!SUCCEEDED(hr))
    {
        ERROR_AND_DIE(Stringf("CreateTextureFromImage failed for image file \"%s\".", image.GetImageFilePath().c_str()))
    }

    hr = m_device->CreateShaderResourceView(newTexture->m_texture, NULL, &newTexture->m_shaderResourceView);

    if (!SUCCEEDED(hr))
    {
        ERROR_AND_DIE(Stringf("CreateShaderResourceView failed for image file \"%s\".", image.GetImageFilePath().c_str()))
    }

    m_loadedTextures.push_back(newTexture);
    return newTexture;
}
