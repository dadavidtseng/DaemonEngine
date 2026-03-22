//----------------------------------------------------------------------------------------------------
// MeshCache.hpp
// Engine Resource Module - Procedural Mesh Cache
//
// Purpose:
//   Caches procedural vertex data keyed by meshType string. Multiple entities sharing the same
//   meshType resolve to the same vertex buffer — no per-entity indirection required.
//
// Thread Safety:
//   Main thread only (vertex data is used for rendering).
//
// Author: Engine Resource Module
//----------------------------------------------------------------------------------------------------

#pragma once

//----------------------------------------------------------------------------------------------------
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Renderer/VertexUtils.hpp"
#include <string>
#include <unordered_map>

//----------------------------------------------------------------------------------------------------
// MeshCache Class
//
// Single-map cache: meshType → VertexList_PCU.
// Vertex data is created lazily on the first GetOrCreate() call for each meshType.
//
// Usage:
//   VertexList_PCU const* verts = meshCache->GetOrCreate("cube", 1.0f, Rgba8::WHITE);
//   if (verts) renderer->DrawVertexArray(verts->size(), verts->data());
//----------------------------------------------------------------------------------------------------
class MeshCache
{
public:
	// Get cached vertex data for meshType, creating it on first access.
	// Returns nullptr for unknown meshType strings.
	VertexList_PCU const* GetOrCreate(std::string const& meshType, float radius, Rgba8 const& color);

	// Number of unique mesh types currently cached.
	size_t GetMeshTypeCount() const { return m_cache.size(); }

	// Release all cached vertex data.
	void Clear();

private:
	std::unordered_map<std::string, VertexList_PCU> m_cache;
};
