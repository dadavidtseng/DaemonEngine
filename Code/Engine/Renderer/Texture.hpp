//----------------------------------------------------------------------------------------------------
// Texture.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include <d3d11.h>

#include "Engine/Core/StringUtils.hpp"
#include "Engine/Math/IntVec2.hpp"

//----------------------------------------------------------------------------------------------------
class Texture
{
    friend class Renderer; // Only the Renderer can create new Texture objects!
    friend class RendererEx;

public:
    ~Texture();
    // Add a getter for texture dimensions
    IntVec2 GetDimensions() const { return m_dimensions; }

protected:
    String  m_name;
    IntVec2 m_dimensions;

    // #ToDo: multi-renderer compability
    // unsigned int m_openglTextureID = 0xFFFFFFFF;
    /// A 2D texture interface manages texel data, which is structured memory.
    ID3D11Texture2D*          m_texture            = nullptr;
    /// A shader-resource-view interface specifies the subresources a shader can access during rendering.
    /// Examples of shader resources include a constant buffer, a texture buffer, and a texture.
    ID3D11ShaderResourceView* m_shaderResourceView = nullptr;
};
