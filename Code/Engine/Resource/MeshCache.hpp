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
#include "Engine/Renderer/Vertex_PCUTBN.hpp"
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
struct ModelMeshData
{
	VertexList_PCUTBN vertices;
	IndexList         indices;
};

//----------------------------------------------------------------------------------------------------

class MeshCache
{
public:
	// Get cached PCU vertex data for primitive meshType, creating it on first access.
	VertexList_PCU const* GetOrCreate(std::string const& meshType, float radius, Rgba8 const& color);

	// Get cached PCUTBN vertex + index data for OBJ models, loading on first access.
	ModelMeshData const* GetOrCreateModel(std::string const& meshType);

	size_t GetMeshTypeCount() const { return m_cache.size() + m_modelCache.size(); }

	void Clear();

private:
	std::unordered_map<std::string, VertexList_PCU> m_cache;
	std::unordered_map<std::string, ModelMeshData>  m_modelCache;
};
