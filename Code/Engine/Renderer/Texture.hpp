//----------------------------------------------------------------------------------------------------
// Texture.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include <string>

#include "Engine/Math/IntVec2.hpp"

//----------------------------------------------------------------------------------------------------
class Texture
{
    friend class Renderer; // Only the Renderer can create new Texture objects!

public:
    // Add a getter for texture dimensions
    IntVec2 GetDimensions() const { return m_dimensions; }
    
protected:
    std::string m_name;
    IntVec2     m_dimensions;

    // #ToDo: generalize/replace this for D3D11 support!
    unsigned int m_openglTextureID = 0xFFFFFFFF;
};
