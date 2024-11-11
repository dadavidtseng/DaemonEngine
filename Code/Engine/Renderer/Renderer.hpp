#pragma once
#include <cstdint>
#include <vector>

#include "Engine/Renderer/Texture.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Math/AABB2.hpp"

struct IntVec2;
class Window;

struct RenderConfig
{
	Window* m_window = nullptr;
};

//-----------------------------------------------------------------------------------------------
class Renderer
{
public:
	explicit Renderer(RenderConfig const& render_config);

	void Startup();
	void BeginFrame();
	void EndFrame() const;
	void Shutdown();

	void ClearScreen(const Rgba8& clearColor);
	void BeginCamera(const Camera& camera);
	void EndCamera(const Camera& camera);
	void DrawVertexArray(int numVertexes, const Vertex_PCU* vertexes);

	Texture* CreateOrGetTextureFromFile(char const* imageFilePath);
	void     BindTexture(const Texture* texture);
	void     DrawTexturedQuad(const AABB2& bounds, const Texture*      texture, const Rgba8& tint,
	                      const float      uniformScaleXY, const float rotationDegreesAboutZ);

private:
	// Private (internal) member function will go here
	Texture* GetTextureForFileName(char const* imageFilePath) const;
	void     CreateRenderingContext();
	Texture* CreateTextureFromFile(const char* imageFilePath);
	Texture* CreateTextureFromData(const char* name, IntVec2 dimensions, int bytesPerTexel, uint8_t* texelData);

	// Private (internal) data members will go here
	RenderConfig          m_config;
	void*                 m_apiRenderingContext = nullptr;
	std::vector<Texture*> m_loadedTextures;
};
