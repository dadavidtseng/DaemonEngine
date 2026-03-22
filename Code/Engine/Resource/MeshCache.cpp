//----------------------------------------------------------------------------------------------------
// MeshCache.cpp
// Engine Resource Module - Procedural Mesh Cache Implementation
//
// Author: Engine Resource Module
//----------------------------------------------------------------------------------------------------

#include "Engine/Resource/MeshCache.hpp"

//----------------------------------------------------------------------------------------------------
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Renderer/VertexUtils.hpp"

//----------------------------------------------------------------------------------------------------
VertexList_PCU const* MeshCache::GetOrCreate(std::string const& meshType, float radius, Rgba8 const& color)
{
	// Return cached data if it already exists
	auto it = m_cache.find(meshType);
	if (it != m_cache.end())
	{
		return &it->second;
	}

	// Create vertex data for the requested mesh type
	VertexList_PCU verts;

	if (meshType == "cube")
	{
		Vec3 const frontBottomLeft(0.5f, -0.5f, -0.5f);
		Vec3 const frontBottomRight(0.5f, 0.5f, -0.5f);
		Vec3 const frontTopLeft(0.5f, -0.5f, 0.5f);
		Vec3 const frontTopRight(0.5f, 0.5f, 0.5f);
		Vec3 const backBottomLeft(-0.5f, 0.5f, -0.5f);
		Vec3 const backBottomRight(-0.5f, -0.5f, -0.5f);
		Vec3 const backTopLeft(-0.5f, 0.5f, 0.5f);
		Vec3 const backTopRight(-0.5f, -0.5f, 0.5f);

		AddVertsForQuad3D(verts, frontBottomLeft, frontBottomRight, frontTopLeft, frontTopRight, color);
		AddVertsForQuad3D(verts, backBottomLeft, backBottomRight, backTopLeft, backTopRight, color);
		AddVertsForQuad3D(verts, frontBottomRight, backBottomLeft, frontTopRight, backTopLeft, color);
		AddVertsForQuad3D(verts, backBottomRight, frontBottomLeft, backTopRight, frontTopLeft, color);
		AddVertsForQuad3D(verts, frontTopLeft, frontTopRight, backTopRight, backTopLeft, color);
		AddVertsForQuad3D(verts, backBottomRight, backBottomLeft, frontBottomLeft, frontBottomRight, color);
	}
	else if (meshType == "sphere")
	{
		AddVertsForSphere3D(verts, Vec3::ZERO, radius, color, AABB2::ZERO_TO_ONE, 32, 16);
	}
	else if (meshType == "grid")
	{
		float gridLineLength = 100.f;

		for (int i = -(int)gridLineLength / 2; i < (int)gridLineLength / 2; i++)
		{
			float lineWidth = 0.05f;
			if (i == 0)
			{
				lineWidth = 0.3f;
			}

			AABB3 boundsX(
				Vec3(-gridLineLength / 2.f, -lineWidth / 2.f + (float)i, -lineWidth / 2.f),
				Vec3(gridLineLength / 2.f, lineWidth / 2.f + (float)i, lineWidth / 2.f)
			);

			AABB3 boundsY(
				Vec3(-lineWidth / 2.f + (float)i, -gridLineLength / 2.f, -lineWidth / 2.f),
				Vec3(lineWidth / 2.f + (float)i, gridLineLength / 2.f, lineWidth / 2.f)
			);

			Rgba8 colorX = Rgba8::DARK_GREY;
			Rgba8 colorY = Rgba8::DARK_GREY;

			if (i % 5 == 0)
			{
				colorX = Rgba8::RED;
				colorY = Rgba8::GREEN;
			}

			AddVertsForAABB3D(verts, boundsX, colorX);
			AddVertsForAABB3D(verts, boundsY, colorY);
		}
	}
	else if (meshType == "plane")
	{
		float halfSize = radius;
		Vec3  bottomLeft(-halfSize, -halfSize, 0.0f);
		Vec3  bottomRight(halfSize, -halfSize, 0.0f);
		Vec3  topLeft(-halfSize, halfSize, 0.0f);
		Vec3  topRight(halfSize, halfSize, 0.0f);
		AddVertsForQuad3D(verts, bottomLeft, bottomRight, topLeft, topRight, color);
	}
	else
	{
		return nullptr;
	}

	if (verts.empty())
	{
		return nullptr;
	}

	// Insert into cache and return pointer to stored data
	auto [insertIt, _] = m_cache.emplace(meshType, std::move(verts));
	return &insertIt->second;
}

//----------------------------------------------------------------------------------------------------
void MeshCache::Clear()
{
	m_cache.clear();
}
