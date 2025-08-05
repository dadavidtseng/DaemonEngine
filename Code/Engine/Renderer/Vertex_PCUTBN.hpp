//----------------------------------------------------------------------------------------------------
// Vertex_PCUTBN.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"

//----------------------------------------------------------------------------------------------------
/**
 * @struct Vertex_PCUTBN
 * @brief A vertex structure for rendering with complete geometric information
 *
 * This structure contains all necessary data for advanced rendering techniques
 * including normal mapping and lighting calculations.
 */
struct Vertex_PCUTBN
{
    /**
     * @brief 3D position of the vertex
     */
    Vec3 m_position = Vec3::ZERO;

    /**
     * @brief RGBA color value
     */
    Rgba8 m_color = Rgba8::MAGENTA;

    /**
     * @brief Texture coordinates for UV mapping
     */
    Vec2 m_uvTexCoords = Vec2::ZERO;

    /**
     * @brief Tangent vector for normal mapping
     */
    Vec3 m_tangent;

    /**
     * @brief Bitangent vector for normal mapping
     */
    Vec3 m_bitangent;

    /**
     * @brief Normal vector for lighting calculations
     */
    Vec3 m_normal;

    /**
     * @brief Default constructor
     */
    Vertex_PCUTBN() = default;


    Vertex_PCUTBN(Vec3 const& position, Rgba8 const& color, Vec2 const& uvTexCoords, Vec3 const& tangent, Vec3 const& bitangent, Vec3 const& normal);
    Vertex_PCUTBN(float px, float py, float pz, unsigned char r, unsigned char g, unsigned char b, unsigned char a, float u, float v, float tx, float ty, float tz, float bx, float by, float bz, float nx, float ny, float nz);
};
