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
    bool                       CanLoad(String const& extension) const override;
    std::shared_ptr<IResource> Load(String const& path) override;
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

// 需要的輔助結構（可以放在 ObjModelLoader.hpp 中）:
struct Vec3Hasher
{
    std::size_t operator()(const Vec3& v) const
    {
        std::size_t h1 = std::hash<float>{}(v.x);
        std::size_t h2 = std::hash<float>{}(v.y);
        std::size_t h3 = std::hash<float>{}(v.z);
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};
