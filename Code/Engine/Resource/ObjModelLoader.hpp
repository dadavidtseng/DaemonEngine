//----------------------------------------------------------------------------------------------------
// ObjModelLoader.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include <unordered_map>
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Math/Mat44.hpp"
#include "Engine/Renderer/VertexUtils.hpp"
#include "Engine/Resource/IResourceLoader.hpp"

//----------------------------------------------------------------------------------------------------
class ObjModelLoader : public IResourceLoader
{
public:
    bool                       CanLoad(String const& extension) const override;
    std::shared_ptr<IResource> Load(String const& path) override;
    std::vector<String>        GetSupportedExtensions() const override;

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
