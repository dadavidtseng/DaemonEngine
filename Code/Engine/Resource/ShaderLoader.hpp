//----------------------------------------------------------------------------------------------------
// ShaderLoader.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once

#include "Engine/Resource/IResourceLoader.hpp"
#include "Engine/Renderer/RenderCommon.hpp"

//----------------------------------------------------------------------------------------------------
struct ID3D11Device;
class Shader;

//----------------------------------------------------------------------------------------------------
class ShaderLoader : public IResourceLoader
{
public:
    explicit ShaderLoader(ID3D11Device* device);
    ~ShaderLoader() override = default;

    // IResourceLoader interface
    bool CanLoad(String const& extension) const override;
    std::shared_ptr<IResource> Load(String const& path) override;
    std::vector<String> GetSupportedExtensions() const override;

    // Shader-specific loading with vertex type
    std::shared_ptr<IResource> LoadShader(String const& path, eVertexType vertexType);

private:
    ID3D11Device* m_device;

    // Helper methods
    Shader* CreateShaderFromFile(String const& path, eVertexType vertexType);
    bool CompileShaderToByteCode(std::vector<unsigned char>& out_byteCode,
                                char const* name,
                                char const* source,
                                char const* entryPoint,
                                char const* target);
    bool CreateInputLayoutForVertexType(Shader* shader,
                                       std::vector<uint8_t> const& vertexShaderByteCode,
                                       eVertexType vertexType);
};
