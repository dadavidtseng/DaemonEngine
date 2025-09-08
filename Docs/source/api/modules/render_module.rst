Render Module
=============

The Renderer Module contains Daemon Engine's rendering system functionality. This module is responsible for handling all graphics rendering and visual effects.

Overview
--------

The Renderer Module primarily provides the following functionality:

* **Graphics Rendering** - Handles 2D and 3D graphics rendering
* **DirectX11 Support** - Uses DirectX11 as the rendering backend
* **Window Management** - Supports multi-window rendering
* **Resource Management** - Manages textures, shaders, and other rendering resources
* **Vertex Data Structures** - Comprehensive vertex formats for advanced rendering

Vertex_PCUTBN Struct
---------------------

Advanced vertex data structure containing position, color, UV coordinates, tangent, bitangent, and normal information for complete geometric rendering support.

This structure is designed for advanced rendering techniques including normal mapping, tangent space calculations, and comprehensive lighting models.

.. doxygenstruct:: Vertex_PCUTBN
   :members:
   :undoc-members:

Vertex Components
~~~~~~~~~~~~~~~~~

* **Position (m_position)** - 3D world space coordinates
* **Color (m_color)** - RGBA8 color value with alpha transparency
* **UV Coordinates (m_uvTexCoords)** - Texture mapping coordinates
* **Tangent (m_tangent)** - Tangent vector for normal mapping calculations
* **Bitangent (m_bitangent)** - Bitangent vector perpendicular to tangent
* **Normal (m_normal)** - Surface normal vector for lighting calculations

Usage Example
~~~~~~~~~~~~~

.. code-block:: cpp

   #include "Engine/Renderer/Vertex_PCUTBN.hpp"
   
   // Create a vertex with all geometric data
   Vertex_PCUTBN vertex(
       Vec3(0.0f, 0.0f, 0.0f),           // position
       Rgba8::WHITE,                      // color
       Vec2(0.0f, 0.0f),                 // UV coordinates
       Vec3(1.0f, 0.0f, 0.0f),           // tangent
       Vec3(0.0f, 1.0f, 0.0f),           // bitangent
       Vec3(0.0f, 0.0f, 1.0f)            // normal
   );
   
   // Or create using individual float values
   Vertex_PCUTBN detailedVertex(
       0.5f, 1.0f, -0.5f,                // position x, y, z
       255, 255, 255, 255,               // color r, g, b, a
       0.5f, 0.5f,                       // UV u, v
       1.0f, 0.0f, 0.0f,                 // tangent x, y, z
       0.0f, 1.0f, 0.0f,                 // bitangent x, y, z
       0.0f, 0.0f, 1.0f                  // normal x, y, z
   );

Renderer Class
--------------

Main renderer class responsible for coordinating the entire rendering pipeline.

.. doxygenclass:: Renderer
   :members:
   :undoc-members:

Usage Example
~~~~~~~~~~~~~

.. code-block:: cpp

   // Initialize renderer
   Renderer renderer;
   renderer.Initialize(windowHandle, width, height);

   // Render loop
   while (isRunning) {
       renderer.BeginFrame();

       // Render objects
       renderer.RenderObject(mesh, transform);

       renderer.EndFrame();
   }

Rendering Pipeline
------------------

A typical rendering pipeline includes the following steps:

1. **Initialization** - Set up rendering device and resources
2. **Begin Frame** - Clear buffers, prepare for rendering
3. **Render Objects** - Draw objects in the scene
4. **End Frame** - Present rendering results

Performance Considerations
--------------------------

For optimal rendering performance, it is recommended to:

* Batch similar rendering calls
* Use appropriate shader optimizations
* Manage memory usage
* Avoid unnecessary state changes

Related Resources
-----------------

* :doc:`../api` - Back to API Overview
* :doc:`core_module` - Core Module Documentation
* :doc:`../quickstart` - Quick Start Guide