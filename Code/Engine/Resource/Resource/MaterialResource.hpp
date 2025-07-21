//----------------------------------------------------------------------------------------------------
// MaterialResource.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include "Engine/Resource/Resource/IResource.hpp"
#include "Engine/Math/Vec4.hpp"
#include "Engine/Core/Rgba8.hpp"
#include <unordered_map>
#include <string>
#include <d3d11.h>

#include "Engine/Math/Vec2.hpp"

// Forward declarations
class TextureResource;
class ShaderResource;

class MaterialResource : public IResource
{
public:
    // 材質屬性結構
    struct MaterialProperty
    {
        // 基本顏色屬性
        Vec4 diffuseColor  = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
        Vec4 specularColor = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
        Vec4 ambientColor  = Vec4(0.2f, 0.2f, 0.2f, 1.0f);
        Vec4 emissiveColor = Vec4(0.0f, 0.0f, 0.0f, 1.0f);

        // 材質參數
        float shininess      = 32.0f;        // Phong 高光指數
        float metallic       = 0.0f;          // PBR 金屬度
        float roughness      = 0.5f;         // PBR 粗糙度
        float opacity        = 1.0f;           // 透明度
        float normalStrength = 1.0f;    // 法線強度
        float aoStrength     = 1.0f;        // AO 強度

        // 紋理路徑
        std::string diffuseTexture;     // 漫反射貼圖
        std::string normalTexture;      // 法線貼圖
        std::string specularTexture;    // 高光貼圖
        std::string emissiveTexture;    // 自發光貼圖
        std::string aoTexture;          // 環境遮蔽貼圖
        std::string roughnessTexture;   // 粗糙度貼圖
        std::string metallicTexture;    // 金屬度貼圖

        // 紋理設定
        Vec2 uvScale  = Vec2(1.0f, 1.0f);
        Vec2 uvOffset = Vec2(0.0f, 0.0f);

        // 渲染狀態
        bool  doubleSided        = false;
        bool  alphaTest          = false;
        float alphaTestThreshold = 0.5f;

        // 著色器相關
        std::string                            shaderName = "default";
        std::unordered_map<std::string, float> customFloats;
        std::unordered_map<std::string, Vec4>  customVectors;
    };

    // 建構與解構
    // MaterialResource(const std::string& path) : IResource(path) {}
    ~MaterialResource() override { Unload(); }

    // IResource 介面實作
    bool         Load() override;
    void         Unload() override;
    ResourceType GetType() const override { return ResourceType::Material; }
    size_t       GetMemorySize() const override;

    // 材質屬性管理
    void                    AddProperty(const std::string& name, const MaterialProperty& property);
    const MaterialProperty* GetProperty(const std::string& name) const;
    MaterialProperty*       GetProperty(const std::string& name);
    bool                    HasProperty(const std::string& name) const;
    void                    RemoveProperty(const std::string& name);

    // 取得所有材質
    const std::unordered_map<std::string, MaterialProperty>& GetAllProperties() const { return m_properties; }

    // GPU 資源管理
    bool          CreateConstantBuffer(ID3D11Device* device);
    void          UpdateConstantBuffer(ID3D11DeviceContext* context, const std::string& propertyName);
    ID3D11Buffer* GetConstantBuffer() const { return m_constantBuffer; }

    // 紋理資源管理
    void             SetTextureResource(String const& propertyName, String const& textureType, TextureResource* texture);
    TextureResource* GetTextureResource(const std::string& propertyName, const std::string& textureType) const;

    // 著色器資源管理
    void            SetShaderResource(const std::string& propertyName, ShaderResource* shader);
    ShaderResource* GetShaderResource(const std::string& propertyName) const;

private:
    // 材質常數緩衝區結構（對齊到 16 bytes）
    struct MaterialConstants
    {
        Vec4 diffuseColor;
        Vec4 specularColor;
        Vec4 ambientColor;
        Vec4 emissiveColor;

        float shininess;
        float metallic;
        float roughness;
        float opacity;

        float normalStrength;
        float aoStrength;
        Vec2  uvScale;

        Vec2  uvOffset;
        float alphaTestThreshold;
        float padding;  // 對齊到 16 bytes
    };

    // 資料成員
    std::unordered_map<std::string, MaterialProperty> m_properties;

    // GPU 資源
    ID3D11Buffer* m_constantBuffer = nullptr;

    // 紋理資源快取（避免重複載入）
    struct TextureKey
    {
        std::string propertyName;
        std::string textureType;

        bool operator==(const TextureKey& other) const
        {
            return propertyName == other.propertyName && textureType == other.textureType;
        }
    };

    struct TextureKeyHash
    {
        size_t operator()(const TextureKey& key) const
        {
            return std::hash<std::string>()(key.propertyName) ^
                (std::hash<std::string>()(key.textureType) << 1);
        }
    };

    std::unordered_map<TextureKey, TextureResource*, TextureKeyHash> m_textureResources;
    std::unordered_map<std::string, ShaderResource*>                 m_shaderResources;
};
