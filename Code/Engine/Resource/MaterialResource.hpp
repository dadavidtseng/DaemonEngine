//----------------------------------------------------------------------------------------------------
// ModelResource.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include "IResource.hpp"
#include "IVertexData.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Core/Vertex_PCUTBN.hpp"
#include <memory>
#include <variant>

class ModelResource : public IResource
{
public:
    // SubMesh 現在支援多種頂點格式
    struct SubMesh
    {
        std::unique_ptr<IVertexData> vertexData;  // 多型頂點資料
        std::vector<unsigned int>    indices;
        std::string                  materialName;
        std::string                  materialPath;
        VertexFormat                 vertexFormat;

        // GPU 資源
        ID3D11Buffer* indexBuffer = nullptr;

        // 輔助方法
        template <typename VertexType>
        VertexData<VertexType>* GetVertexDataAs()
        {
            return dynamic_cast<VertexData<VertexType>*>(vertexData.get());
        }

        template <typename VertexType>
        const VertexData<VertexType>* GetVertexDataAs() const
        {
            return dynamic_cast<const VertexData<VertexType>*>(vertexData.get());
        }
    };

    explicit ModelResource(const std::string& path)
        : IResource(path)
    {
    }

    ~ModelResource() override { Unload(); }

    // IResource 介面
    bool         Load() override { return true; }
    void         Unload() override;
    ResourceType GetType() const override { return ResourceType::Model; }
    size_t       GetMemorySize() const override;

    // SubMesh 管理
    void                        AddSubMesh(SubMesh&& subMesh) { m_subMeshes.push_back(std::move(subMesh)); }
    const std::vector<SubMesh>& GetSubMeshes() const { return m_subMeshes; }
    std::vector<SubMesh>&       GetSubMeshes() { return m_subMeshes; }

    // GPU 資源
    bool CreateGPUResources(ID3D11Device* device);
    void ReleaseGPUResources();

    // 設定預設頂點格式
    void         SetDefaultVertexFormat(VertexFormat format) { m_defaultVertexFormat = format; }
    VertexFormat GetDefaultVertexFormat() const { return m_defaultVertexFormat; }

private:
    std::vector<SubMesh> m_subMeshes;
    VertexFormat         m_defaultVertexFormat = VertexFormat::PCUTBN;
    bool                 m_gpuResourcesCreated = false;
};
