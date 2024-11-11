#include "ThirdParty/stb/stb_image.h"
#define WIN32_LEAN_AND_MEAN			// Always #define this before #including <windows.h>
#include "Windows.h"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Window.hpp"

//-----------------------------------------------------------------------------------------------
// Both of the following lines will eventually move to the top of Engine/Renderer/Renderer.cpp
//
#include <wingdi.h>
#include <gl/gl.h>					// Include basic OpenGL constants and function declarations

#include "Engine/Renderer/Texture.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Math/IntVec2.hpp"

#pragma comment( lib, "opengl32" )	// Link in the OpenGL32.lib static library

extern HWND g_hWnd;

Renderer::Renderer(RenderConfig const& render_config)
{
	m_config = render_config;
}

//-----------------------------------------------------------------------------------------------
void Renderer::Startup()
{
	CreateRenderingContext();
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
		SwapBuffers(displayContext); // Note: call this only once at the very end of each frame
	}
}

//-----------------------------------------------------------------------------------------------
void Renderer::Shutdown()
{
}

//-----------------------------------------------------------------------------------------------
void Renderer::ClearScreen(const Rgba8& clearColor)
{
	// Clear all screen (back buffer) pixels to clearColor
	glClearColor(static_cast<float>(clearColor.r) / 255.0f,
	             static_cast<float>(clearColor.g) / 255.0f,
	             static_cast<float>(clearColor.b) / 255.0f,
	             clearColor.a);
	// Note; glClearColor takes colors as floats in [0,1], not bytes in [0,255]
	// ALWAYS clear the screen at the top of each frame's Render()!
	glClear(GL_COLOR_BUFFER_BIT);
}

//-----------------------------------------------------------------------------------------------
void Renderer::BeginCamera(const Camera& camera)
{
	Vec2 bottomLeft = camera.GetOrthoBottomLeft();
	Vec2 topRight   = camera.GetOrthoTopRight();

	// Establish a 2D (orthographic) drawing coordinate system (bottom-left to top-right)
	glLoadIdentity();
	// arguments are: xLeft, xRight, yBottom, yTop, zNear, zFar
	glOrtho(bottomLeft.x, topRight.x, bottomLeft.y, topRight.y, 0.f, 1.f);
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
	glBegin(GL_TRIANGLES);
	{
		for (int i = 0; i < numVertexes; i++)
		{
			const Vec3  position    = vertexes[i].m_position;
			const Rgba8 color       = vertexes[i].m_color;
			const Vec2  uvTexCoords = vertexes[i].m_uvTexCoords;

			glColor4ub(color.r, color.g, color.b, color.a);
			glTexCoord2f(uvTexCoords.x, uvTexCoords.y);
			glVertex2f(position.x, position.y);
		}
	}
	glEnd();
}

Texture* Renderer::GetTextureForFileName(char const* imageFilePath) const
{
	for (Texture* texture : m_loadedTextures)
	{
		if (texture && !strcmp(texture->m_name.c_str() , imageFilePath))
		{
			return texture;
		}
	}
	return nullptr;
}

//------------------------------------------------------------------------------------------------
// Given an existing OS Window, create a Rendering Context (RC) for OpenGL or DirectX to draw to it.
//
void Renderer::CreateRenderingContext()
{
	// Creates an OpenGL rendering context (RC) and binds it to the current window's device context (DC)
	PIXELFORMATDESCRIPTOR pixelFormatDescriptor;

	memset(&pixelFormatDescriptor, 0, sizeof(pixelFormatDescriptor));
	pixelFormatDescriptor.nSize        = sizeof(pixelFormatDescriptor);
	pixelFormatDescriptor.nVersion     = 1;
	pixelFormatDescriptor.dwFlags      = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pixelFormatDescriptor.iPixelType   = PFD_TYPE_RGBA;
	pixelFormatDescriptor.cColorBits   = 24;
	pixelFormatDescriptor.cDepthBits   = 24;
	pixelFormatDescriptor.cAccumBits   = 0;
	pixelFormatDescriptor.cStencilBits = 8;

	const HDC displayContext = static_cast<HDC>(m_config.m_window->GetDisplayContext());

	// These two OpenGL-like functions (wglCreateContext and wglMakeCurrent) will remain here for now.
	const int pixelFormatCode = ChoosePixelFormat(displayContext, &pixelFormatDescriptor);

	SetPixelFormat(displayContext, pixelFormatCode, &pixelFormatDescriptor);

	m_apiRenderingContext = wglCreateContext(displayContext);

	wglMakeCurrent(displayContext, static_cast<HGLRC>(m_apiRenderingContext));

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

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
	int numComponentsRequested = 0; // don't care; we support 3 (24-bit RGB) or 4 (32-bit RGBA)

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
Texture* Renderer::CreateTextureFromData(char const* name, IntVec2 dimensions, int bytesPerTexel, uint8_t* texelData)
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
	glEnable(GL_TEXTURE_2D);

	// Tell OpenGL that our pixel data is single-byte aligned
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	// Ask OpenGL for an unused texName (ID number) to use for this texture
	glGenTextures(1, (GLuint*)&newTexture->m_openglTextureID);

	// Tell OpenGL to bind (set) this as the currently active texture
	glBindTexture(GL_TEXTURE_2D, newTexture->m_openglTextureID);

	// Set texture clamp vs. wrap (repeat) default settings
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); // GL_CLAMP or GL_REPEAT
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); // GL_CLAMP or GL_REPEAT

	// Set magnification (texel > pixel) and minification (texel < pixel) filters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); // one of: GL_NEAREST, GL_LINEAR
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	// one of: GL_NEAREST, GL_LINEAR, GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST_MIPMAP_LINEAR, GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR_MIPMAP_LINEAR

	// Pick the appropriate OpenGL format (RGB or RGBA) for this texel data
	GLenum bufferFormat = GL_RGBA;
	// the format our source pixel data is in; any of: GL_RGB, GL_RGBA, GL_LUMINANCE, GL_LUMINANCE_ALPHA, ...
	if (bytesPerTexel == 3)
	{
		bufferFormat = GL_RGB;
	}
	GLenum internalFormat = bufferFormat;
	// the format we want the texture to be on the card; technically allows us to translate into a different texture format as we upload to OpenGL

	// Upload the image texel data (raw pixels bytes) to OpenGL under this textureID
	glTexImage2D( // Upload this pixel data to our new OpenGL texture
		GL_TEXTURE_2D, // Creating this as a 2d texture
		0, // Which mipmap level to use as the "root" (0 = the highest-quality, full-res image), if mipmaps are enabled
		internalFormat, // Type of texel format we want OpenGL to use for this texture internally on the video card
		dimensions.x,
		// Texel-width of image; for maximum compatibility, use 2^N + 2^B, where N is some integer in the range [3,11], and B is the border thickness [0,1]
		dimensions.y,
		// Texel-height of image; for maximum compatibility, use 2^M + 2^B, where M is some integer in the range [3,11], and B is the border thickness [0,1]
		0,                // Border size, in texels (must be 0 or 1, recommend 0)
		bufferFormat,     // Pixel format describing the composition of the pixel data in buffer
		GL_UNSIGNED_BYTE, // Pixel color components are unsigned bytes (one byte per color channel/component)
		texelData);       // Address of the actual pixel data bytes/buffer in system memory

	m_loadedTextures.push_back(newTexture);
	return newTexture;
}


//-----------------------------------------------------------------------------------------------
void Renderer::BindTexture(const Texture* texture)
{
	if (texture)
	{
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, texture->m_openglTextureID);
	}
	else
	{
		glDisable(GL_TEXTURE_2D);
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
