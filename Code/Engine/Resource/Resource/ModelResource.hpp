//----------------------------------------------------------------------------------------------------
// ModelResource.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include "Engine/Resource/Resource/IResource.hpp"
#include "Engine/Core/Vertex_PCUTBN.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include <vector>
#include <unordered_map>

class ModelResource : public IResource
{
public:
    struct SubMesh
    {
        std::string name;
        VertexList_PCUTBN vertices;
        IndexList indices;
        std::string materialName;
        bool hasNormals = false;
        bool hasUVs = false;
    };

    explicit ModelResource(String const& path);

    ~ModelResource() override
    {
        Unload();
    }

    bool Load() override;
    void Unload() override;
    size_t CalculateMemorySize() const override;

    // 模型專用方法
     std::vector<SubMesh> const& GetSubMeshes() const { return m_subMeshes; }
     SubMesh const* GetSubMesh( String const& name) const;

    // 直接取得頂點和索引（為了兼容你現有的程式碼）
     VertexList_PCUTBN const& GetVertices() const { return m_vertices; }
     IndexList const& GetIndices() const { return m_indices; }
    bool HasNormals() const { return m_hasNormals; }
    bool HasUVs() const { return m_hasUVs; }

    // 材質資訊
     std::unordered_map<std::string, Rgba8> const& GetMaterials() const { return m_materials; }

private:
    friend class ObjModelLoader;

    std::vector<SubMesh> m_subMeshes;
    std::unordered_map<std::string, Rgba8> m_materials;

    // 為了兼容性，保留整體的頂點和索引列表
    VertexList_PCUTBN m_vertices;
    IndexList m_indices;
    bool m_hasNormals = false;
    bool m_hasUVs = false;
};