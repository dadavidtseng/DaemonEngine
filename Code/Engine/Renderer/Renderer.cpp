//----------------------------------------------------------------------------------------------------
// Renderer.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Renderer/Renderer.hpp"

#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <dxgi1_2.h>

#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/Image.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Core/Vertex_PCUTBN.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Platform/Window.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/ConstantBuffer.hpp"
#include "Engine/Renderer/RenderCommon.hpp"
#include "Engine/Renderer/Shader.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"
#include "ThirdParty/stb/stb_image.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

// Define ENGINE_DEBUG_RENDERER if this is a Debug configuration and link debug libraries if it is.
#if defined(_DEBUG)
#define ENGINE_DEBUG_RENDER
#endif

#if defined(ENGINE_DEBUG_RENDER)
#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")
#endif

//----------------------------------------------------------------------------------------------------
STATIC int Renderer::k_perFrameConstantSlot = 1;
STATIC int Renderer::k_lightConstantSlot    = 2;
STATIC int Renderer::k_cameraConstantSlot   = 3;
STATIC int Renderer::k_modelConstantsSlot   = 4;

//----------------------------------------------------------------------------------------------------
Renderer::Renderer(sRenderConfig const& config)
{
    m_config = config;
    // sceneWidth  = Window::s_mainWindow->GetClientDimensions().x;
    // sceneHeight = Window::s_mainWindow->GetClientDimensions().y;
    // sceneWidth  = GetSystemMetrics(SM_CXSCREEN);
    // sceneHeight = GetSystemMetrics(SM_CYSCREEN);

    float screenWidth  = Window::s_mainWindow->GetScreenDimensions().x;
    float screenHeight = Window::s_mainWindow->GetScreenDimensions().y;

    // RendererEx
    ZeroMemory(&m_bitmapInfo, sizeof(BITMAPINFO));
    m_bitmapInfo.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    m_bitmapInfo.bmiHeader.biWidth       = screenWidth;
    m_bitmapInfo.bmiHeader.biHeight      = static_cast<LONG>(screenHeight);
    m_bitmapInfo.bmiHeader.biPlanes      = 1;
    m_bitmapInfo.bmiHeader.biBitCount    = 32;
    m_bitmapInfo.bmiHeader.biCompression = BI_RGB;

    m_pixelData.resize(screenWidth * screenHeight * 4);
}

void Renderer::CreateDeviceAndSwapChain(unsigned int deviceFlags)
{
    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    Window const*        window        = m_config.m_window;

    swapChainDesc.BufferDesc.Width  = window->GetClientDimensions().x;
    swapChainDesc.BufferDesc.Height = window->GetClientDimensions().y;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.SampleDesc.Count  = 1;
    swapChainDesc.BufferUsage       = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount       = 2;
    swapChainDesc.OutputWindow      = static_cast<HWND>(window->GetWindowHandle());
    swapChainDesc.Windowed          = true;
    swapChainDesc.SwapEffect        = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr,
                                               D3D_DRIVER_TYPE_HARDWARE,
                                               nullptr,
                                               deviceFlags,
                                               nullptr,
                                               0,
                                               D3D11_SDK_VERSION,
                                               &swapChainDesc,
                                               &m_swapChain,
                                               &m_device,
                                               nullptr,
                                               &m_deviceContext);

    if (FAILED(hr))
    {
        ERROR_AND_DIE("Could not create D3D 11 device and swap chain.")
    }
}

void Renderer::CreateRenderTargetView()
{
    ID3D11Texture2D* backBuffer;

    HRESULT hr = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer));

    if (FAILED(hr))
    {
        ERROR_AND_DIE("Could not get swap chain buffer.")
    }

    hr = m_device->CreateRenderTargetView(backBuffer, nullptr, &m_renderTargetView);

    if (FAILED(hr))
    {
        ERROR_AND_DIE("Could create render target view for swap chain buffer.")
    }

    backBuffer->Release();
}

void Renderer::CreateBlendStates()
{
    D3D11_BLEND_DESC blendDesc                      = {};
    blendDesc.RenderTarget[0].BlendEnable           = TRUE;
    blendDesc.RenderTarget[0].SrcBlend              = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlend             = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOp               = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha         = blendDesc.RenderTarget[0].SrcBlend;
    blendDesc.RenderTarget[0].DestBlendAlpha        = blendDesc.RenderTarget[0].DestBlend;
    blendDesc.RenderTarget[0].BlendOpAlpha          = blendDesc.RenderTarget[0].BlendOp;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    HRESULT hr = m_device->CreateBlendState(&blendDesc, &m_blendStates[static_cast<int>(eBlendMode::OPAQUE)]);
    if (!SUCCEEDED(hr))
    {
        ERROR_AND_DIE("CreateBlendState for BlendMode::OPAQUE failed.")
    }

    blendDesc.RenderTarget[0].SrcBlend  = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;

    hr = m_device->CreateBlendState(&blendDesc, &m_blendStates[static_cast<int>(eBlendMode::ALPHA)]);
    if (!SUCCEEDED(hr))
    {
        ERROR_AND_DIE("CreateBlendState for BlendMode::ALPHA failed.")
    }

    blendDesc.RenderTarget[0].SrcBlend  = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;

    hr = m_device->CreateBlendState(&blendDesc, &m_blendStates[static_cast<int>(eBlendMode::ADDITIVE)]);
    if (!SUCCEEDED(hr))
    {
        ERROR_AND_DIE("CreateBlendState for BlendMode::ADDITIVE failed.")
    }
}

void Renderer::CreateDepthStencilTextureAndView()
{
    D3D11_TEXTURE2D_DESC depthTextureDesc = {};
    depthTextureDesc.Width                = m_config.m_window->GetClientDimensions().x;
    depthTextureDesc.Height               = m_config.m_window->GetClientDimensions().y;
    depthTextureDesc.MipLevels            = 1;
    depthTextureDesc.ArraySize            = 1;
    depthTextureDesc.Usage                = D3D11_USAGE_DEFAULT;
    depthTextureDesc.Format               = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthTextureDesc.BindFlags            = D3D11_BIND_DEPTH_STENCIL;
    depthTextureDesc.SampleDesc.Count     = 1;

    HRESULT hr = m_device->CreateTexture2D(&depthTextureDesc, nullptr, &m_depthStencilTexture);

    if (!SUCCEEDED(hr))
    {
        ERROR_AND_DIE("Could not create texture for depth stencil.")
    }

    hr = m_device->CreateDepthStencilView(m_depthStencilTexture, nullptr, &m_depthStencilDSV);

    if (!SUCCEEDED(hr))
    {
        ERROR_AND_DIE("Could not create depth stencil view.")
    }
}

void Renderer::CreateDepthStencilState()
{
    D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};

    HRESULT hr = m_device->CreateDepthStencilState(&depthStencilDesc, &m_depthStencilStates[static_cast<int>(eDepthMode::DISABLED)]);

    if (!SUCCEEDED(hr))
    {
        ERROR_AND_DIE("CreateDepthStencilState for DepthMode::DISABLED failed.")
    }

    // For the rest you need to enable depth.
    depthStencilDesc.DepthEnable    = true;
    depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    depthStencilDesc.DepthFunc      = D3D11_COMPARISON_ALWAYS;

    hr = m_device->CreateDepthStencilState(&depthStencilDesc, &m_depthStencilStates[static_cast<int>(eDepthMode::READ_ONLY_ALWAYS)]);

    if (!SUCCEEDED(hr))
    {
        ERROR_AND_DIE("CreateDepthStencilState for DepthMode::READ_ONLY_ALWAYS failed.")
    }

    depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    depthStencilDesc.DepthFunc      = D3D11_COMPARISON_LESS_EQUAL;

    hr = m_device->CreateDepthStencilState(&depthStencilDesc, &m_depthStencilStates[static_cast<int>(eDepthMode::READ_ONLY_LESS_EQUAL)]);

    if (!SUCCEEDED(hr))
    {
        ERROR_AND_DIE("CreateDepthStencilState for DepthMode::READ_ONLY_LESS_EQUAL failed.")
    }

    depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    depthStencilDesc.DepthFunc      = D3D11_COMPARISON_LESS_EQUAL;

    hr = m_device->CreateDepthStencilState(&depthStencilDesc, &m_depthStencilStates[static_cast<int>(eDepthMode::READ_WRITE_LESS_EQUAL)]);

    if (!SUCCEEDED(hr))
    {
        ERROR_AND_DIE("CreateDepthStencilState for DepthMode::READ_WRITE_LESS_EQUAL failed.")
    }

    SetDepthMode(eDepthMode::READ_WRITE_LESS_EQUAL);
}

void Renderer::CreateSamplerState()
{
    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter             = D3D11_FILTER_MIN_MAG_MIP_POINT;
    samplerDesc.AddressU           = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressV           = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressW           = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.ComparisonFunc     = D3D11_COMPARISON_NEVER;
    samplerDesc.MaxLOD             = D3D11_FLOAT32_MAX;

    HRESULT hr = m_device->CreateSamplerState(&samplerDesc, &m_samplerStates[static_cast<int>(eSamplerMode::POINT_CLAMP)]);

    if (!SUCCEEDED(hr))
    {
        ERROR_AND_DIE("CreateSamplerState for SamplerMode::POINT_CLAMP failed.")
    }

    samplerDesc.Filter   = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;

    hr = m_device->CreateSamplerState(&samplerDesc, &m_samplerStates[static_cast<int>(eSamplerMode::BILINEAR_CLAMP)]);
    if (!SUCCEEDED(hr))
    {
        ERROR_AND_DIE("CreateSamplerState for SamplerMode::BILINEAR_CLAMP failed.")
    }

    // Default the sampler state to point clamp
    SetSamplerMode(eSamplerMode::POINT_CLAMP);
}

void Renderer::CreateRasterizerState()
{
    D3D11_RASTERIZER_DESC rasterizerDesc;
    rasterizerDesc.FillMode              = D3D11_FILL_SOLID;
    rasterizerDesc.CullMode              = D3D11_CULL_NONE;
    rasterizerDesc.FrontCounterClockwise = true;
    rasterizerDesc.DepthBias             = 0;
    rasterizerDesc.DepthBiasClamp        = 0.f;
    rasterizerDesc.SlopeScaledDepthBias  = 0.f;
    rasterizerDesc.DepthClipEnable       = true;
    rasterizerDesc.ScissorEnable         = false;
    rasterizerDesc.MultisampleEnable     = false;
    rasterizerDesc.AntialiasedLineEnable = true;

    HRESULT hr = m_device->CreateRasterizerState(&rasterizerDesc, &m_rasterizerStates[static_cast<int>(eRasterizerMode::SOLID_CULL_NONE)]);
    if (FAILED(hr))
    {
        ERROR_AND_DIE("CreateRasterizerState for RasterizerMode::SOLID_CULL_NONE failed.")
    }

    rasterizerDesc.CullMode = D3D11_CULL_BACK;

    hr = m_device->CreateRasterizerState(&rasterizerDesc, &m_rasterizerStates[static_cast<int>(eRasterizerMode::SOLID_CULL_BACK)]);
    if (FAILED(hr))
    {
        ERROR_AND_DIE("CreateRasterizerState for RasterizerMode::SOLID_CULL_BACK failed.")
    }

    rasterizerDesc.FillMode = D3D11_FILL_WIREFRAME;
    rasterizerDesc.CullMode = D3D11_CULL_NONE;

    hr = m_device->CreateRasterizerState(&rasterizerDesc, &m_rasterizerStates[static_cast<int>(eRasterizerMode::WIREFRAME_CULL_NONE)]);
    if (FAILED(hr))
    {
        ERROR_AND_DIE("CreateRasterizerState for RasterizerMode::WIREFRAME_CULL_NONE failed.")
    }

    rasterizerDesc.CullMode = D3D11_CULL_BACK;

    hr = m_device->CreateRasterizerState(&rasterizerDesc, &m_rasterizerStates[static_cast<int>(eRasterizerMode::WIREFRAME_CULL_BACK)]);
    if (FAILED(hr))
    {
        ERROR_AND_DIE("CreateRasterizerState for RasterizerMode::WIREFRAME_CULL_BACK failed.")
    }
}

//----------------------------------------------------------------------------------------------------
void Renderer::Startup()
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


    CreateDeviceAndSwapChain(deviceFlags);
    CreateRenderTargetView();
    CreateBlendStates();
    CreateDepthStencilTextureAndView();
    CreateDepthStencilState();
    CreateSamplerState();
    CreateRasterizerState();


    m_immediateVBO_PCU    = CreateVertexBuffer(sizeof(Vertex_PCU), sizeof(Vertex_PCU));
    m_immediateVBO_PCUTBN = CreateVertexBuffer(sizeof(Vertex_PCUTBN), sizeof(Vertex_PCUTBN));
    m_immediateIBO        = CreateIndexBuffer(sizeof(Vertex_PCU), sizeof(Vertex_PCU));
    m_lightCBO            = CreateConstantBuffer(sizeof(sLightConstants));
    m_cameraCBO           = CreateConstantBuffer(sizeof(sCameraConstants));
    m_modelCBO            = CreateConstantBuffer(sizeof(sModelConstants));
    m_perFrameCBO         = CreateConstantBuffer(sizeof(sPerFrameConstants));

    //------------------------------------------------------------------------------------------------
    // Initialize m_defaultTexture to a 2x2 white image
    Image const defaultImage(IntVec2(2, 2), Rgba8::WHITE);
    m_defaultTexture         = CreateTextureFromImage(defaultImage);
    m_defaultTexture->m_name = "Default";
    m_defaultShader          = CreateOrGetShaderFromFile("Data/Shaders/Default", eVertexType::VERTEX_PCU);
    m_currentShader          = CreateOrGetShaderFromFile("Data/Shaders/Default", eVertexType::VERTEX_PCU);
    // m_currentShader = CreateShader("Default", DEFAULT_SHADER_SOURCE);

    BindShader(m_defaultShader);
    BindTexture(m_defaultTexture, 0);
}

//----------------------------------------------------------------------------------------------------
void Renderer::BeginFrame() const
{
    m_deviceContext->OMSetRenderTargets(1, &m_renderTargetView, m_depthStencilDSV);
}

//----------------------------------------------------------------------------------------------------
void Renderer::EndFrame() const
{
    bool constexpr isVSync = false;
    HRESULT const  hr      = m_swapChain->Present(isVSync, 0);

    if (hr == DXGI_ERROR_DEVICE_REMOVED ||
        hr == DXGI_ERROR_DEVICE_RESET)
    {
        ERROR_AND_DIE("Device has been lost, application will now terminate.")
    }
}

//----------------------------------------------------------------------------------------------------
void Renderer::Shutdown()
{
    for (int i = 0; i < static_cast<int>(m_loadedShaders.size()); ++i)
    {
        ENGINE_SAFE_RELEASE(m_loadedShaders[i]);
    }

    for (int i = 0; i < static_cast<int>(m_loadedTextures.size()); ++i)
    {
        ENGINE_SAFE_RELEASE(m_loadedTextures[i]);
    }

    m_loadedFonts.clear();
    m_loadedShaders.clear();
    m_loadedTextures.clear();

    ENGINE_SAFE_RELEASE(m_perFrameCBO);
    ENGINE_SAFE_RELEASE(m_modelCBO);
    ENGINE_SAFE_RELEASE(m_lightCBO);
    ENGINE_SAFE_RELEASE(m_cameraCBO);
    ENGINE_SAFE_RELEASE(m_immediateIBO);
    ENGINE_SAFE_RELEASE(m_immediateVBO_PCUTBN);
    ENGINE_SAFE_RELEASE(m_immediateVBO_PCU);

    // Release all DirectX objects and check for memory leaks in your Shutdown function.
    // Release all rasterizer states
    for (int i = 0; i < static_cast<int>(eRasterizerMode::COUNT); ++i)
    {
        DX_SAFE_RELEASE(m_rasterizerStates[i])
    }

    // Release all sampler states
    for (int i = 0; i < static_cast<int>(eSamplerMode::COUNT); ++i)
    {
        DX_SAFE_RELEASE(m_samplerStates[i])
    }

    // Release all depth states
    for (int i = 0; i < static_cast<int>(eDepthMode::COUNT); ++i)
    {
        DX_SAFE_RELEASE(m_depthStencilStates[i])
    }

    DX_SAFE_RELEASE(m_depthStencilDSV)
    DX_SAFE_RELEASE(m_depthStencilTexture)

    // Release all blend states
    for (int i = 0; i < static_cast<int>(eBlendMode::COUNT); ++i)
    {
        DX_SAFE_RELEASE(m_blendStates[i])
    }

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
}

void Renderer::Render()
{
    ReadStagingTextureToPixelData();
}

//----------------------------------------------------------------------------------------------------
void Renderer::ClearScreen(Rgba8 const& clearColor) const
{
    float colorAsFloats[4];
    clearColor.GetAsFloats(colorAsFloats);
    m_deviceContext->ClearRenderTargetView(m_renderTargetView, colorAsFloats);
    m_deviceContext->ClearDepthStencilView(m_depthStencilDSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

//----------------------------------------------------------------------------------------------------
void Renderer::BeginCamera(Camera const& camera) const
{
    // Set viewport
    D3D11_VIEWPORT viewport;
    //Window const*  window       = m_config.m_window;
    AABB2 const& viewportAABB = camera.GetViewPortUnnormalized(Vec2::ZERO);
    viewport.TopLeftX         = viewportAABB.m_mins.x;
    viewport.TopLeftY         = viewportAABB.m_mins.y;
    // viewport.TopLeftX = 0.f;
    // viewport.TopLeftY = 0.f;
    viewport.Width  = viewportAABB.m_maxs.x - viewportAABB.m_mins.x;
    viewport.Height = viewportAABB.m_maxs.y - viewportAABB.m_mins.y;

    // viewport.Width    = static_cast<float>(window->GetClientDimensions().x);
    // viewport.Height   = static_cast<float>(window->GetClientDimensions().y);
    viewport.MinDepth = 0.f;
    viewport.MaxDepth = 1.f;

    m_deviceContext->RSSetViewports(1, &viewport);


    // Create a local CameraConstants structure
    sCameraConstants cameraConstants;

    cameraConstants.WorldToCameraTransform  = camera.GetWorldToCameraTransform();
    cameraConstants.CameraToRenderTransform = camera.GetCameraToRenderTransform();
    cameraConstants.RenderToClipTransform   = camera.GetRenderToClipTransform();
    cameraConstants.CameraWorldPosition[0]  = camera.GetPosition().x;
    cameraConstants.CameraWorldPosition[1]  = camera.GetPosition().y;
    cameraConstants.CameraWorldPosition[2]  = camera.GetPosition().z;

    // Copy the data from the local structure to the constant buffer
    CopyCPUToGPU(&cameraConstants, sizeof(sCameraConstants), m_cameraCBO);

    // Bind the constant buffer
    BindConstantBuffer(k_cameraConstantSlot, m_cameraCBO);

    // LightConstants lightConstants;
    //
    // Vec3 const sunDirection = Vec3(2.f, 1.f, -1.f).GetNormalized();
    //
    // lightConstants.SunDirection[0]  = sunDirection.x;
    // lightConstants.SunDirection[1]  = sunDirection.y;
    // lightConstants.SunDirection[2]  = sunDirection.z;
    // lightConstants.SunIntensity     = 0.85f;
    // lightConstants.AmbientIntensity = 0.35f;
    //
    // CopyCPUToGPU(&lightConstants, sizeof(LightConstants), m_lightCBO);
    // BindConstantBuffer(k_lightConstantSlot, m_lightCBO);

    // Set model constants to default
    SetModelConstants();
    // SetLightConstants();
    // SetPerFrameConstants();
}

//----------------------------------------------------------------------------------------------------
void Renderer::EndCamera(Camera const& camera)
{
    UNUSED(camera)
}

//----------------------------------------------------------------------------------------------------
void Renderer::DrawVertexArray(int const         numVertexes,
                               Vertex_PCU const* vertexes)
{
    CopyCPUToGPU(vertexes, numVertexes * sizeof(Vertex_PCU), m_immediateVBO_PCU);
    DrawVertexBuffer(m_immediateVBO_PCU, numVertexes);
}

//----------------------------------------------------------------------------------------------------
void Renderer::DrawVertexArray(int const            numVertexes,
                               Vertex_PCUTBN const* vertexes)
{
    CopyCPUToGPU(vertexes, numVertexes * sizeof(Vertex_PCUTBN), m_immediateVBO_PCUTBN);
    DrawVertexBuffer(m_immediateVBO_PCUTBN, numVertexes);
}

//----------------------------------------------------------------------------------------------------
void Renderer::DrawVertexArray(VertexList_PCU const& verts)
{
    DrawVertexArray(static_cast<int>(verts.size()), verts.data());
}

//----------------------------------------------------------------------------------------------------
void Renderer::DrawVertexArray(VertexList_PCUTBN const& verts)
{
    DrawVertexArray(static_cast<int>(verts.size()), verts.data());
}

//----------------------------------------------------------------------------------------------------
void Renderer::DrawVertexArray(VertexList_PCU const& verts,
                               IndexList const&      indexes)
{
    CopyCPUToGPU(verts.data(), static_cast<int>(verts.size()) * sizeof(Vertex_PCU), m_immediateVBO_PCU);
    CopyCPUToGPU(indexes.data(), static_cast<int>(indexes.size()) * sizeof(unsigned int), m_immediateIBO);
    DrawIndexedVertexBuffer(m_immediateVBO_PCU, m_immediateIBO, static_cast<int>(indexes.size()));
}

//----------------------------------------------------------------------------------------------------
void Renderer::DrawVertexArray(VertexList_PCUTBN const& verts,
                               IndexList const&         indexes)
{
    CopyCPUToGPU(verts.data(), static_cast<int>(verts.size()) * sizeof(Vertex_PCUTBN), m_immediateVBO_PCUTBN);
    CopyCPUToGPU(indexes.data(), static_cast<int>(indexes.size()) * sizeof(unsigned int), m_immediateIBO);
    DrawIndexedVertexBuffer(m_immediateVBO_PCUTBN, m_immediateIBO, static_cast<int>(indexes.size()));
}

//----------------------------------------------------------------------------------------------------
// TODO: BindTexture(Texture const* texture, int slot=1);
void Renderer::BindTexture(Texture const* texture,
                           int const      slot) const
{
    if (texture == nullptr)
    {
        texture = m_defaultTexture;
    }

    m_deviceContext->PSSetShaderResources(slot, 1, &texture->m_shaderResourceView);
}

//----------------------------------------------------------------------------------------------------
void Renderer::BindShader(Shader const* shader) const
{
    if (shader == nullptr)
    {
        shader = m_defaultShader;
    }

    m_deviceContext->VSSetShader(shader->m_vertexShader, nullptr, 0);
    m_deviceContext->PSSetShader(shader->m_pixelShader, nullptr, 0);
    m_deviceContext->IASetInputLayout(shader->m_inputLayout);
}

//----------------------------------------------------------------------------------------------------
void Renderer::DrawTexturedQuad(AABB2 const&   bounds,
                                Texture const* texture,
                                Rgba8 const&   tint,
                                float const    uniformScaleXY,
                                float const    rotationDegreesAboutZ)
{
    VertexList_PCU quadVerts;

    AddVertsForAABB2D(quadVerts, bounds, tint);

    TransformVertexArrayXY3D(static_cast<int>(quadVerts.size()), quadVerts.data(),
                             uniformScaleXY, rotationDegreesAboutZ, Vec2(0, 0));

    BindTexture(texture);
    DrawVertexArray(static_cast<int>(quadVerts.size()), quadVerts.data());
}

//----------------------------------------------------------------------------------------------------
Texture* Renderer::CreateOrGetTextureFromFile(char const* imageFilePath)
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
BitmapFont* Renderer::CreateOrGetBitmapFontFromFile(char const* bitmapFontFilePathWithNoExtension)
{
    if (BitmapFont* existingBitMapFont = GetBitMapFontForFileName(bitmapFontFilePathWithNoExtension))
    {
        return existingBitMapFont;
    }

    String const   textureFilePath = Stringf("%s.png", bitmapFontFilePathWithNoExtension);
    Texture const* newTexture      = CreateOrGetTextureFromFile(textureFilePath.c_str());

    if (!newTexture)
    {
        // Handle error: texture could not be created or retrieved
        return nullptr;
    }

    // Consider using smart pointers for better memory management
    BitmapFont* newBitMapFont = new BitmapFont(bitmapFontFilePathWithNoExtension, *newTexture, IntVec2(16, 16));
    m_loadedFonts.push_back(newBitMapFont);

    return newBitMapFont;
}

Shader* Renderer::CreateOrGetShaderFromFile(char const*       shaderFilePath,
                                            eVertexType const vertexType)
{
    if (Shader* existingShader = GetShaderForFileName(shaderFilePath))
    {
        return existingShader;
    }

    // String const   textureFilePath = Stringf("%s.hlsl", shaderName);
    // Texture const* newTexture      = CreateOrGetTextureFromFile(textureFilePath.c_str());
    //
    // if (!newTexture)
    // {
    //     // Handle error: texture could not be created or retrieved
    //     return nullptr;
    // }

    // Consider using smart pointers for better memory management
    Shader* newShader = CreateShader(shaderFilePath, vertexType);
    m_loadedShaders.push_back(newShader);

    return newShader;
}

//----------------------------------------------------------------------------------------------------
void Renderer::SetBlendMode(eBlendMode const mode)
{
    m_desiredBlendMode = mode;
}

//----------------------------------------------------------------------------------------------------
void Renderer::SetSamplerMode(eSamplerMode const mode)
{
    m_desiredSamplerMode = mode;
}

//----------------------------------------------------------------------------------------------------
void Renderer::SetRasterizerMode(eRasterizerMode const mode)
{
    m_desiredRasterizerMode = mode;
}

//----------------------------------------------------------------------------------------------------
void Renderer::SetDepthMode(eDepthMode const mode)
{
    m_desiredDepthMode = mode;
}

//----------------------------------------------------------------------------------------------------
void Renderer::SetLightConstants(std::vector<Light*> const& lights,
                                 int const                  numLights) const
{
    sLightConstants lightConstants = {};

    // // 設定太陽光
    // float colorAsFloat[4];
    // lightColor.GetAsFloats(colorAsFloat);
    // lightConstants.LightColor[0] = colorAsFloat[0];
    // lightConstants.LightColor[1] = colorAsFloat[1];
    // lightConstants.LightColor[2] = colorAsFloat[2];
    // lightConstants.LightColor[3] = colorAsFloat[3];
    //
    // Vec3 const normalizedSunDirection = sunDirection.GetNormalized();
    // lightConstants.SunDirection[0]    = normalizedSunDirection.x;
    // lightConstants.SunDirection[1]    = normalizedSunDirection.y;
    // lightConstants.SunDirection[2]    = normalizedSunDirection.z;
    // lightConstants.AmbientIntensity   = ambientIntensity;

    lightConstants.NumLights = numLights;

    for (int i = 0; i < numLights && i < MAX_LIGHTS; ++i)
    {
        if (lights[i] != nullptr)
        {
            lightConstants.lightArray[i] = *lights[i];  // 複製 Light 實例的值
        }
    }


    // lightConstants.lightArray[0] = light1;
    // lightConstants.lightArray[1] = light2;

    CopyCPUToGPU(&lightConstants, sizeof(sLightConstants), m_lightCBO);
    BindConstantBuffer(k_lightConstantSlot, m_lightCBO);
}

//----------------------------------------------------------------------------------------------------
void Renderer::SetModelConstants(Mat44 const& modelToWorldTransform,
                                 Rgba8 const& modelColor) const
{
    sModelConstants modelConstants;
    modelConstants.ModelToWorldTransform = modelToWorldTransform;

    float colorAsFloat[4];
    modelColor.GetAsFloats(colorAsFloat);

    modelConstants.ModelColor[0] = colorAsFloat[0];
    modelConstants.ModelColor[1] = colorAsFloat[1];
    modelConstants.ModelColor[2] = colorAsFloat[2];
    modelConstants.ModelColor[3] = colorAsFloat[3];

    CopyCPUToGPU(&modelConstants, sizeof(sModelConstants), m_modelCBO);
    BindConstantBuffer(k_modelConstantsSlot, m_modelCBO);
}

//----------------------------------------------------------------------------------------------------
void Renderer::SetPerFrameConstants(float const time,
                                    int const   debugInt,
                                    float const debugFloat) const
{
    sPerFrameConstants perFrameConstants;
    perFrameConstants.c_time       = time;
    perFrameConstants.c_debugInt   = debugInt;
    perFrameConstants.c_debugFloat = debugFloat;

    CopyCPUToGPU(&perFrameConstants, sizeof(sPerFrameConstants), m_perFrameCBO);
    BindConstantBuffer(k_perFrameConstantSlot, m_perFrameCBO);
}


//----------------------------------------------------------------------------------------------------
void Renderer::DrawVertexBuffer(VertexBuffer const* vbo,
                                unsigned int const  vertexCount)
{
    BindVertexBuffer(vbo);
    SetStatesIfChanged();

    // Draw
    m_deviceContext->Draw(vertexCount, 0);
}

//----------------------------------------------------------------------------------------------------
void Renderer::DrawIndexedVertexBuffer(VertexBuffer const* vbo,
                                       IndexBuffer const*  ibo,
                                       unsigned int const  indexCount)
{
    BindVertexBuffer(vbo);
    BindIndexBuffer(ibo);
    SetStatesIfChanged();

    // Draw
    m_deviceContext->DrawIndexed(indexCount, 0, 0);
}

//----------------------------------------------------------------------------------------------------
Texture* Renderer::GetTextureForFileName(char const* imageFilePath) const
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
BitmapFont* Renderer::GetBitMapFontForFileName(const char* bitmapFontFilePathWithNoExtension) const
{
    for (BitmapFont* font : m_loadedFonts)
    {
        if (font && !strcmp(font->m_fontFilePathNameWithNoExtension.c_str(), bitmapFontFilePathWithNoExtension))
        {
            return font;
        }
    }

    return nullptr;
}

//----------------------------------------------------------------------------------------------------
Shader* Renderer::GetShaderForFileName(char const* shaderFilePath) const
{
    for (Shader* shader : m_loadedShaders)
    {
        if (shader && !strcmp(shader->GetName().c_str(), shaderFilePath))
        {
            return shader;
        }
    }

    return nullptr;
}

//----------------------------------------------------------------------------------------------------
Texture* Renderer::CreateTextureFromFile(char const* imageFilePath)
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
Texture* Renderer::CreateTextureFromData(char const*    name,
                                         IntVec2 const& dimensions,
                                         int const      bytesPerTexel,
                                         uint8_t const* texelData)
{
    // Check if the load was successful
    GUARANTEE_OR_DIE(texelData,
                     Stringf("CreateTextureFromData failed for \"%s\" - texelData was null!",
                         name))

    GUARANTEE_OR_DIE(bytesPerTexel >= 3 && bytesPerTexel <= 4,
                     Stringf("CreateTextureFromData failed for \"%s\" - unsupported BPP=%i (must be 3 or 4)",
                         name,
                         bytesPerTexel))

    GUARANTEE_OR_DIE(dimensions.x > 0 && dimensions.y > 0,
                     Stringf("CreateTextureFromData failed for \"%s\" - illegal texture dimensions (%i x %i)",
                         name,
                         dimensions.x, dimensions.y))

    Texture* newTexture      = new Texture();
    newTexture->m_name       = name;            // NOTE: m_name must be a std::string, otherwise it may point to temporary data!
    newTexture->m_dimensions = dimensions;

    return newTexture;
}

//----------------------------------------------------------------------------------------------------
Texture* Renderer::CreateTextureFromImage(Image const& image)
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

//----------------------------------------------------------------------------------------------------
Image Renderer::CreateImageFromFile(char const* imageFilePath)
{
    return Image(imageFilePath);
}

//----------------------------------------------------------------------------------------------------
Shader* Renderer::CreateShader(char const*       shaderName,
                               char const*       shaderSource,
                               eVertexType const vertexType)
{
    ShaderConfig shaderConfig;
    shaderConfig.m_name = shaderName;
    Shader* shader      = new Shader(shaderConfig);

    std::vector<uint8_t> vertexShaderByteCode;
    std::vector<uint8_t> pixelShaderByteCode;

    CompileShaderToByteCode(vertexShaderByteCode, shaderName, shaderSource, shader->m_config.m_vertexEntryPoint.c_str(), "vs_5_0");

    // Create vertex shader
    HRESULT hr = m_device->CreateVertexShader(
        vertexShaderByteCode.data(),
        vertexShaderByteCode.size(),
        nullptr,
        &shader->m_vertexShader);

    if (!SUCCEEDED(hr))
    {
        ERROR_AND_DIE(Stringf("Could not create vertex shader."))
    }

    CompileShaderToByteCode(pixelShaderByteCode, shaderName, shaderSource, shader->m_config.m_pixelEntryPoint.c_str(), "ps_5_0");

    // Create pixel shader
    hr = m_device->CreatePixelShader(
        pixelShaderByteCode.data(),
        pixelShaderByteCode.size(),
        nullptr,
        &shader->m_pixelShader);

    if (!SUCCEEDED(hr))
    {
        ERROR_AND_DIE(Stringf("Could not create pixel shader."))
    }

    // Create a local array of input element descriptions that defines the vertex layout.
    D3D11_INPUT_ELEMENT_DESC inputElementDesc[6];
    UINT                     numElements = 0;

    if (vertexType == eVertexType::VERTEX_PCU)
    {
        inputElementDesc[0] = {"VERTEX_POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0};
        inputElementDesc[1] = {"VERTEX_COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0};
        inputElementDesc[2] = {"VERTEX_UVTEXCOORDS", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0};
        numElements         = 3;
    }
    else if (vertexType == eVertexType::VERTEX_PCUTBN)
    {
        inputElementDesc[0] = {"VERTEX_POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0};
        inputElementDesc[1] = {"VERTEX_COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0};
        inputElementDesc[2] = {"VERTEX_UVTEXCOORDS", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0};
        inputElementDesc[3] = {"VERTEX_TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0};
        inputElementDesc[4] = {"VERTEX_BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0};
        inputElementDesc[5] = {"VERTEX_NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0};
        numElements         = 6;
    }

    hr = m_device->CreateInputLayout(
        inputElementDesc,
        numElements,
        vertexShaderByteCode.data(),
        vertexShaderByteCode.size(),
        &shader->m_inputLayout);

    if (!SUCCEEDED(hr))
    {
        ERROR_AND_DIE(Stringf("Could not create input layout."))
    }

    return shader;
}

//----------------------------------------------------------------------------------------------------
Shader* Renderer::CreateShader(char const*       shaderName,
                               eVertexType const vertexType)
{
    // Append the .hlsl extension to the shader name
    String const shaderFileName = Stringf("%s.hlsl", shaderName);

    String shaderSource;

    if (!FileReadToString(shaderSource, shaderFileName))
    {
        ERROR_AND_DIE(Stringf("Failed to read shader file: %s", shaderFileName.c_str()))
    }

    // Create the shader using the file contents
    return CreateShader(shaderName, shaderSource.c_str(), vertexType);
}

//----------------------------------------------------------------------------------------------------
bool Renderer::CompileShaderToByteCode(std::vector<unsigned char>& out_byteCode,
                                       char const*                 name,
                                       char const*                 source,
                                       char const*                 entryPoint,
                                       char const*                 target)
{
    // Compile vertex shader
    DWORD shaderFlags = D3DCOMPILE_OPTIMIZATION_LEVEL3;
#if defined(ENGINE_DEBUG_RENDER)
    shaderFlags = D3DCOMPILE_DEBUG;
    shaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
    shaderFlags |= D3DCOMPILE_WARNINGS_ARE_ERRORS;
#endif
    ID3DBlob* shaderBlob = nullptr;
    ID3DBlob* errorBlob  = nullptr;

    HRESULT const hr = D3DCompile(source,
                                  strlen(source),
                                  name,
                                  nullptr,
                                  nullptr,
                                  entryPoint,
                                  target,
                                  shaderFlags,
                                  0,
                                  &shaderBlob,
                                  &errorBlob);

    if (SUCCEEDED(hr))
    {
        out_byteCode.resize(shaderBlob->GetBufferSize());
        memcpy(
            out_byteCode.data(),
            shaderBlob->GetBufferPointer(),
            shaderBlob->GetBufferSize());
    }
    else
    {
        if (errorBlob)
        {
            DebuggerPrintf(static_cast<char*>(errorBlob->GetBufferPointer()));
        }
        ERROR_AND_DIE(Stringf("Could not compile shader."))
    }

    shaderBlob->Release();

    if (errorBlob)
    {
        errorBlob->Release();
    }

    return SUCCEEDED(hr);
    // // Compile pixel shader
    // hr = D3DCompile(
    //     shaderSource,
    //     strlen(shaderSource),
    //     "PixelShader",
    //     nullptr,
    //     nullptr,
    //     "PixelMain",
    //     "ps_5_0",
    //     shaderFlags,
    //     0,
    //     &shaderBlob,
    //     &errorBlob);
    //
    // if (SUCCEEDED(hr))
    // {
    //     outByteCode.resize(shaderBlob->GetBufferSize());
    //     memcpy(
    //         outByteCode.data(),
    //         shaderBlob->GetBufferPointer(),
    //         shaderBlob->GetBufferSize());
    // }
    // else
    // {
    //     if (!errorBlob)
    //     {
    //         DebuggerPrintf(static_cast<char*>(errorBlob->GetBufferPointer()));
    //     }
    //     ERROR_AND_DIE(Stringf("Could not compile pixel shader."))
    // }
    //
    // shaderBlob->Release();
    //
    // if (errorBlob)
    // {
    //     errorBlob->Release();
    // }
}

//----------------------------------------------------------------------------------------------------
VertexBuffer* Renderer::CreateVertexBuffer(unsigned int const size,
                                           unsigned int const stride) const
{
    VertexBuffer* vertexBuffer = new VertexBuffer(m_device, size, stride);

    return vertexBuffer;
}

//----------------------------------------------------------------------------------------------------
IndexBuffer* Renderer::CreateIndexBuffer(unsigned int const size,
                                         unsigned int const stride) const
{
    IndexBuffer* indexBuffer = new IndexBuffer(m_device, size, stride);

    return indexBuffer;
}

//----------------------------------------------------------------------------------------------------
void Renderer::CopyCPUToGPU(void const*        data,
                            unsigned int const size,
                            VertexBuffer*      vbo) const
{
    // Check if the vertex buffer is large enough to hold the data
    if (vbo->GetSize() < size)
    {
        vbo->Resize(size);
    }

    // Map the buffer
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    HRESULT const            hr = m_deviceContext->Map(vbo->m_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);

    if (!SUCCEEDED(hr))
    {
        ERROR_AND_DIE("Failed to map vertex buffer.")
    }

    // Copy the data
    memcpy(mappedResource.pData, data, size);

    // Unmap the buffer
    m_deviceContext->Unmap(vbo->m_buffer, 0);
}

//----------------------------------------------------------------------------------------------------
void Renderer::CopyCPUToGPU(void const*        data,
                            unsigned int const size,
                            IndexBuffer*       ibo) const
{
    // Check if the index buffer is large enough to hold the data
    if (ibo->GetSize() < size)
    {
        ibo->Resize(size);
    }

    // Map the buffer
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    HRESULT const            hr = m_deviceContext->Map(ibo->m_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);

    if (!SUCCEEDED(hr))
    {
        ERROR_AND_DIE("Failed to map index buffer.")
    }

    // Copy the data
    memcpy(mappedResource.pData, data, size);

    // Unmap the buffer
    m_deviceContext->Unmap(ibo->m_buffer, 0);
}

//----------------------------------------------------------------------------------------------------
void Renderer::BindVertexBuffer(VertexBuffer const* vbo) const
{
    // Bind the vertex buffer
    UINT const     stride = vbo->GetStride();
    UINT constexpr offset = 0;

    m_deviceContext->IASetVertexBuffers(0,
                                        1,
                                        &vbo->m_buffer,
                                        &stride,
                                        &offset);

    // Set the primitive topology
    m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

//----------------------------------------------------------------------------------------------------
void Renderer::BindIndexBuffer(IndexBuffer const* ibo) const
{
    // Bind the vertex buffer
    m_deviceContext->IASetIndexBuffer(ibo->m_buffer,
                                      DXGI_FORMAT_R32_UINT,
                                      0);
}

//----------------------------------------------------------------------------------------------------
ConstantBuffer* Renderer::CreateConstantBuffer(unsigned int const size) const
{
    ConstantBuffer* constantBuffer = new ConstantBuffer(m_device, size);

    return constantBuffer;
}

//----------------------------------------------------------------------------------------------------
void Renderer::CopyCPUToGPU(void const* data, unsigned int const size, ConstantBuffer* cbo) const
{
    // Check if the constant buffer is large enough to hold the data
    if (cbo->GetSize() < size)
    {
        cbo->Resize(size);
    }

    // Map the buffer
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    HRESULT const            hr = m_deviceContext->Map(cbo->m_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);

    if (!SUCCEEDED(hr))
    {
        ERROR_AND_DIE("Failed to map constant buffer.")
    }

    // Copy the data
    memcpy(mappedResource.pData, data, size);

    // Unmap the buffer
    m_deviceContext->Unmap(cbo->m_buffer, 0);
}

//----------------------------------------------------------------------------------------------------
void Renderer::BindConstantBuffer(int const slot, ConstantBuffer const* cbo) const
{
    m_deviceContext->VSSetConstantBuffers(slot, 1, &cbo->m_buffer);
    m_deviceContext->PSSetConstantBuffers(slot, 1, &cbo->m_buffer);
}

//----------------------------------------------------------------------------------------------------
void Renderer::SetStatesIfChanged()
{
    if (m_blendState != m_blendStates[static_cast<int>(m_desiredBlendMode)])
    {
        m_blendState                   = m_blendStates[static_cast<int>(m_desiredBlendMode)];
        float constexpr blendFactor[4] = {0.f, 0.f, 0.f, 0.f};
        UINT constexpr  sampleMask     = 0xffffffff;

        m_deviceContext->OMSetBlendState(m_blendState, blendFactor, sampleMask);
    }

    if (m_depthStencilState != m_depthStencilStates[static_cast<int>(m_desiredDepthMode)])
    {
        m_depthStencilState = m_depthStencilStates[static_cast<int>(m_desiredDepthMode)];

        m_deviceContext->OMSetDepthStencilState(m_depthStencilState, 0);
    }

    if (m_rasterizerStates[static_cast<int>(m_desiredRasterizerMode)] != m_rasterizerState)
    {
        m_rasterizerState = m_rasterizerStates[static_cast<int>(m_desiredRasterizerMode)];
        m_deviceContext->RSSetState(m_rasterizerStates[static_cast<int>(m_desiredRasterizerMode)]);
    }

    if (m_samplerStates[static_cast<int>(m_desiredSamplerMode)] != m_samplerState)
    {
        m_samplerState = m_samplerStates[static_cast<int>(m_desiredSamplerMode)];
        m_deviceContext->PSSetSamplers(0, 1, &m_samplerState);
        m_deviceContext->PSSetSamplers(1, 1, &m_samplerState);
        m_deviceContext->PSSetSamplers(2, 1, &m_samplerState);
    }
}

void Renderer::ReadStagingTextureToPixelData()
{
    // 1. 取得主 Render Target 的描述
    ID3D11Texture2D* mainRenderTargetTexture = nullptr;
    m_renderTargetView->GetResource((ID3D11Resource**)&mainRenderTargetTexture);
    D3D11_TEXTURE2D_DESC desc = {};
    mainRenderTargetTexture->GetDesc(&desc);

    // 2. 建立 staging texture（CPU 可讀）
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    desc.Usage          = D3D11_USAGE_STAGING;
    desc.BindFlags      = 0;
    desc.MiscFlags      = 0;

    ID3D11Texture2D* stagingTex = nullptr;
    HRESULT          hr         = m_device->CreateTexture2D(&desc, nullptr, &stagingTex);
    if (FAILED(hr))
    {
        DebuggerPrintf("Failed to create staging texture: 0x%08X\n", hr);
        return;
    }

    // 3. 複製 render target 資料到 staging texture
    m_deviceContext->CopyResource(stagingTex, mainRenderTargetTexture);

    // 4. Map staging texture
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    hr = m_deviceContext->Map(stagingTex, 0, D3D11_MAP_READ, 0, &mappedResource);
    if (FAILED(hr))
    {
        DebuggerPrintf("Failed to map staging texture: 0x%08X\n", hr);
        stagingTex->Release();
        return;
    }

    // 5. 讀取像素資料
    BYTE* srcData  = static_cast<BYTE*>(mappedResource.pData);
    UINT  srcPitch = mappedResource.RowPitch;

    m_pixelData.resize(desc.Width * desc.Height * 4); // RGBA 每個像素 4 bytes

    for (UINT row = 0; row < desc.Height; ++row)
    {
        BYTE* srcRow = srcData + row * srcPitch;
        BYTE* dstRow = &m_pixelData[row * desc.Width * 4];

        for (UINT col = 0; col < desc.Width; ++col)
        {
            // 注意格式根據你的 render target 設定，這裡假設是 DXGI_FORMAT_R8G8B8A8_UNORM
            BYTE b = srcRow[col * 4 + 0];
            BYTE g = srcRow[col * 4 + 1];
            BYTE r = srcRow[col * 4 + 2];
            BYTE a = srcRow[col * 4 + 3];

            dstRow[col * 4 + 0] = r;
            dstRow[col * 4 + 1] = g;
            dstRow[col * 4 + 2] = b;
            dstRow[col * 4 + 3] = a;
        }
    }

    // 6. Unmap + release
    m_deviceContext->Unmap(stagingTex, 0);
    stagingTex->Release();
    mainRenderTargetTexture->Release();
}

void Renderer::RenderViewportToWindow(Window const& window)
{
    if (!window.m_displayContext) return;
    // ReadStagingTextureToPixelData();
    // 取得子視窗在螢幕上的實際客戶區域位置
    RECT windowRect, clientRect;
    GetWindowRect((HWND)window.m_windowHandle, &windowRect);
    GetClientRect((HWND)window.m_windowHandle, &clientRect);

    // 將客戶區域的左上角轉換為螢幕座標
    POINT clientTopLeft = {0, 0};
    ClientToScreen((HWND)window.m_windowHandle, &clientTopLeft);

    // 計算客戶區域相對於視窗左上角的偏移
    int clientOffsetX = clientTopLeft.x - windowRect.left;  // 左邊框寬度
    int clientOffsetY = clientTopLeft.y - windowRect.top;   // 標題列高度 + 上邊框

    // 取得主視窗的位置（假設您有主視窗的控制代碼）
    RECT mainWindowRect;
    GetWindowRect((HWND)Window::s_mainWindow->m_windowHandle, &mainWindowRect);

    // 計算子視窗客戶區域在主視窗中的相對位置
    int relativeX = clientTopLeft.x - mainWindowRect.left;
    int relativeY = clientTopLeft.y - mainWindowRect.top;

    // 取得子視窗客戶區域大小
    int clientWidth  = clientRect.right - clientRect.left;
    int clientHeight = clientRect.bottom - clientRect.top;

    float screenWidth  = Window::s_mainWindow->GetScreenDimensions().x;
    float screenHeight = Window::s_mainWindow->GetScreenDimensions().y;

    // 計算在場景紋理中的區域（使用實際的相對位置）
    int srcX      = max(0, min(relativeX, (int)screenWidth - 1));
    int srcY      = max(0, min(relativeY, (int)screenHeight - 1));
    int srcWidth  = min(clientWidth, (int)screenWidth - srcX);
    int srcHeight = min(clientHeight, (int)screenHeight - srcY);

    // 確保有效的複製區域
    if (srcWidth <= 0 || srcHeight <= 0) return;

    // 創建臨時的 DIB 數據
    std::vector<BYTE> windowPixels(srcWidth * srcHeight * 4);

    // 從場景數據複製指定區域
    for (int y = 0; y < srcHeight; y++)
    {
        int srcRowIndex = (srcY + y) * screenWidth + srcX;
        int dstRowIndex = y * srcWidth;

        memcpy(&windowPixels[dstRowIndex * 4],
               &m_pixelData[srcRowIndex * 4],
               srcWidth * 4);
    }

    // 設置 DIB 信息
    BITMAPINFO localBitmapInfo         = m_bitmapInfo;
    localBitmapInfo.bmiHeader.biWidth  = srcWidth;
    localBitmapInfo.bmiHeader.biHeight = -srcHeight;

    // 使用 StretchDIBits 來縮放顯示到整個客戶區域
    StretchDIBits(
        (HDC)window.m_displayContext,
        0, 0,                          // 目標位置（客戶區域左上角）
        clientWidth, clientHeight,     // 目標大小（客戶區域大小）
        0, 0,                          // 源起始位置
        srcWidth, srcHeight,           // 源大小
        windowPixels.data(),           // 像素數據
        &localBitmapInfo,              // DIB 信息
        DIB_RGB_COLORS,                // 顏色模式
        SRCCOPY                        // 複製模式
    );
}

void Renderer::RenderViewportToWindowDX11(Window& window)
{
    if (!window.m_swapChain || !window.m_renderTargetView) return;

    // 取得子視窗在螢幕上的實際客戶區域位置
    RECT windowRect, clientRect;
    GetWindowRect((HWND)window.m_windowHandle, &windowRect);
    GetClientRect((HWND)window.m_windowHandle, &clientRect);

    // 將客戶區域的左上角轉換為螢幕座標
    POINT clientTopLeft = {0, 0};
    ClientToScreen((HWND)window.m_windowHandle, &clientTopLeft);

    // 計算客戶區域相對於視窗左上角的偏移
    int clientOffsetX = clientTopLeft.x - windowRect.left;  // 左邊框寬度
    int clientOffsetY = clientTopLeft.y - windowRect.top;   // 標題列高度 + 上邊框

    // 取得主視窗的位置（假設您有主視窗的控制代碼）
    RECT mainWindowRect;
    GetWindowRect((HWND)Window::s_mainWindow->m_windowHandle, &mainWindowRect);  // 需要主視窗控制代碼

    // 計算子視窗客戶區域在主視窗中的相對位置
    int relativeX = clientTopLeft.x - mainWindowRect.left;
    int relativeY = clientTopLeft.y - mainWindowRect.top;

    // 1. 從主視窗 RenderTarget 複製指定區域到子視窗
    D3D11_BOX sourceBox;
    sourceBox.left   = (UINT)relativeX;
    sourceBox.top    = (UINT)relativeY;
    sourceBox.right  = (UINT)(relativeX + clientRect.right);
    sourceBox.bottom = (UINT)(relativeY + clientRect.bottom);
    sourceBox.front  = 0;
    sourceBox.back   = 1;

    // 獲取子視窗的 texture
    ID3D11Texture2D* windowTexture = nullptr;
    window.m_renderTargetView->GetResource((ID3D11Resource**)&windowTexture);
    ID3D11Texture2D* mainRenderTargetTexture = nullptr;
    m_renderTargetView->GetResource((ID3D11Resource**)&mainRenderTargetTexture);

    // 從主 RenderTarget 複製到子視窗
    m_deviceContext->CopySubresourceRegion(
        windowTexture,          // 目標
        0,                      // 目標子資源
        0, 0, 0,               // 目標位置
        mainRenderTargetTexture, // 來源（主視窗的 texture）
        0,                      // 來源子資源
        &sourceBox             // 來源區域
    );

    windowTexture->Release();
    mainRenderTargetTexture->Release();
    // window.m_shouldUpdatePosition = false;
    // 2. Present 子視窗
    window.m_swapChain->Present(0, 0);
}

HRESULT Renderer::ResizeWindowSwapChain(Window& window) const
{
    if (!window.m_swapChain) return E_FAIL;

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
        2,                              // BufferCount
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
    window.m_windowDimensions.x = newWidth;
    window.m_windowDimensions.y = newHeight;

    // 7. Recalculate viewport parameters
    RECT windowRect;
    if (GetWindowRect((HWND)window.m_windowHandle, &windowRect))
    {
        window.m_viewportPosition.x   = (float)windowRect.left / (float)window.GetScreenDimensions().x;
        window.m_viewportPosition.y   = (float)windowRect.top / (float)window.GetScreenDimensions().y;
        window.m_viewportDimensions.x = (float)window.m_windowDimensions.x / (float)window.GetScreenDimensions().x;
        window.m_viewportDimensions.y = (float)window.m_windowDimensions.y / (float)window.GetScreenDimensions().y;

        // Clamp to valid range
        window.m_viewportPosition.x   = max(0.0f, min(1.0f, window.m_viewportPosition.x));
        window.m_viewportPosition.y   = max(0.0f, min(1.0f, window.m_viewportPosition.y ));
        window.m_viewportDimensions.x = max(0.0f, min(1.0f - window.m_viewportPosition.x, window.m_viewportDimensions.x));
        window.m_viewportDimensions.y = max(0.0f, min(1.0f - window.m_viewportPosition.y, window.m_viewportDimensions.y));
    }

    window.m_shouldUpdateDimension = false;

    DebuggerPrintf("Window resized successfully to %dx%d\n", newWidth, newHeight);
    return S_OK;
}

HRESULT Renderer::CreateWindowSwapChain(Window& window)
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

    // 強制釋放 deferred 資源
    m_deviceContext->ClearState();
    m_deviceContext->Flush();

    RECT clientRect;
    GetClientRect((HWND)window.m_windowHandle, &clientRect);
    window.m_windowDimensions.x = clientRect.right - clientRect.left;
    window.m_windowDimensions.y = clientRect.bottom - clientRect.top;

    // 獲取 DXGI Factory2（注意這裡會用到 IDXGIFactory2 而非舊的 IDXGIFactory）
    IDXGIDevice* dxgiDevice = nullptr;
    HRESULT      hr         = m_device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice));
    if (FAILED(hr)) return hr;

    IDXGIAdapter* adapter = nullptr;
    hr                    = dxgiDevice->GetAdapter(&adapter);
    if (FAILED(hr))
    {
        dxgiDevice->Release();
        return hr;
    }

    IDXGIFactory2* factory2 = nullptr;
    hr                      = adapter->GetParent(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(&factory2));
    adapter->Release();
    dxgiDevice->Release();
    if (FAILED(hr)) return hr;

    // 使用 DXGI_SWAP_CHAIN_DESC1（新版描述）
    DXGI_SWAP_CHAIN_DESC1 scDesc = {};
    scDesc.Width                 = window.m_windowDimensions.x;
    scDesc.Height                = window.m_windowDimensions.y;
    scDesc.Format                = DXGI_FORMAT_R8G8B8A8_UNORM;
    scDesc.SampleDesc.Count      = 1;
    scDesc.SampleDesc.Quality    = 0;
    scDesc.BufferUsage           = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scDesc.BufferCount           = 2;  // Flip 模式需要至少 2
    scDesc.Scaling               = DXGI_SCALING_STRETCH;
    scDesc.SwapEffect            = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    scDesc.AlphaMode             = DXGI_ALPHA_MODE_UNSPECIFIED;
    scDesc.Stereo                = FALSE;

    // fullscreen 描述（我們通常使用 Windowed 模式，設為 nullptr 即可）
    DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsDesc = {};
    fsDesc.Windowed                        = TRUE;

    // 建立 swap chain
    hr = factory2->CreateSwapChainForHwnd(
        m_device,
        (HWND)window.m_windowHandle,
        &scDesc,
        &fsDesc,
        nullptr,
        &window.m_swapChain
    );
    factory2->Release();
    if (FAILED(hr)) return hr;

    // render target view
    ID3D11Texture2D* backBuffer = nullptr;
    hr                          = window.m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
    if (FAILED(hr)) return hr;

    hr = m_device->CreateRenderTargetView(backBuffer, nullptr, &window.m_renderTargetView);
    backBuffer->Release();

    return hr;
}
