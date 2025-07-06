// ============================================
// 更新後的 ObjModelLoader.hpp
// ============================================
#pragma once
#include "Engine/Resource/ResourceLoader/IResourceLoader.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Math/Mat44.hpp"
#include <unordered_map>

struct Rgba8;
struct Vertex_PCUTBN;

class ObjModelLoader : public IResourceLoader
{
public:
    // IResourceLoader 介面實作
    bool                       CanLoad(const std::string& extension) const override;
    std::shared_ptr<IResource> Load(const std::string& path) override;
    std::vector<std::string>   GetSupportedExtensions() const override;

    // 保留原有的靜態方法供直接使用或內部使用
    static bool Load(const String&      fileName,
                     VertexList_PCUTBN& out_vertexes,
                     IndexList&         out_indexes,
                     bool&              out_hasNormals,
                     bool&              out_hasUVs,
                     const Mat44&       transform = Mat44()) noexcept;

    static bool LoadMaterial(const String&                      path,
                             std::unordered_map<String, Rgba8>& materialMap) noexcept;
};
