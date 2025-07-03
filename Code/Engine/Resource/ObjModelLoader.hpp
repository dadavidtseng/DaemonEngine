//----------------------------------------------------------------------------------------------------
// ObjModelLoader.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include <string>

#include <unordered_map>

#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Math/Mat44.hpp"
#include "Engine/Resource/IResourceLoader.hpp"

//-Forward-Declaration--------------------------------------------------------------------------------
struct Rgba8;
struct Vertex_PCUTBN;

//----------------------------------------------------------------------------------------------------
class ObjModelLoader : public IResourceLoader
{
public:
    static bool Load(String const& fileName, VertexList_PCUTBN& out_vertexes, IndexList& out_indexes, bool& out_hasNormals, bool& out_hasUVs, Mat44 const& transform = Mat44()) noexcept;
    static bool LoadMaterial(String const& path, std::unordered_map<String, Rgba8>& materialMap) noexcept;

private:
};

// // OBJModelLoader.hpp - OBJ模型載入器（整合你現有的OBJLoader）
// #pragma once
// #include "IResourceLoader.hpp"
// #include "ModelResource.hpp"
//
// class OBJModelLoader : public IResourceLoader
// {
// public:
//     bool CanLoad(const std::string& extension) const override
//     {
//         return extension == ".obj" || extension == ".OBJ";
//     }
//
//     std::shared_ptr<IResource> Load(const std::string& path) override
//     {
//         auto model = std::make_shared<ModelResource>(path);
//
//         // 使用你現有的OBJLoader
//         VertexList_PCUTBN vertices;
//         IndexList indices;
//         bool hasNormals, hasUVs;
//
//         if (OBJLoader::Load(path, vertices, indices, hasNormals, hasUVs))
//         {
//             // 轉換為ModelResource格式
//             ModelResource::SubMesh mesh;
//             mesh.vertices = std::move(vertices);
//             mesh.indices = std::move(indices);
//             mesh.hasNormals = hasNormals;
//             mesh.hasUVs = hasUVs;
//
//             model->m_subMeshes.push_back(std::move(mesh));
//             model->m_state = ResourceState::Loaded;
//
//             return model;
//         }
//
//         return nullptr;
//     }
//
//     std::vector<std::string> GetSupportedExtensions() const override
//     {
//         return { ".obj", ".OBJ" };
//     }
// };
