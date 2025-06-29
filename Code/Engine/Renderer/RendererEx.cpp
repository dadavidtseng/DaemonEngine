//----------------------------------------------------------------------------------------------------
// RendererEx.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Renderer/RendererEx.hpp"

#include <d3d11.h>
#include <d3dcompiler.h>
#include <vector>

#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/Image.hpp"
#include "ThirdParty/stb/stb_image.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

RendererEx::RendererEx()
{
    ZeroMemory(&m_bitmapInfo, sizeof(BITMAPINFO));
    m_bitmapInfo.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    m_bitmapInfo.bmiHeader.biWidth       = sceneWidth;
    m_bitmapInfo.bmiHeader.biHeight      = static_cast<LONG>(sceneHeight);
    m_bitmapInfo.bmiHeader.biPlanes      = 1;
    m_bitmapInfo.bmiHeader.biBitCount    = 32;
    m_bitmapInfo.bmiHeader.biCompression = BI_RGB;

    m_pixelData.resize(sceneWidth * sceneHeight * 4);
}

RendererEx::~RendererEx()
{
    Cleanup();
}

void RendererEx::Startup()
{
}


void RendererEx::EndFrame()
{
    ID3D11ShaderResourceView* nullSRV[1] = {nullptr};
    m_deviceContext->PSSetShaderResources(0, 1, nullSRV);
    // 切換到主窗口的後緩衝區
    m_deviceContext->OMSetRenderTargets(1, &m_mainBackBufferRenderTargetView, nullptr);

    // 設置主窗口的視口
    D3D11_VIEWPORT viewport = {};
    viewport.Width          = 1920.0f;  // 或使用您的主窗口寬度
    viewport.Height         = 1080.0f; // 或使用您的主窗口高度
    viewport.MinDepth       = 0.0f;
    viewport.MaxDepth       = 1.0f;
    m_deviceContext->RSSetViewports(1, &viewport);

    // 清除主窗口背景（可選）
    float clearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    m_deviceContext->ClearRenderTargetView(m_mainBackBufferRenderTargetView, clearColor);

    // 將場景紋理渲染到主窗口
    RenderSceneTextureToMainWindow();
    m_mainSwapChain->Present(1, 0);
}

HRESULT RendererEx::Initialize()
{
    // CoInitialize(nullptr);
    mainWindow = CreateWindowEx(
        NULL,
        L"STATIC",
        L"Hidden",
        WS_POPUP,
        0, 0, 1920, 1080,
        nullptr,
        nullptr,
        GetModuleHandle(nullptr),
        nullptr
    );
    ShowWindow(mainWindow, SW_SHOW);
    HRESULT hr = CreateDeviceAndSwapChain();
    if (FAILED(hr)) return hr;


    hr = CreateSceneRenderTexture();
    if (FAILED(hr)) return hr;

    hr = CreateStagingTexture();
    if (FAILED(hr)) return hr;

    // Image const defaultImage(IntVec2(2, 2), Rgba8::WHITE);
    m_defaultTexture         = CreateOrGetTextureFromFile("Data/Images/test.png");
    m_defaultTexture->m_name = "Default";

    hr = CreateShaders();
    if (FAILED(hr)) return hr;

    hr = CreateVertexBuffer();
    if (FAILED(hr)) return hr;

    hr = CreateSampler();
    if (FAILED(hr)) return hr;

    hr = CreateFullscreenShaders();
    if (FAILED(hr)) return hr;

    return S_OK;
}

void RendererEx::ReadStagingTextureToPixelData()
{
    if (!m_stagingTexture || !m_stagingTexture->m_texture) return;

    D3D11_MAPPED_SUBRESOURCE mappedResource;
    HRESULT                  hr = m_deviceContext->Map(m_stagingTexture->m_texture, 0, D3D11_MAP_READ, 0, &mappedResource);

    if (FAILED(hr))
    {
        DebuggerPrintf("Failed to map staging texture: 0x%08X\n", hr);
        return;
    }

    // 复制数据到m_pixelData
    BYTE* srcData  = (BYTE*)mappedResource.pData;
    UINT  srcPitch = mappedResource.RowPitch;

    for (UINT row = 0; row < sceneHeight; ++row)
    {
        BYTE* srcRow = srcData + row * srcPitch;
        BYTE* dstRow = &m_pixelData[row * sceneWidth * 4];

        // 复制一行数据（注意格式可能是BGRA，需要转换为RGBA）
        for (UINT col = 0; col < sceneWidth; ++col)
        {
            BYTE r = srcRow[col * 4 + 0];  // Red
            BYTE g = srcRow[col * 4 + 1];  // Green
            BYTE b = srcRow[col * 4 + 2];  // Blue
            BYTE a = srcRow[col * 4 + 3];  // Alpha

            // 转换为RGBA格式存储到m_pixelData
            dstRow[col * 4 + 0] = r;  // Red
            dstRow[col * 4 + 1] = g;  // Green
            dstRow[col * 4 + 2] = b;  // Blue
            dstRow[col * 4 + 3] = a;  // Alpha
        }
    }

    m_deviceContext->Unmap(m_stagingTexture->m_texture, 0);
}

void RendererEx::Render()
{
    if (!m_sceneRenderTargetView || !m_deviceContext) return;

    // CRITICAL: Unbind scene texture from shader resources before using as render target
    ID3D11ShaderResourceView* nullSRV[1] = {nullptr};
    m_deviceContext->PSSetShaderResources(0, 1, nullSRV);
    m_deviceContext->OMSetRenderTargets(1, &m_sceneRenderTargetView, nullptr);

    D3D11_VIEWPORT viewport = {};
    viewport.Width          = (FLOAT)sceneWidth;
    viewport.Height         = (FLOAT)sceneHeight;
    viewport.MinDepth       = 0.0f;
    viewport.MaxDepth       = 1.0f;
    m_deviceContext->RSSetViewports(1, &viewport);

    float clearColor[4] = {0.1f, 0.1f, 0.2f, 1.0f};
    m_deviceContext->ClearRenderTargetView(m_sceneRenderTargetView, clearColor);

    RenderTexture(m_defaultTexture);
    m_deviceContext->CopyResource(m_stagingTexture->m_texture, m_sceneTexture->m_texture);
    ReadStagingTextureToPixelData();
    // 更新和渲染窗口
    // UpdateWindows(windows);
}

HRESULT RendererEx::CreateDeviceAndSwapChain()
{
    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferCount          = 1;
    swapChainDesc.BufferDesc.Width     = 1920;
    swapChainDesc.BufferDesc.Height    = 1080;
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
    m_sceneTexture = new Texture();

    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width                = sceneWidth;
    texDesc.Height               = sceneHeight;
    texDesc.MipLevels            = 1;
    texDesc.ArraySize            = 1;
    texDesc.Format               = DXGI_FORMAT_B8G8R8A8_UNORM;
    texDesc.SampleDesc.Count     = 1;
    texDesc.Usage                = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags            = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    HRESULT hr = m_device->CreateTexture2D(&texDesc, nullptr, &m_sceneTexture->m_texture);
    if (FAILED(hr)) return hr;

    hr = m_device->CreateRenderTargetView(m_sceneTexture->m_texture, nullptr, &m_sceneRenderTargetView);
    if (FAILED(hr)) return hr;

    hr = m_device->CreateShaderResourceView(m_sceneTexture->m_texture, nullptr, &m_sceneTexture->m_shaderResourceView);
    if (FAILED(hr)) return hr;

    return S_OK;
}

HRESULT RendererEx::CreateStagingTexture()
{
    m_stagingTexture = new Texture();

    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width                = sceneWidth;
    texDesc.Height               = sceneHeight;
    texDesc.MipLevels            = 1;
    texDesc.ArraySize            = 1;
    texDesc.Format               = DXGI_FORMAT_B8G8R8A8_UNORM;
    texDesc.SampleDesc.Count     = 1;
    texDesc.Usage                = D3D11_USAGE_STAGING;
    texDesc.CPUAccessFlags       = D3D11_CPU_ACCESS_READ;

    return m_device->CreateTexture2D(&texDesc, nullptr, &m_stagingTexture->m_texture);
}

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
            // 檢查是否有有效的 UV 坐標（用於區分是否使用紋理）
        if (input.tex.x > 0.001 || input.tex.y > 0.001)
        {
            // 有紋理座標，使用紋理
            float4 texColor = tex.Sample(sam, input.tex);
            return texColor * input.color;
        }
        else
        {
            // 沒有紋理座標，直接使用頂點顏色
            return input.color;
        }
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
    m_vertexList.clear();

    // Fullscreen quad vertices
    m_vertexList.emplace_back(Vec3(-1.f, -1.f, 0.f), Rgba8(255, 255, 255, 255), Vec2(0, 0)); // 左下
    m_vertexList.emplace_back(Vec3(-1.f, 1.f, 0.f), Rgba8(255, 255, 255, 255), Vec2(0, 1)); // 左上
    m_vertexList.emplace_back(Vec3(1.f, 1.f, 0.f), Rgba8(255, 255, 255, 255), Vec2(1, 1)); // 右上
    m_vertexList.emplace_back(Vec3(1.f, -1.f, 0.f), Rgba8(255, 255, 255, 255), Vec2(1, 0)); // 右下

    // AABB2D vertices
    float aabbLeft   = -0.5f;
    float aabbRight  = 0.5f;
    float aabbBottom = -0.3f;
    float aabbTop    = 0.3f;

    m_vertexList.emplace_back(Vec3(aabbLeft, aabbBottom, 0.f), Rgba8(255, 255, 0, 255), Vec2(0, 0));   // 左下
    m_vertexList.emplace_back(Vec3(aabbLeft, aabbTop, 0.f), Rgba8(255, 255, 0, 255), Vec2(0, 0));      // 左上
    m_vertexList.emplace_back(Vec3(aabbRight, aabbTop, 0.f), Rgba8(255, 255, 0, 255), Vec2(0, 0));     // 右上
    m_vertexList.emplace_back(Vec3(aabbRight, aabbBottom, 0.f), Rgba8(255, 255, 0, 255), Vec2(0, 0));  // 右下

    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.Usage             = D3D11_USAGE_DEFAULT;
    bufferDesc.ByteWidth         = m_vertexList.size() * sizeof(Vertex_PCU);
    bufferDesc.BindFlags         = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem                = m_vertexList.data();

    HRESULT hr = m_device->CreateBuffer(&bufferDesc, &initData, &vertexBuffer);
    if (FAILED(hr)) return hr;

    // FIX: Increase index buffer size to accommodate all draw calls
    UINT indices[] = {
        // Fullscreen quad (triangles)
        0, 1, 2,  // First triangle
        0, 2, 3,  // Second triangle

        // AABB2D (triangles) - FIXED: was trying to draw 8 indices but only had 6
        4, 5, 6,  // First triangle
        4, 6, 7   // Second triangle
    };

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

void RendererEx::RenderTexture(Texture* texture)
{
    m_deviceContext->VSSetShader(vertexShader, nullptr, 0);
    m_deviceContext->PSSetShader(pixelShader, nullptr, 0);
    m_deviceContext->IASetInputLayout(inputLayout);

    m_deviceContext->PSSetShaderResources(0, 1, &texture->m_shaderResourceView);
    m_deviceContext->PSSetSamplers(0, 1, &sampler);

    UINT stride = sizeof(Vertex_PCU);
    UINT offset = 0;
    m_deviceContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
    m_deviceContext->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
    m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Draw fullscreen quad (6 indices)
    m_deviceContext->DrawIndexed(6, 0, 0);

    // Draw AABB quad (6 indices, starting from index 6)
    m_deviceContext->DrawIndexed(6, 6, 0);  // FIXED: was 8 indices, now 6
}

void RendererEx::RenderSceneTextureToMainWindow()
{
    // 設置著色器
    m_deviceContext->VSSetShader(vertexShader, nullptr, 0);
    m_deviceContext->PSSetShader(pixelShader, nullptr, 0);
    m_deviceContext->IASetInputLayout(inputLayout);

    // 使用場景紋理作為輸入
    m_deviceContext->PSSetShaderResources(0, 1, &m_sceneTexture->m_shaderResourceView);
    m_deviceContext->PSSetSamplers(0, 1, &sampler);

    // 設置頂點和索引緩衝區
    UINT stride = sizeof(Vertex_PCU);
    UINT offset = 0;
    m_deviceContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
    m_deviceContext->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
    m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // 渲染全螢幕四邊形 (6 indices)
    m_deviceContext->DrawIndexed(6, 0, 0);

    // Draw AABB quad (6 indices, starting from index 6)
    m_deviceContext->DrawIndexed(6, 6, 0);  // FIXED: was 8 indices, now 6
}

// 额外添加一个辅助函数来检查设备状态
HRESULT RendererEx::CheckDeviceStatus()
{
    if (!m_device) return E_FAIL;

    HRESULT hr = m_device->GetDeviceRemovedReason();
    if (hr != S_OK)
    {
        DebuggerPrintf("Device removed/reset detected: 0x%08X\n", hr);
        return hr;
    }

    return S_OK;
}

void RendererEx::UpdateWindows(std::vector<WindowEx>& windows)
{
    for (int i = 0; i < windows.size(); ++i)
    {
        // 先检查是否需要调整SwapChain大小
        if (windows[i].needsResize)
        {
            HRESULT hr             = ResizeWindowSwapChain(windows[i]);
            windows[i].needsResize = false;
            if (FAILED(hr))
            {
                DebuggerPrintf("Failed to resize window swap chain: 0x%08X\n", hr);

                // 如果是设备丢失，可以考虑重新创建设备
                if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
                {
                    DebuggerPrintf("Device lost, may need to recreate device and resources\n");
                    // TODO: 实现设备重新创建逻辑
                }
                continue; // 跳过这个窗口的更新
            }
        }

        if (windows[i].needsUpdate)
        {
            // 使用 DirectX 11 版本渲染
            RenderViewportToWindowDX11(windows[i]);
            // RenderViewportToWindow(windows[i]);
            // window.needsUpdate = false; // 保持注释状态用于调试
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
               &m_pixelData[srcRowIndex * 4],
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
    // if (m_stagingTexture)
    // {
    //     m_stagingTexture->Release();
    //     m_stagingTexture = nullptr;
    // }
    // if (m_sceneShaderResourceView)
    // {
    //     m_sceneShaderResourceView->Release();
    //     m_sceneShaderResourceView = nullptr;
    // }
    if (m_sceneRenderTargetView)
    {
        m_sceneRenderTargetView->Release();
        m_sceneRenderTargetView = nullptr;
    }
    // if (m_sceneTexture)
    // {
    //     m_sceneTexture->Release();
    //     m_sceneTexture = nullptr;
    // }
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

HRESULT RendererEx::CreateWindowSwapChain(WindowEx& window)
{
    RECT clientRect;
    GetClientRect((HWND)window.m_windowHandle, &clientRect);
    window.width  = clientRect.right - clientRect.left;
    window.height = clientRect.bottom - clientRect.top;

    // 創建 swap chain 描述
    DXGI_SWAP_CHAIN_DESC swapChainDesc               = {};
    swapChainDesc.BufferCount                        = 1;
    swapChainDesc.BufferDesc.Width                   = window.width;
    swapChainDesc.BufferDesc.Height                  = window.height;
    swapChainDesc.BufferDesc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferDesc.RefreshRate.Numerator   = 60;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.BufferUsage                        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.OutputWindow                       = (HWND)window.m_windowHandle;
    swapChainDesc.SampleDesc.Count                   = 1;
    swapChainDesc.SampleDesc.Quality                 = 0;
    swapChainDesc.Windowed                           = TRUE;
    swapChainDesc.SwapEffect                         = DXGI_SWAP_EFFECT_DISCARD;

    // 獲取 DXGI Factory
    IDXGIDevice* dxgiDevice = nullptr;
    HRESULT      hr         = m_device->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);
    if (FAILED(hr)) return hr;

    IDXGIAdapter* adapter = nullptr;
    hr                    = dxgiDevice->GetAdapter(&adapter);
    if (FAILED(hr))
    {
        dxgiDevice->Release();
        return hr;
    }

    IDXGIFactory* factory = nullptr;
    hr                    = adapter->GetParent(__uuidof(IDXGIFactory), (void**)&factory);
    if (FAILED(hr))
    {
        adapter->Release();
        dxgiDevice->Release();
        return hr;
    }

    // 創建 swap chain
    hr = factory->CreateSwapChain(m_device, &swapChainDesc, &window.m_swapChain);

    // 清理臨時物件
    factory->Release();
    adapter->Release();
    dxgiDevice->Release();

    if (FAILED(hr)) return hr;

    // 創建 render target view
    ID3D11Texture2D* backBuffer = nullptr;
    hr                          = window.m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
    if (FAILED(hr)) return hr;

    hr = m_device->CreateRenderTargetView(backBuffer, nullptr, &window.m_renderTargetView);
    backBuffer->Release();

    return hr;
}

// 創建全屏四邊形渲染用的著色器
HRESULT RendererEx::CreateFullscreenShaders()
{
    // 全屏四邊形頂點著色器
    const char* fullscreenVS = R"(
        struct VS_INPUT
        {
            float2 pos : POSITION;
            float2 tex : TEXCOORD0;
        };

        struct VS_OUTPUT
        {
            float4 pos : SV_POSITION;
            float2 tex : TEXCOORD0;
        };

        VS_OUTPUT main(VS_INPUT input)
        {
            VS_OUTPUT output;
            output.pos = float4(input.pos, 0.0f, 1.0f);
            output.tex = input.tex;
            return output;
        }
    )";

    // 全屏四邊形像素著色器（支援視窗區域裁切）
    const char* fullscreenPS = R"(
        Texture2D sceneTexture : register(t0);
        SamplerState sceneSampler : register(s0);

        cbuffer ViewportParams : register(b0)
        {
            float2 viewportOffset;  // 視窗在場景中的起始位置 (0-1)
            float2 viewportSize;    // 視窗在場景中的大小 (0-1)
        };

        struct PS_INPUT
        {
            float4 pos : SV_POSITION;
            float2 tex : TEXCOORD0;
        };

        float4 main(PS_INPUT input) : SV_TARGET
        {
            // 將螢幕空間的 UV 座標轉換為場景紋理中的對應區域
            float2 sceneUV = viewportOffset + input.tex * viewportSize;

            // 確保 UV 座標在有效範圍內
            if (sceneUV.x < 0.0 || sceneUV.x > 1.0 || sceneUV.y < 0.0 || sceneUV.y > 1.0)
            {
                return float4(0.0, 0.0, 0.0, 1.0); // 黑色背景
            }

            return sceneTexture.Sample(sceneSampler, sceneUV);
        }
    )";

    // 編譯頂點著色器
    ID3DBlob* vsBlob    = nullptr;
    ID3DBlob* errorBlob = nullptr;
    HRESULT   hr        = D3DCompile(fullscreenVS, strlen(fullscreenVS), nullptr, nullptr, nullptr,
                                     "main", "vs_5_0", 0, 0, &vsBlob, &errorBlob);
    if (FAILED(hr))
    {
        if (errorBlob) errorBlob->Release();
        return hr;
    }

    hr = m_device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
                                      nullptr, &m_fullscreenVS);
    if (FAILED(hr))
    {
        vsBlob->Release();
        return hr;
    }

    // 創建輸入佈局
    D3D11_INPUT_ELEMENT_DESC inputElements[] =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };

    hr = m_device->CreateInputLayout(inputElements, 2, vsBlob->GetBufferPointer(),
                                     vsBlob->GetBufferSize(), &m_fullscreenInputLayout);
    vsBlob->Release();
    if (FAILED(hr)) return hr;

    // 編譯像素著色器
    ID3DBlob* psBlob = nullptr;
    hr               = D3DCompile(fullscreenPS, strlen(fullscreenPS), nullptr, nullptr, nullptr,
                                  "main", "ps_5_0", 0, 0, &psBlob, &errorBlob);
    if (FAILED(hr))
    {
        if (errorBlob) errorBlob->Release();
        return hr;
    }

    hr = m_device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(),
                                     nullptr, &m_fullscreenPS);
    psBlob->Release();
    if (FAILED(hr)) return hr;

    // 創建全屏四邊形頂點緩衝區
    struct FullscreenVertex
    {
        float x, y;    // 位置
        float u, v;    // UV 座標
    };

    FullscreenVertex vertices[] =
    {
        {-1.0f, -1.0f, 0.0f, 1.0f},  // 左下
        {-1.0f, 1.0f, 0.0f, 0.0f},  // 左上
        {1.0f, -1.0f, 1.0f, 1.0f},  // 右下
        {1.0f, 1.0f, 1.0f, 0.0f}   // 右上
    };

    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.Usage             = D3D11_USAGE_DEFAULT;
    bufferDesc.ByteWidth         = sizeof(vertices);
    bufferDesc.BindFlags         = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem                = vertices;

    return m_device->CreateBuffer(&bufferDesc, &initData, &m_fullscreenVertexBuffer);
}

// DirectX 11 版本的視窗渲染
void RendererEx::RenderViewportToWindowDX11(const WindowEx& window) const
{
    if (!window.m_swapChain || !window.m_renderTargetView) return;

    // CRITICAL: Unbind scene texture from shader resources before using window render target
    ID3D11ShaderResourceView* nullSRV[1] = {nullptr};
    m_deviceContext->PSSetShaderResources(0, 1, nullSRV);

    // Set window render target
    m_deviceContext->OMSetRenderTargets(1, &window.m_renderTargetView, nullptr);

    // Set window viewport
    D3D11_VIEWPORT viewport = {};
    viewport.Width          = (float)window.width;
    viewport.Height         = (float)window.height;
    viewport.MinDepth       = 0.0f;
    viewport.MaxDepth       = 1.0f;
    m_deviceContext->RSSetViewports(1, &viewport);

    // Clear window background
    float clearColor[4] = {0.1f, 0.1f, 0.2f, 1.0f};
    m_deviceContext->ClearRenderTargetView(window.m_renderTargetView, clearColor);

    // Set shaders and resources
    m_deviceContext->VSSetShader(m_fullscreenVS, nullptr, 0);
    m_deviceContext->PSSetShader(m_fullscreenPS, nullptr, 0);
    m_deviceContext->IASetInputLayout(m_fullscreenInputLayout);

    // Bind scene texture
    m_deviceContext->PSSetShaderResources(0, 1, &m_sceneTexture->m_shaderResourceView);
    m_deviceContext->PSSetSamplers(0, 1, &sampler);

    // Set viewport parameters constant buffer
    struct ViewportParams
    {
        float viewportOffset[2];
        float viewportSize[2];
    };

    ViewportParams params;
    params.viewportOffset[0] = window.viewportX;
    params.viewportOffset[1] = window.viewportY;
    params.viewportSize[0]   = window.viewportWidth;
    params.viewportSize[1]   = window.viewportHeight;

    // Create constant buffer
    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.Usage             = D3D11_USAGE_DEFAULT;
    cbDesc.ByteWidth         = sizeof(ViewportParams);
    cbDesc.BindFlags         = D3D11_BIND_CONSTANT_BUFFER;

    D3D11_SUBRESOURCE_DATA cbData = {};
    cbData.pSysMem                = &params;

    ID3D11Buffer* constantBuffer = nullptr;
    HRESULT       hr             = m_device->CreateBuffer(&cbDesc, &cbData, &constantBuffer);
    if (SUCCEEDED(hr))
    {
        m_deviceContext->PSSetConstantBuffers(0, 1, &constantBuffer);

        // Set vertex buffer
        UINT stride = sizeof(float) * 4; // 2 for position + 2 for UV
        UINT offset = 0;
        m_deviceContext->IASetVertexBuffers(0, 1, &m_fullscreenVertexBuffer, &stride, &offset);
        m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

        // Render fullscreen quad
        m_deviceContext->Draw(4, 0);

        // Cleanup
        constantBuffer->Release();
    }

    // Present to window
    window.m_swapChain->Present(0, 0);
}

// 添加視窗（DirectX 11 版本）
HRESULT RendererEx::AddWindowDX11(HWND hwnd)
{
    // windows.emplace_back();
    // WindowEx& window = windows.back();
    //
    // window.m_windowHandle = hwnd;
    // window.m_displayContext = GetDC(hwnd); // 保留作為後備
    // window.needsUpdate = true;
    //
    // // 創建 DirectX 11 資源
    // HRESULT hr = CreateWindowSwapChain(window);
    // if (FAILED(hr))
    // {
    //     windows.pop_back();
    //     return hr;
    // }

    return S_OK;
}

// 清理視窗資源
void RendererEx::CleanupWindowResources(WindowEx& window)
{
    if (window.m_renderTargetView)
    {
        window.m_renderTargetView->Release();
        window.m_renderTargetView = nullptr;
    }

    if (window.m_swapChain)
    {
        window.m_swapChain->Release();
        window.m_swapChain = nullptr;
    }

    if (window.m_displayContext)
    {
        ReleaseDC((HWND)window.m_windowHandle, (HDC)window.m_displayContext);
        window.m_displayContext = nullptr;
    }
}

// 添加重新创建SwapChain的方法
HRESULT RendererEx::ResizeWindowSwapChain(WindowEx& window)
{
    if (!window.m_swapChain) return E_FAIL;

    // 1. 超徹底的資源解綁
    // m_deviceContext->OMSetRenderTargets(0, nullptr, nullptr);
    // m_deviceContext->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
    // m_deviceContext->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
    // m_deviceContext->VSSetShader(nullptr, nullptr, 0);
    // m_deviceContext->PSSetShader(nullptr, nullptr, 0);
    // m_deviceContext->PSSetShaderResources(0, 1, nullptr);
    // m_deviceContext->PSSetSamplers(0, 1, nullptr);
    // m_deviceContext->PSSetConstantBuffers(0, 1, nullptr);

    m_deviceContext->ClearState();
    m_deviceContext->Flush();

    // 2. 強制等待 GPU 完成所有操作
    ID3D11Query*     query     = nullptr;
    D3D11_QUERY_DESC queryDesc = {};
    queryDesc.Query            = D3D11_QUERY_EVENT;
    m_device->CreateQuery(&queryDesc, &query);
    m_deviceContext->End(query);

    BOOL queryData = FALSE;
    while (m_deviceContext->GetData(query, &queryData, sizeof(BOOL), 0) == S_FALSE)
    {
        Sleep(1); // 等待 GPU 完成
    }
    query->Release();

    // 3. 釋放 RenderTargetView
    if (window.m_renderTargetView)
    {
        ULONG refCount = window.m_renderTargetView->Release();
        DebuggerPrintf("RTV released, ref count: %lu\n", refCount);

        window.m_renderTargetView = nullptr;
    }


    // 2. Get new window dimensions
    RECT clientRect;
    if (!GetClientRect((HWND)window.m_windowHandle, &clientRect))
    {
        return E_FAIL;
    }

    UINT newWidth  = clientRect.right - clientRect.left;
    UINT newHeight = clientRect.bottom - clientRect.top;

    if (newWidth == 0 || newHeight == 0)
    {
        return E_FAIL;
    }

    // 4. Resize swap chain buffers
    HRESULT hr = window.m_swapChain->ResizeBuffers(
        1,                              // BufferCount
        newWidth,                       // Width
        newHeight,                      // Height
        DXGI_FORMAT_R8G8B8A8_UNORM,    // NewFormat
        0                               // SwapChainFlags
    );

    if (FAILED(hr))
    {
        DebuggerPrintf("ResizeBuffers failed with HRESULT: 0x%08X\n", hr);
        return hr;
    }

    // 5. Recreate RenderTargetView
    ID3D11Texture2D* backBuffer = nullptr;
    hr                          = window.m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
    if (FAILED(hr))
    {
        DebuggerPrintf("Failed to get back buffer: 0x%08X\n", hr);
        return hr;
    }

    hr = m_device->CreateRenderTargetView(backBuffer, nullptr, &window.m_renderTargetView);
    backBuffer->Release(); // Release immediately

    if (FAILED(hr))
    {
        DebuggerPrintf("Failed to create render target view: 0x%08X\n", hr);
        return hr;
    }

    // 6. Update window info
    window.width  = newWidth;
    window.height = newHeight;

    // 7. Recalculate viewport parameters
    RECT windowRect;
    if (GetWindowRect((HWND)window.m_windowHandle, &windowRect))
    {
        window.viewportX      = (float)windowRect.left / (float)window.virtualScreenWidth;
        window.viewportY      = (float)windowRect.top / (float)window.virtualScreenHeight;
        window.viewportWidth  = (float)window.width / (float)window.virtualScreenWidth;
        window.viewportHeight = (float)window.height / (float)window.virtualScreenHeight;

        // Clamp to valid range
        window.viewportX      = max(0.0f, min(1.0f, window.viewportX));
        window.viewportY      = max(0.0f, min(1.0f, window.viewportY));
        window.viewportWidth  = max(0.0f, min(1.0f - window.viewportX, window.viewportWidth));
        window.viewportHeight = max(0.0f, min(1.0f - window.viewportY, window.viewportHeight));
    }

    window.needsUpdate = true;
    window.needsResize = false;

    DebuggerPrintf("Window resized successfully to %dx%d\n", newWidth, newHeight);
    return S_OK;
}
