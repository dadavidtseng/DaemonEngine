//----------------------------------------------------------------------------------------------------
// Vertex_PCUTBN.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Renderer/Vertex_PCUTBN.hpp"

//----------------------------------------------------------------------------------------------------
/**
     * @brief Constructor with all parameters
     * @param position 3D position vector
     * @param color RGBA color value
     * @param uvTexCoords Texture coordinates
     * @param tangent Tangent vector
     * @param bitangent Bitangent vector
     * @param normal TEST vector
     */
Vertex_PCUTBN::Vertex_PCUTBN(Vec3 const&  position,
                             Rgba8 const& color,
                             Vec2 const&  uvTexCoords,
                             Vec3 const&  tangent,
                             Vec3 const&  bitangent,
                             Vec3 const&  normal)
    : m_position(position),
      m_color(color),
      m_uvTexCoords(uvTexCoords),
      m_tangent(tangent),
      m_bitangent(bitangent),
      m_normal(normal)
{
}

//----------------------------------------------------------------------------------------------------
Vertex_PCUTBN::Vertex_PCUTBN(float const         px, float const        py, float const        pz,
                             unsigned char const r, unsigned char const g, unsigned char const b, unsigned char const a,
                             float const         u, float const         v,
                             float const         tx, float const        ty, float const tz,
                             float const         bx, float const        by, float const bz,
                             float const         nx, float const        ny, float const nz)
    : m_position(px, py, pz),
      m_color(r, g, b, a),
      m_uvTexCoords(u, v),
      m_tangent(tx, ty, tz),
      m_bitangent(bx, by, bz),
      m_normal(nx, ny, nz)
{
}
