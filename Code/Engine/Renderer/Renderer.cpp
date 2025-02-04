//----------------------------------------------------------------------------------------------------
// Renderer.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Renderer/Renderer.hpp"

#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/DefaultShader.hpp"
#include "Engine/Renderer/Shader.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Renderer/Window.hpp"
#include "ThirdParty/stb/stb_image.h"

#define WIN32_LEAN_AND_MEAN			// Always #define this before #including <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi.h>
#include <windows.h>

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
const char* sourceShader = R"(
struct vs_input_t
{
    float3 localPosition : POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
};

struct v2p_t
{
    float4 position : SV_Position;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
};

v2p_t VertexMain(vs_input_t input)
{
    v2p_t v2p;
    v2p.position = float4(input.localPosition, 1);
    v2p.color = input.color;
    v2p.uv = input.uv;
    return v2p;
}

float4 PixelMain(v2p_t input) : SV_Target0
{
    return float4(input.color);
})";

//----------------------------------------------------------------------------------------------------
Renderer::Renderer(RenderConfig const& render_config)
{
    m_config = render_config;
}

//-----------------------------------------------------------------------------------------------
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

    // Create device and swap chain
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

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr,
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

    if (!SUCCEEDED(hr))
    {
        ERROR_AND_DIE("Could not create D3D 11 device and swap chain.")
    }

    // Get back buffer texture
    ID3D11Texture2D* backBuffer;
    hr = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer));

    if (!SUCCEEDED(hr))
    {
        ERROR_AND_DIE("Could not get swap chain buffer.")
    }

    hr = m_device->CreateRenderTargetView(backBuffer, nullptr, &m_renderTargetView);

    if (!SUCCEEDED(hr))
    {
        ERROR_AND_DIE("Could create render target view for swap chain buffer.")
    }

    backBuffer->Release();

    // Set rasterizer state
    D3D11_RASTERIZER_DESC rasterizerDesc;
    rasterizerDesc.FillMode              = D3D11_FILL_SOLID;
    rasterizerDesc.CullMode              = D3D11_CULL_NONE;
    rasterizerDesc.FrontCounterClockwise = false;
    rasterizerDesc.DepthBias             = 0;
    rasterizerDesc.DepthBiasClamp        = 0.f;
    rasterizerDesc.SlopeScaledDepthBias  = 0.f;
    rasterizerDesc.DepthClipEnable       = true;
    rasterizerDesc.ScissorEnable         = false;
    rasterizerDesc.MultisampleEnable     = false;
    rasterizerDesc.AntialiasedLineEnable = true;

    hr = m_device->CreateRasterizerState(&rasterizerDesc, &m_rasterizerState);

    if (!SUCCEEDED(hr))
    {
        ERROR_AND_DIE("Could not create rasterizer state.")
    }

    m_deviceContext->RSSetState(m_rasterizerState);

    m_currentShader = CreateShader("Default", DEFAULT_SHADER_SOURCE);
    BindShader(m_currentShader);

    // Create the immediate vertex buffer with an initial size for one Vertex_PCU
    m_immediateVBO = CreateVertexBuffer(sizeof(Vertex_PCU), sizeof(Vertex_PCU));


    // Test FileReadToBuffer()
    String const filename = "Data/test.txt";

    std::vector<uint8_t> buffer;

    if (FileReadToBuffer(buffer, filename))
    {
        DebuggerPrintf("FileReadToBuffer succeeded, bytes read: %zd", buffer.size());
    }

    // Test FileReadToString()
    String content;
    if (FileReadToString(content, filename))
    {
        // Print the content of the file if reading is successful
        DebuggerPrintf("FileReadToString succeeded, content: %s\n", content.c_str());
    }
    else
    {
        // Handle failure (e.g., file couldn't be read)
        DebuggerPrintf("Failed to read the file.\n");
    }
}

//-----------------------------------------------------------------------------------------------
void Renderer::BeginFrame()
{
    // This code needs to run every frame and should be in your Render function.
    m_deviceContext->OMSetRenderTargets(1, &m_renderTargetView, nullptr);
}

//-----------------------------------------------------------------------------------------------
void Renderer::EndFrame() const
{
    HRESULT const hr = m_swapChain->Present(0, 0);

    if (hr == DXGI_ERROR_DEVICE_REMOVED ||
        hr == DXGI_ERROR_DEVICE_RESET)
    {
        ERROR_AND_DIE("Device has been lost, application will now terminate.")
    }
}

//-----------------------------------------------------------------------------------------------
void Renderer::Shutdown()
{
    // Release all DirectX objects and check for memory leaks in your Shutdown function.
    DX_SAFE_RELEASE(m_rasterizerState)
    DX_SAFE_RELEASE(m_renderTargetView)
    DX_SAFE_RELEASE(m_swapChain)
    DX_SAFE_RELEASE(m_deviceContext)
    DX_SAFE_RELEASE(m_device)

    // Delete the immediate vertex buffer
    if (m_immediateVBO)
    {
        delete m_immediateVBO;
        m_immediateVBO = nullptr;
    }

    if (m_currentShader)
    {
        delete m_currentShader;
        m_currentShader = nullptr;
    }

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

//-----------------------------------------------------------------------------------------------
void Renderer::ClearScreen(Rgba8 const& clearColor)
{
    // Clear the screen
    float colorAsFloats[4];
    clearColor.GetAsFloats(colorAsFloats);
    m_deviceContext->ClearRenderTargetView(m_renderTargetView, colorAsFloats);
}

//-----------------------------------------------------------------------------------------------
void Renderer::BeginCamera(Camera const& camera)
{
    UNUSED(camera)

    // Set viewport
    D3D11_VIEWPORT viewport;
    Window const*  window = m_config.m_window;

    viewport.TopLeftX = 0.f;
    viewport.TopLeftY = 0.f;
    viewport.Width    = static_cast<float>(window->GetClientDimensions().x);
    viewport.Height   = static_cast<float>(window->GetClientDimensions().y);
    viewport.MinDepth = 0.f;
    viewport.MaxDepth = 1.f;

    m_deviceContext->RSSetViewports(1, &viewport);
}

//-----------------------------------------------------------------------------------------------
void Renderer::EndCamera(Camera const& camera)
{
    UNUSED(camera)
}

//-----------------------------------------------------------------------------------------------
void Renderer::DrawVertexArray(int const numVertexes, Vertex_PCU const* vertexes) const
{
    CopyCPUToGPU(vertexes, numVertexes * sizeof(Vertex_PCU), m_immediateVBO);
    DrawVertexBuffer(m_immediateVBO, numVertexes);
}

//-----------------------------------------------------------------------------------------------
void Renderer::BindTexture(Texture const* texture)
{
    if (texture)
    {
        // Crushing the app if m_openglTextureID is 0xFFFFFFFF
        // glEnable(GL_TEXTURE_2D);
        // glBindTexture(GL_TEXTURE_2D, texture->m_openglTextureID);
    }
    else
    {
        // Bind the texture with vertices color
        // glDisable(GL_TEXTURE_2D);
    }
}

//----------------------------------------------------------------------------------------------------
void Renderer::DrawTexturedQuad(AABB2 const&   bounds,
                                Texture const* texture,
                                Rgba8 const&   tint,
                                float const    uniformScaleXY,
                                float const    rotationDegreesAboutZ)
{
    VertexList quadVerts;

    AddVertsForAABB2D(quadVerts, bounds, tint);

    TransformVertexArrayXY3D(static_cast<int>(quadVerts.size()), quadVerts.data(),
                             uniformScaleXY, rotationDegreesAboutZ, Vec2(0, 0));

    BindTexture(texture);
    DrawVertexArray(static_cast<int>(quadVerts.size()), quadVerts.data());
}

//------------------------------------------------------------------------------------------------
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
    BitmapFont* existingBitMapFont = GetBitMapFontForFileName(bitmapFontFilePathWithNoExtension);

    if (existingBitMapFont)
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

    return newBitMapFont;
}

//----------------------------------------------------------------------------------------------------
void Renderer::SetBlendMode(BlendMode const mode)
{
    if (mode == BlendMode::ALPHA) // enum class BlendMode, defined near top of Renderer.hpp
    {
        // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    else if (mode == BlendMode::ADDITIVE)
    {
        // glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    }
    else
    {
        ERROR_AND_DIE(Stringf( "Unknown / unsupported blend mode #%i", mode ))
    }
}

//----------------------------------------------------------------------------------------------------
void Renderer::DrawVertexBuffer(VertexBuffer const* vbo, unsigned int const vertexCount) const
{
    BindVertexBuffer(vbo);

    // Draw
    m_deviceContext->Draw(vertexCount, 0);
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

//------------------------------------------------------------------------------------------------
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

    Texture* newTexture = CreateTextureFromData(imageFilePath, dimensions, bytesPerTexel, texelData);

    // Free the raw image texel data now that we've sent a copy of it down to the GPU to be stored in video memory
    stbi_image_free(texelData);

    m_loadedTextures.push_back(newTexture);
    return newTexture;
}

//------------------------------------------------------------------------------------------------
Texture* Renderer::CreateTextureFromData(char const* name, IntVec2 const& dimensions, int const bytesPerTexel, uint8_t const* texelData)
{
    // Check if the load was successful
    GUARANTEE_OR_DIE(texelData, Stringf("CreateTextureFromData failed for \"%s\" - texelData was null!", name))
    GUARANTEE_OR_DIE(bytesPerTexel >= 3 && bytesPerTexel <= 4,
                     Stringf("CreateTextureFromData failed for \"%s\" - unsupported BPP=%i (must be 3 or 4)", name,
                         bytesPerTexel))
    GUARANTEE_OR_DIE(dimensions.x > 0 && dimensions.y > 0,
                     Stringf("CreateTextureFromData failed for \"%s\" - illegal texture dimensions (%i x %i)", name,
                         dimensions.x, dimensions.y))

    Texture* newTexture      = new Texture();
    newTexture->m_name       = name; // NOTE: m_name must be a std::string, otherwise it may point to temporary data!
    newTexture->m_dimensions = dimensions;

    m_loadedTextures.push_back(newTexture);
    return newTexture;
}

//----------------------------------------------------------------------------------------------------
Shader* Renderer::CreateShader(char const* shaderName,
                               char const* shaderSource)
{
    ShaderConfig const shaderConfig;
    Shader*            shader = new Shader(shaderConfig);

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
    D3D11_INPUT_ELEMENT_DESC inputElementDesc[] =
    {
        {
            "POSITION",
            0,
            DXGI_FORMAT_R32G32B32_FLOAT,
            0,
            0,
            D3D11_INPUT_PER_VERTEX_DATA,
            0
        },
        {
            "COLOR",
            0,
            DXGI_FORMAT_R8G8B8A8_UNORM,
            0,
            D3D11_APPEND_ALIGNED_ELEMENT,
            D3D11_INPUT_PER_VERTEX_DATA,
            0
        },
        {
            "TEXCOORD",
            0,
            DXGI_FORMAT_R32G32_FLOAT,
            0,
            D3D11_APPEND_ALIGNED_ELEMENT,
            D3D11_INPUT_PER_VERTEX_DATA,
            0
        },
    };

    UINT constexpr numElements = ARRAYSIZE(inputElementDesc);

    hr = m_device->CreateInputLayout(
        inputElementDesc,
        numElements,
        vertexShaderByteCode.data(),
        vertexShaderByteCode.size(),
        &shader->m_inputLayout);

    return shader;
}

//----------------------------------------------------------------------------------------------------
Shader* Renderer::CreateShader(char const* shaderName)
{
    // Append the .hlsl extension to the shader name
    String const shaderFileName = Stringf("Data/Shaders/%s.hlsl", shaderName);

    String shaderSource;

    if (!FileReadToString(shaderSource, shaderFileName))
    {
        ERROR_AND_DIE(Stringf("Failed to read shader file: %s", shaderFileName.c_str()))
    }

    // Create the shader using the file contents
    return CreateShader(shaderName, shaderSource.c_str());
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

    HRESULT hr;

    hr = D3DCompile(source,
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
void Renderer::BindShader(Shader const* shader) const
{
    if (!shader)
    {
        shader = m_defaultShader;
    }

    m_deviceContext->VSSetShader(shader->m_vertexShader, nullptr, 0);
    m_deviceContext->PSSetShader(shader->m_pixelShader, nullptr, 0);
    m_deviceContext->IASetInputLayout(shader->m_inputLayout);
}

//----------------------------------------------------------------------------------------------------
VertexBuffer* Renderer::CreateVertexBuffer(unsigned int const size, unsigned int const stride) const
{
    // Create a new VertexBuffer object
    VertexBuffer* vertexBuffer = new VertexBuffer(m_device, size, stride);

    // Create the buffer
    //vertexBuffer->Create();

    return vertexBuffer;
}

//----------------------------------------------------------------------------------------------------
void Renderer::CopyCPUToGPU(void const* data, unsigned int const size, VertexBuffer* vbo) const
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
