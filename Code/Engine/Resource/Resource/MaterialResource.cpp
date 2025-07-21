//----------------------------------------------------------------------------------------------------
// MaterialResource.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Resource/Resource/MaterialResource.hpp"
#include "Engine/Resource/Resource/TextureResource.hpp"
#include "Engine/Resource/Resource/ShaderResource.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Renderer/Renderer.hpp"

//----------------------------------------------------------------------------------------------------
bool MaterialResource::Load()
{
    // 如果是從檔案載入（例如 .mtl 或自定義格式）
    // 這裡可以加入解析邏輯
    // 目前假設材質屬性已經由 ModelLoader 設定

    m_state = ResourceState::Loaded;
    return true;
}

void MaterialResource::Unload()
{
    // 釋放 GPU 資源
    if (m_constantBuffer)
    {
        m_constantBuffer->Release();
        m_constantBuffer = nullptr;
    }

    // 清理資料
    m_properties.clear();
    m_textureResources.clear();
    m_shaderResources.clear();

    m_state = ResourceState::Unloaded;
}

size_t MaterialResource::GetMemorySize() const
{
    size_t totalSize = sizeof(MaterialResource);

    // 材質屬性的記憶體
    totalSize += m_properties.size() * sizeof(MaterialProperty);

    // 常數緩衝區的記憶體
    if (m_constantBuffer)
    {
        totalSize += sizeof(MaterialConstants);
    }

    return totalSize;
}

void MaterialResource::AddProperty(const std::string& name, const MaterialProperty& property)
{
    m_properties[name] = property;
}

const MaterialResource::MaterialProperty* MaterialResource::GetProperty(const std::string& name) const
{
    auto it = m_properties.find(name);
    return (it != m_properties.end()) ? &it->second : nullptr;
}

MaterialResource::MaterialProperty* MaterialResource::GetProperty(const std::string& name)
{
    auto it = m_properties.find(name);
    return (it != m_properties.end()) ? &it->second : nullptr;
}

bool MaterialResource::HasProperty(const std::string& name) const
{
    return m_properties.find(name) != m_properties.end();
}

void MaterialResource::RemoveProperty(const std::string& name)
{
    m_properties.erase(name);
}

bool MaterialResource::CreateConstantBuffer(ID3D11Device* device)
{
    if (m_constantBuffer) return true;

    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.Usage             = D3D11_USAGE_DYNAMIC;
    cbDesc.ByteWidth         = sizeof(MaterialConstants);
    cbDesc.BindFlags         = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags    = D3D11_CPU_ACCESS_WRITE;

    HRESULT hr = device->CreateBuffer(&cbDesc, nullptr, &m_constantBuffer);
    if (FAILED(hr)) ERROR_AND_DIE("Failed to create material constant buffer")

    return true;
}

void MaterialResource::UpdateConstantBuffer(ID3D11DeviceContext* context, const std::string& propertyName)
{
    if (!m_constantBuffer) return;

    auto prop = GetProperty(propertyName);
    if (!prop) return;

    // 準備常數資料
    MaterialConstants constants;
    constants.diffuseColor  = prop->diffuseColor;
    constants.specularColor = prop->specularColor;
    constants.ambientColor  = prop->ambientColor;
    constants.emissiveColor = prop->emissiveColor;

    constants.shininess = prop->shininess;
    constants.metallic  = prop->metallic;
    constants.roughness = prop->roughness;
    constants.opacity   = prop->opacity;

    constants.normalStrength     = prop->normalStrength;
    constants.aoStrength         = prop->aoStrength;
    constants.uvScale            = prop->uvScale;
    constants.uvOffset           = prop->uvOffset;
    constants.alphaTestThreshold = prop->alphaTestThreshold;

    // 更新緩衝區
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    HRESULT                  hr = context->Map(m_constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (SUCCEEDED(hr))
    {
        memcpy(mappedResource.pData, &constants, sizeof(MaterialConstants));
        context->Unmap(m_constantBuffer, 0);
    }
}

void MaterialResource::SetTextureResource(String const&    propertyName,
                                          String const&    textureType,
                                          TextureResource* texture)
{
    TextureKey key{propertyName, textureType};
    m_textureResources[key] = texture;
}

TextureResource* MaterialResource::GetTextureResource(const std::string& propertyName,
                                                      const std::string& textureType) const
{
    TextureKey key{propertyName, textureType};
    auto       it = m_textureResources.find(key);
    return (it != m_textureResources.end()) ? it->second : nullptr;
}

void MaterialResource::SetShaderResource(const std::string& propertyName, ShaderResource* shader)
{
    m_shaderResources[propertyName] = shader;
}

ShaderResource* MaterialResource::GetShaderResource(const std::string& propertyName) const
{
    auto it = m_shaderResources.find(propertyName);
    return (it != m_shaderResources.end()) ? it->second : nullptr;
}
