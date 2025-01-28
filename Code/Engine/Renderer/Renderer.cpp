//----------------------------------------------------------------------------------------------------
// Renderer.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "ThirdParty/stb/stb_image.h"
#include "Engine/Renderer/Renderer.hpp"

#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Renderer/Window.hpp"

#define WIN32_LEAN_AND_MEAN			// Always #define this before #including <windows.h>
#include <windows.h>
#include <d3dcompiler.h>
#include <dxgi.h>


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

    swapChainDesc.BufferDesc.Width     = window->GetClientDimensions().x;
    swapChainDesc.BufferDesc.Height    = window->GetClientDimensions().y;
    swapChainDesc.BufferDesc.Format    = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.SampleDesc.Count     = 1;
    swapChainDesc.BufferUsage          = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount          = 2;
    swapChainDesc.OutputWindow         = static_cast<HWND>(window->GetWindowHandle());
    swapChainDesc.Windowed             = true;
    swapChainDesc.SwapEffect           = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    HRESULT hr;

    hr = D3D11CreateDeviceAndSwapChain(
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
    D3D11_RASTERIZER_DESC rasterizerDesc = {};
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
}

//-----------------------------------------------------------------------------------------------
void Renderer::BeginFrame()
{
}

//-----------------------------------------------------------------------------------------------
void Renderer::EndFrame() const
{
    if (m_config.m_window)
    {
        const HDC displayContext = static_cast<HDC>(m_config.m_window->GetDisplayContext());

        // "Present" the back buffer by swapping the front (visible) and back (working) screen buffers
        // SwapBuffers(displayContext); // Note: call this only once at the very end of each frame
        // Present
        HRESULT const hr = m_swapChain->Present(0, 0);

        if (hr == DXGI_ERROR_DEVICE_REMOVED ||
            hr == DXGI_ERROR_DEVICE_RESET)
        {
            ERROR_AND_DIE("Device has been lost, application will now terminate.")
        }
    }
}

//-----------------------------------------------------------------------------------------------
void Renderer::Shutdown()
{
    // Release all DirectX objects and check for memory leaks in your Shutdown function.
    m_rasterizerState->Release();
    m_renderTargetView->Release();
    m_swapChain->Release();
    m_deviceContext->Release();
    m_device->Release();

    // Report error leaks and release debug module
#if defined(ENGINE_DEBUG_RENDER)
    constexpr DXGI_DEBUG_RLO_FLAGS flags = static_cast<DXGI_DEBUG_RLO_FLAGS>(DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL);
    static_cast<IDXGIDebug*>(m_dxgiDebug)->ReportLiveObjects(DXGI_DEBUG_ALL, flags);

    static_cast<IDXGIDebug*>(m_dxgiDebug)->Release();
    m_dxgiDebug = nullptr;

    FreeLibrary(static_cast<HMODULE>(m_dxgiDebugModule));
    m_dxgiDebugModule = nullptr;
#endif
}

//-----------------------------------------------------------------------------------------------
void Renderer::ClearScreen(Rgba8 const& clearColor)
{
    // This code needs to run every frame and should be in your Render function.
    m_deviceContext->OMSetRenderTargets(1, &m_renderTargetView, nullptr);

    // Clear the screen
    float       colorAsFloats[4];
    clearColor.GetAsFloats(colorAsFloats);
    m_deviceContext->ClearRenderTargetView(m_renderTargetView, colorAsFloats);

    // constexpr UINT stride      = sizeof(Vertex_PCU);
    // constexpr UINT startOffset = 0;
    // m_deviceContext->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &startOffset);
    // m_deviceContext->IASetInputLayout(m_inputLayoutForVertex_PCU);
    // m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    // m_deviceContext->VSSetShader(m_vertexShader, nullptr, 0);
    // m_deviceContext->PSSetShader(m_pixelShader, nullptr, 0);
}

//-----------------------------------------------------------------------------------------------
void Renderer::BeginCamera(const Camera& camera)
{
    Vec2 const bottomLeft = camera.GetOrthoBottomLeft();
    Vec2 const topRight   = camera.GetOrthoTopRight();

    // Establish a 2D (orthographic) drawing coordinate system (bottom-left to top-right)
    // glLoadIdentity();
    // arguments are: xLeft, xRight, yBottom, yTop, zNear, zFar
    // glOrtho(bottomLeft.x, topRight.x, bottomLeft.y, topRight.y, 0.f, 1.f);
}

//-----------------------------------------------------------------------------------------------
void Renderer::EndCamera(const Camera& camera)
{
    UNUSED(camera)
}

//-----------------------------------------------------------------------------------------------
void Renderer::DrawVertexArray(const int numVertexes, const Vertex_PCU* vertexes)
{
    // Draw some triangles (provide 3 vertexes each)
    //
    // glBegin(GL_TRIANGLES);
    // {
    //     for (int i = 0; i < numVertexes; i++)
    //     {
    //         const Vec3  position    = vertexes[i].m_position;
    //         const Rgba8 color       = vertexes[i].m_color;
    //         const Vec2  uvTexCoords = vertexes[i].m_uvTexCoords;
    //
    //         glColor4ub(color.r, color.g, color.b, color.a);
    //         glTexCoord2f(uvTexCoords.x, uvTexCoords.y);
    //         glVertex2f(position.x, position.y);
    //     }
    // }
    // glEnd();
}

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
// Given an existing OS Window, create a Rendering Context (RC) for OpenGL or DirectX to draw to it.
//
// void Renderer::CreateRenderingContext()
// {
//     // Creates an OpenGL rendering context (RC) and binds it to the current window's device context (DC)
//     PIXELFORMATDESCRIPTOR pixelFormatDescriptor;
//
//     memset(&pixelFormatDescriptor, 0, sizeof(pixelFormatDescriptor));
//     pixelFormatDescriptor.nSize        = sizeof(pixelFormatDescriptor);
//     pixelFormatDescriptor.nVersion     = 1;
//     pixelFormatDescriptor.dwFlags      = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
//     pixelFormatDescriptor.iPixelType   = PFD_TYPE_RGBA;
//     pixelFormatDescriptor.cColorBits   = 24;
//     pixelFormatDescriptor.cDepthBits   = 24;
//     pixelFormatDescriptor.cAccumBits   = 0;
//     pixelFormatDescriptor.cStencilBits = 8;
//
//     HDC const displayContext = static_cast<HDC>(m_config.m_window->GetDisplayContext());
//
//     // These two OpenGL-like functions (wglCreateContext and wglMakeCurrent) will remain here for now.
//     int const pixelFormatCode = ChoosePixelFormat(displayContext, &pixelFormatDescriptor);
//
//     SetPixelFormat(displayContext, pixelFormatCode, &pixelFormatDescriptor);
//
//     m_apiRenderingContext = wglCreateContext(displayContext);
//
//     wglMakeCurrent(displayContext, static_cast<HGLRC>(m_apiRenderingContext));
//
//     glEnable(GL_BLEND);
//     glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
// }

//-----------------------------------------------------------------------------------------------
// Sample code for loading an image from disk and creating an OpenGL texture from its data.
// 
// Game code calls RenderContext::CreateOrGetTextureFromFile(), which in turn will
//	check that name amongst the registry of already-loaded textures (by name).  If that image
//	has already been loaded, the renderer simply returns the Texture* it already has.  If the
//	image has not been loaded before, CreateTextureFromFile() gets called internally, which in
//	turn calls CreateTextureFromData.  The new Texture* is then added to the registry of
//	already-loaded textures, and then returned.
//------------------------------------------------------------------------------------------------


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


//------------------------------------------------------------------------------------------------
Texture* Renderer::CreateTextureFromFile(char const* imageFilePath)
{
    IntVec2 dimensions    = IntVec2::ZERO; // This will be filled in for us to indicate image width & height
    int     bytesPerTexel = 0;
    // This will be filled in for us to indicate how many color components the image had (e.g. 3=RGB=24bit, 4=RGBA=32bit)
    constexpr int numComponentsRequested = 0; // don't care; we support 3 (24-bit RGB) or 4 (32-bit RGBA)

    // Load (and decompress) the image RGB(A) bytes from a file on disk into a memory buffer (array of bytes)
    stbi_set_flip_vertically_on_load(1); // We prefer uvTexCoords has origin (0,0) at BOTTOM LEFT
    unsigned char* texelData = stbi_load(imageFilePath, &dimensions.x, &dimensions.y, &bytesPerTexel,
                                         numComponentsRequested);

    // Check if the load was successful
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
    GUARANTEE_OR_DIE(texelData, Stringf("CreateTextureFromData failed for \"%s\" - texelData was null!", name));
    GUARANTEE_OR_DIE(bytesPerTexel >= 3 && bytesPerTexel <= 4,
                     Stringf("CreateTextureFromData failed for \"%s\" - unsupported BPP=%i (must be 3 or 4)", name,
                         bytesPerTexel));
    GUARANTEE_OR_DIE(dimensions.x > 0 && dimensions.y > 0,
                     Stringf("CreateTextureFromData failed for \"%s\" - illegal texture dimensions (%i x %i)", name,
                         dimensions.x, dimensions.y));

    Texture* newTexture      = new Texture();
    newTexture->m_name       = name; // NOTE: m_name must be a std::string, otherwise it may point to temporary data!
    newTexture->m_dimensions = dimensions;

    // Enable OpenGL texturing
    // glEnable(GL_TEXTURE_2D);

    // Tell OpenGL that our pixel data is single-byte aligned
    // glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // Ask OpenGL for an unused texName (ID number) to use for this texture
    // glGenTextures(1, (GLuint*) &newTexture->m_openglTextureID);

    // Tell OpenGL to bind (set) this as the currently active texture
    // glBindTexture(GL_TEXTURE_2D, newTexture->m_openglTextureID);

    // Set texture clamp vs. wrap (repeat) default settings
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); // GL_CLAMP or GL_REPEAT
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); // GL_CLAMP or GL_REPEAT

    // Set magnification (texel > pixel) and minification (texel < pixel) filters
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); // one of: GL_NEAREST, GL_LINEAR
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    // one of: GL_NEAREST, GL_LINEAR, GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST_MIPMAP_LINEAR, GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR_MIPMAP_LINEAR

    // Pick the appropriate OpenGL format (RGB or RGBA) for this texel data
    // GLenum bufferFormat = GL_RGBA;
    // the format our source pixel data is in; any of: GL_RGB, GL_RGBA, GL_LUMINANCE, GL_LUMINANCE_ALPHA, ...
    if (bytesPerTexel == 3)
    {
        // bufferFormat = GL_RGB;
    }
    // GLenum internalFormat = bufferFormat;
    // the format we want the texture to be on the card; technically allows us to translate into a different texture format as we upload to OpenGL

    // Upload the image texel data (raw pixels bytes) to OpenGL under this textureID
    // glTexImage2D( // Upload this pixel data to our new OpenGL texture
    // GL_TEXTURE_2D, // Creating this as a 2d texture
    // 0, // Which mipmap level to use as the "root" (0 = the highest-quality, full-res image), if mipmaps are enabled
    // internalFormat, // Type of texel format we want OpenGL to use for this texture internally on the video card
    // dimensions.x,
    // Texel-width of image; for maximum compatibility, use 2^N + 2^B, where N is some integer in the range [3,11], and B is the border thickness [0,1]
    // dimensions.y,
    // Texel-height of image; for maximum compatibility, use 2^M + 2^B, where M is some integer in the range [3,11], and B is the border thickness [0,1]
    // 0,                // Border size, in texels (must be 0 or 1, recommend 0)
    // bufferFormat,     // Pixel format describing the composition of the pixel data in buffer
    // GL_UNSIGNED_BYTE, // Pixel color components are unsigned bytes (one byte per color channel/component)
    // texelData);       // Address of the actual pixel data bytes/buffer in system memory

    m_loadedTextures.push_back(newTexture);
    return newTexture;
}

//----------------------------------------------------------------------------------------------------
BitmapFont* Renderer::CreateOrGetBitmapFontFromFile(const char* bitmapFontFilePathWithNoExtension)
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
        ERROR_AND_DIE(Stringf( "Unknown / unsupported blend mode #%i", mode ));
    }
}


//-----------------------------------------------------------------------------------------------
void Renderer::BindTexture(const Texture* texture)
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

void Renderer::DrawTexturedQuad(const AABB2& bounds, const Texture*      texture, const Rgba8& tint,
                                const float  uniformScaleXY, const float rotationDegreesAboutZ)
{
    std::vector<Vertex_PCU> quadVerts;
    AddVertsForAABB2D(quadVerts, bounds, tint);


    TransformVertexArrayXY3D(static_cast<int>(quadVerts.size()), quadVerts.data(),
                             uniformScaleXY, rotationDegreesAboutZ, Vec2(0, 0));

    BindTexture(texture);
    DrawVertexArray(static_cast<int>(quadVerts.size()), quadVerts.data());
}
