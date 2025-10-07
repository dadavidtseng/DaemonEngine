//----------------------------------------------------------------------------------------------------
// ShaderLoader.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Resource/ShaderLoader.hpp"
#include "Engine/Resource/ShaderResource.hpp"
#include "Engine/Renderer/Shader.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/FileUtils.hpp"
#include <d3d11.h>
#include <d3dcompiler.h>
#include <filesystem>

//----------------------------------------------------------------------------------------------------
ShaderLoader::ShaderLoader(ID3D11Device* device)
    : m_device(device)
{
    GUARANTEE_OR_DIE(device != nullptr, "ShaderLoader requires valid D3D11 device");
}

//----------------------------------------------------------------------------------------------------
bool ShaderLoader::CanLoad(String const& extension) const
{
    String lowerExt = extension;
    std::transform(lowerExt.begin(), lowerExt.end(), lowerExt.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    return lowerExt == ".hlsl" || lowerExt == ".fx" || lowerExt == ".shader";
}

//----------------------------------------------------------------------------------------------------
std::shared_ptr<IResource> ShaderLoader::Load(String const& path)
{
    // Default to VERTEX_PCU for compatibility
    return LoadShader(path, eVertexType::VERTEX_PCU);
}

//----------------------------------------------------------------------------------------------------
std::vector<String> ShaderLoader::GetSupportedExtensions() const
{
    return { ".hlsl", ".fx", ".shader" };
}

//----------------------------------------------------------------------------------------------------
std::shared_ptr<IResource> ShaderLoader::LoadShader(String const& path, eVertexType vertexType)
{
    // Create the resource first
    auto shaderResource = std::make_shared<ShaderResource>(path, eResourceType::SHADER);
    shaderResource->SetVertexType(vertexType);

    // Load the actual shader through DirectX
    Shader* rendererShader = CreateShaderFromFile(path, vertexType);

    if (rendererShader)
    {
        shaderResource->SetRendererShader(rendererShader);
        shaderResource->SetName(std::filesystem::path(path).filename().string());
        return shaderResource;
    }

    ERROR_RECOVERABLE(Stringf("Failed to load shader: %s", path.c_str()));
    return nullptr;
}

//----------------------------------------------------------------------------------------------------
Shader* ShaderLoader::CreateShaderFromFile(String const& path, eVertexType vertexType)
{
    std::filesystem::path filePath(path);
    if (!std::filesystem::exists(filePath))
    {
        // Try adding .hlsl extension if file not found
        std::filesystem::path hlslPath = filePath;
        hlslPath += ".hlsl";
        if (std::filesystem::exists(hlslPath))
        {
            filePath = hlslPath;
        }
        else
        {
            ERROR_RECOVERABLE(Stringf("Shader file not found: %s", path.c_str()));
            return nullptr;
        }
    }

    // Read shader source file
    String shaderSource;
    if (!FileReadToString(shaderSource, filePath.string()))
    {
        ERROR_RECOVERABLE(Stringf("Failed to read shader file: %s", filePath.string().c_str()));
        return nullptr;
    }

    // Create shader config
    sShaderConfig config;
    config.m_name = filePath.filename().string();

    // Create the shader object
    Shader* shader = new Shader(config);

    // Compile vertex shader
    std::vector<uint8_t> vertexShaderByteCode;
    std::vector<uint8_t> pixelShaderByteCode;

    if (!CompileShaderToByteCode(vertexShaderByteCode, config.m_name.c_str(), shaderSource.c_str(),
                                config.m_vertexEntryPoint.c_str(), "vs_5_0"))
    {
        delete shader;
        ERROR_RECOVERABLE(Stringf("Failed to compile vertex shader: %s", path.c_str()));
        return nullptr;
    }

    // Create vertex shader
    HRESULT hr = m_device->CreateVertexShader(
        vertexShaderByteCode.data(),
        vertexShaderByteCode.size(),
        nullptr,
        &shader->m_vertexShader);

    if (!SUCCEEDED(hr))
    {
        delete shader;
        ERROR_RECOVERABLE(Stringf("Could not create vertex shader: %s", path.c_str()));
        return nullptr;
    }

    // Compile pixel shader
    if (!CompileShaderToByteCode(pixelShaderByteCode, config.m_name.c_str(), shaderSource.c_str(),
                                config.m_pixelEntryPoint.c_str(), "ps_5_0"))
    {
        delete shader;
        ERROR_RECOVERABLE(Stringf("Failed to compile pixel shader: %s", path.c_str()));
        return nullptr;
    }

    // Create pixel shader
    hr = m_device->CreatePixelShader(
        pixelShaderByteCode.data(),
        pixelShaderByteCode.size(),
        nullptr,
        &shader->m_pixelShader);

    if (!SUCCEEDED(hr))
    {
        delete shader;
        ERROR_RECOVERABLE(Stringf("Could not create pixel shader: %s", path.c_str()));
        return nullptr;
    }

    // Create input layout based on vertex type
    if (!CreateInputLayoutForVertexType(shader, vertexShaderByteCode, vertexType))
    {
        delete shader;
        ERROR_RECOVERABLE(Stringf("Could not create input layout for shader: %s", path.c_str()));
        return nullptr;
    }

    return shader;
}

//----------------------------------------------------------------------------------------------------
bool ShaderLoader::CompileShaderToByteCode(std::vector<unsigned char>& out_byteCode,
                                          char const* name,
                                          char const* source,
                                          char const* entryPoint,
                                          char const* target)
{
    // Compile shader
    DWORD shaderFlags = D3DCOMPILE_OPTIMIZATION_LEVEL3;
#if defined(ENGINE_DEBUG_RENDER)
    shaderFlags = D3DCOMPILE_DEBUG;
    shaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
    shaderFlags |= D3DCOMPILE_WARNINGS_ARE_ERRORS;
#endif

    ID3DBlob* shaderBlob = nullptr;
    ID3DBlob* errorBlob  = nullptr;

    HRESULT hr = D3DCompile(
        source,
        strlen(source),
        name,
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        entryPoint,
        target,
        shaderFlags,
        0,
        &shaderBlob,
        &errorBlob);

    if (FAILED(hr))
    {
        if (errorBlob)
        {
            ERROR_RECOVERABLE(Stringf("Shader compilation failed (%s): %s", name, (char*)errorBlob->GetBufferPointer()));
            errorBlob->Release();
        }
        else
        {
            ERROR_RECOVERABLE(Stringf("Shader compilation failed (%s): Unknown error", name));
        }

        if (shaderBlob)
        {
            shaderBlob->Release();
        }
        return false;
    }

    if (errorBlob)
    {
        errorBlob->Release();
    }

    // Copy compiled bytecode
    out_byteCode.resize(shaderBlob->GetBufferSize());
    memcpy(out_byteCode.data(), shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize());

    shaderBlob->Release();
    return true;
}

//----------------------------------------------------------------------------------------------------
bool ShaderLoader::CreateInputLayoutForVertexType(Shader* shader,
                                                  std::vector<uint8_t> const& vertexShaderByteCode,
                                                  eVertexType vertexType)
{
    std::vector<D3D11_INPUT_ELEMENT_DESC> inputLayoutDesc;

    // Define input layout based on vertex type
    switch (vertexType)
    {
        case eVertexType::VERTEX_PCU:
        {
            inputLayoutDesc = {
                { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                { "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 }
            };
            break;
        }
        case eVertexType::VERTEX_PCUTBN:
        {
            inputLayoutDesc = {
                { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                { "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                { "BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 48, D3D11_INPUT_PER_VERTEX_DATA, 0 }
            };
            break;
        }
        default:
        {
            ERROR_RECOVERABLE(Stringf("Unsupported vertex type: %d", static_cast<int>(vertexType)));
            return false;
        }
    }

    // Create input layout
    HRESULT hr = m_device->CreateInputLayout(
        inputLayoutDesc.data(),
        static_cast<UINT>(inputLayoutDesc.size()),
        vertexShaderByteCode.data(),
        vertexShaderByteCode.size(),
        &shader->m_inputLayout);

    return SUCCEEDED(hr);
}
