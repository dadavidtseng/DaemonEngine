//----------------------------------------------------------------------------------------------------
// Vertex_PCU.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"

//----------------------------------------------------------------------------------------------------
/**
 * @struct Vertex_PCU
 * @brief A basic vertex structure for simple rendering
 *
 * This structure contains the essential data for basic rendering operations.
 * PCU stands for Position, Color, UV coordinates.
 */
struct Vertex_PCU
{
    /**
     * @brief 3D position of the vertex
     */
    Vec3 m_position = Vec3::ZERO;

    /**
     * @brief RGBA color value
     */
    Rgba8 m_color = Rgba8::WHITE;

    /**
     * @brief Texture coordinates for UV mapping
     */
    Vec2 m_uvTexCoords = Vec2::ZERO;

    /**
     * @brief Default constructor
     */
    Vertex_PCU() = default;

    /**
     * @brief Constructor with parameters
     * @param position 3D position vector
     * @param color RGBA color value
     * @param uvTexCoords Texture coordinates
     */
    Vertex_PCU(Vec3 const& position, Rgba8 const& color, Vec2 const& uvTexCoords = Vec2::ZERO);
};