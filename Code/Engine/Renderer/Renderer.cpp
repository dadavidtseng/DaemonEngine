//----------------------------------------------------------------------------------------------------
// Renderer.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Renderer/Renderer.hpp"

#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi.h>

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
#include "Engine/Renderer/Shader.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/Renderer/RenderCommon.hpp"
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
STATIC int Renderer::k_lightConstantSlot  = 1;
STATIC int Renderer::k_cameraConstantSlot = 2;
STATIC int Renderer::k_modelConstantsSlot = 3;

//----------------------------------------------------------------------------------------------------
Renderer::Renderer(sRenderConfig const& config)
{
    m_config = config;
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

    //-Create device and swap chain-------------------------------------------------------------------
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
    //-Create device and swap chain-------------------------------------------------------------------

    //-Get back buffer texture------------------------------------------------------------------------
    ID3D11Texture2D* backBuffer;
    hr = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer));

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
    //-Get back buffer texture------------------------------------------------------------------------

    //-Create blend states and store the state in m_blendState----------------------------------------
    D3D11_BLEND_DESC blendDesc                      = {};
    blendDesc.RenderTarget[0].BlendEnable           = TRUE;
    blendDesc.RenderTarget[0].SrcBlend              = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlend             = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOp               = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha         = blendDesc.RenderTarget[0].SrcBlend;
    blendDesc.RenderTarget[0].DestBlendAlpha        = blendDesc.RenderTarget[0].DestBlend;
    blendDesc.RenderTarget[0].BlendOpAlpha          = blendDesc.RenderTarget[0].BlendOp;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    hr = m_device->CreateBlendState(&blendDesc, &m_blendStates[static_cast<int>(eBlendMode::OPAQUE)]);
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
    //-Create blend states and store the state in m_blendState----------------------------------------

    //-Create depth stencil texture and view----------------------------------------------------------
    D3D11_TEXTURE2D_DESC depthTextureDesc = {};
    depthTextureDesc.Width                = m_config.m_window->GetClientDimensions().x;
    depthTextureDesc.Height               = m_config.m_window->GetClientDimensions().y;
    depthTextureDesc.MipLevels            = 1;
    depthTextureDesc.ArraySize            = 1;
    depthTextureDesc.Usage                = D3D11_USAGE_DEFAULT;
    depthTextureDesc.Format               = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthTextureDesc.BindFlags            = D3D11_BIND_DEPTH_STENCIL;
    depthTextureDesc.SampleDesc.Count     = 1;

    hr = m_device->CreateTexture2D(&depthTextureDesc, nullptr, &m_depthStencilTexture);

    if (!SUCCEEDED(hr))
    {
        ERROR_AND_DIE("Could not create texture for depth stencil.")
    }

    hr = m_device->CreateDepthStencilView(m_depthStencilTexture, nullptr, &m_depthStencilDSV);

    if (!SUCCEEDED(hr))
    {
        ERROR_AND_DIE("Could not create depth stencil view.")
    }
    //-Create depth stencil texture and view----------------------------------------------------------

    //-Create depth stencil state---------------------------------------------------------------------
    D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};

    hr = m_device->CreateDepthStencilState(&depthStencilDesc, &m_depthStencilStates[static_cast<int>(eDepthMode::DISABLED)]);

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
    //-Create depth stencil state---------------------------------------------------------------------

    //-Create sampler states--------------------------------------------------------------------------
    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter             = D3D11_FILTER_MIN_MAG_MIP_POINT;
    samplerDesc.AddressU           = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressV           = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressW           = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.ComparisonFunc     = D3D11_COMPARISON_NEVER;
    samplerDesc.MaxLOD             = D3D11_FLOAT32_MAX;

    hr = m_device->CreateSamplerState(&samplerDesc, &m_samplerStates[static_cast<int>(eSamplerMode::POINT_CLAMP)]);

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
    //-Create sampler states--------------------------------------------------------------------------

    //-Set rasterizer state---------------------------------------------------------------------------
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

    hr = m_device->CreateRasterizerState(&rasterizerDesc, &m_rasterizerStates[static_cast<int>(eRasterizerMode::SOLID_CULL_NONE)]);
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
    //-Set rasterizer state---------------------------------------------------------------------------

    m_immediateVBO_PCU    = CreateVertexBuffer(sizeof(Vertex_PCU), sizeof(Vertex_PCU));
    m_immediateVBO_PCUTBN = CreateVertexBuffer(sizeof(Vertex_PCUTBN), sizeof(Vertex_PCUTBN));
    m_immediateIBO        = CreateIndexBuffer(sizeof(Vertex_PCU), sizeof(Vertex_PCU));
    m_lightCBO            = CreateConstantBuffer(sizeof(sLightConstants));
    m_cameraCBO           = CreateConstantBuffer(sizeof(sCameraConstants));
    m_modelCBO            = CreateConstantBuffer(sizeof(sModelConstants));

    //------------------------------------------------------------------------------------------------
    // Initialize m_defaultTexture to a 2x2 white image
    Image const defaultImage(IntVec2(2, 2), Rgba8::WHITE);
    m_defaultTexture         = CreateTextureFromImage(defaultImage);
    m_defaultTexture->m_name = "Default";
    m_defaultShader          = CreateOrGetShaderFromFile("Data/Shaders/Default", eVertexType::VERTEX_PCU);
    m_currentShader          = CreateOrGetShaderFromFile("Data/Shaders/Default", eVertexType::VERTEX_PCU);
    // m_currentShader = CreateShader("Default", DEFAULT_SHADER_SOURCE);

    BindShader(m_currentShader);
    BindTexture(m_defaultTexture);
}


//----------------------------------------------------------------------------------------------------
void Renderer::BeginFrame() const
{
    // TODO: BindDefaultRenderTarget();
    // This code needs to run every frame and should be in your Render function.
    // m_config.m_window->m_renderTargetView[0] = m_renderTargetView;
    // m_config.m_window->m_renderTargetView[1] = m_renderTargetView;
    m_deviceContext->OMSetRenderTargets(1, &m_renderTargetView, m_depthStencilDSV);
    // m_deviceContext->OMSetRenderTargets(1, &m_config.m_window->m_renderTargetView[0], m_depthStencilDSV);
    // m_deviceContext->OMSetRenderTargets(1, &m_config.m_window->m_renderTargetView[1], m_depthStencilDSV);
}

//----------------------------------------------------------------------------------------------------
void Renderer::EndFrame() const
{
    HRESULT const hr = m_swapChain->Present(0, 0);

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
void Renderer::BindTexture(Texture const* texture) const
{
    if (texture == nullptr)
    {
        texture = m_defaultTexture;
    }

    m_deviceContext->PSSetShaderResources(0, 1, &texture->m_shaderResourceView);
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
void Renderer::SetLightConstants(Vec3 const& sunDirection,
                                 float const sunIntensity,
                                 float const ambientIntensity) const
{
    sLightConstants lightConstants;

    Vec3 const normalizedSunDirection = sunDirection.GetNormalized();

    lightConstants.SunDirection[0]  = normalizedSunDirection.x;
    lightConstants.SunDirection[1]  = normalizedSunDirection.y;
    lightConstants.SunDirection[2]  = normalizedSunDirection.z;
    lightConstants.SunIntensity     = sunIntensity;
    lightConstants.AmbientIntensity = ambientIntensity;

    CopyCPUToGPU(&lightConstants, sizeof(sLightConstants), m_lightCBO);
    BindConstantBuffer(k_lightConstantSlot, m_lightCBO);
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
        inputElementDesc[0] = {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0};
        inputElementDesc[1] = {"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0};
        inputElementDesc[2] = {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0};
        numElements         = 3;
    }
    else if (vertexType == eVertexType::VERTEX_PCUTBN)
    {
        inputElementDesc[0] = {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0};
        inputElementDesc[1] = {"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0};
        inputElementDesc[2] = {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0};
        inputElementDesc[3] = {"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0};
        inputElementDesc[4] = {"BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0};
        inputElementDesc[5] = {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0};
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
    }
}
