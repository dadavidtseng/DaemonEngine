//----------------------------------------------------------------------------------------------------
// ModelResource.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Resource/ModelResource.hpp"
#include "Engine/Resource/ObjModelLoader.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"

ModelResource::ModelResource(String const& path)
    : IResource(path, ResourceType::Model)
{
}

//----------------------------------------------------------------------------------------------------
bool ModelResource::Load()
{
    if (m_state == ResourceState::Loaded) return true;

    m_state = ResourceState::Loading;

    // 使用 ObjModelLoader 載入
    bool success = ObjModelLoader::Load(m_path, m_vertices, m_indices, m_hasNormals, m_hasUVs);

    if (success)
    {
        // 創建單一 SubMesh（為了未來擴展性）
        SubMesh mainMesh;
        mainMesh.name       = "main";
        mainMesh.vertices   = m_vertices;
        mainMesh.indices    = m_indices;
        mainMesh.hasNormals = m_hasNormals;
        mainMesh.hasUVs     = m_hasUVs;
        m_subMeshes.push_back(mainMesh);

        m_memorySize = CalculateMemorySize();
        m_state      = ResourceState::Loaded;
    }
    else
    {
        m_state = ResourceState::Failed;
    }

    return success;
}

void ModelResource::Unload()
{
    m_vertices.clear();
    m_indices.clear();
    m_subMeshes.clear();
    m_materials.clear();
    m_memorySize = 0;
}

size_t ModelResource::CalculateMemorySize() const
{
    size_t totalSize = 0;

    for (const auto& subMesh : m_subMeshes)
    {
        totalSize += subMesh.vertices.size() * sizeof(Vertex_PCUTBN);
        totalSize += subMesh.indices.size() * sizeof(unsigned int);
    }

    return totalSize;
}

const ModelResource::SubMesh* ModelResource::GetSubMesh(const std::string& name) const
{
    for (const auto& subMesh : m_subMeshes)
    {
        if (subMesh.name == name) return &subMesh;
    }
    return nullptr;
}
