Renderer Module
===============

The Renderer Module contains Daemon Engine's rendering system functionality. This module is responsible for handling all graphics rendering and visual effects.

Overview
--------

The Renderer Module primarily provides the following functionality:

* **Graphics Rendering** - Handles 2D and 3D graphics rendering
* **DirectX11 Support** - Uses DirectX11 as the rendering backend
* **Window Management** - Supports multi-window rendering
* **Resource Management** - Manages textures, shaders, and other rendering resources

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

* :doc:`api` - Back to API Overview
* :doc:`core_module` - Core Module Documentation
* :doc:`quickstart` - Quick Start Guide