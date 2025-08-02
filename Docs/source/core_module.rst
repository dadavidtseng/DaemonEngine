Core Module
===========

The Core Module contains Daemon Engine's core data structures and basic functionality classes. These classes provide the fundamental components required for engine operation.

Overview
--------

The Core Module primarily provides the following functionality:

* **Data Structure Definitions** - Defines basic data types used by the engine
* **String Management** - Provides efficient string processing capabilities
* **Basic Utility Classes** - Supporting auxiliary functions for other modules

Vertex_PCUTBN Struct
---------------------

Vertex data structure containing position, color, UV coordinates, tangent, and normal information.

.. doxygenstruct:: Vertex_PCUTBN
   :members:
   :undoc-members:

Usage Example
~~~~~~~~~~~~~

.. code-block:: cpp

   // Create a vertex
   Vertex_PCUTBN vertex;
   vertex.position = Vec3(0.0f, 0.0f, 0.0f);
   vertex.color = Vec4(1.0f, 1.0f, 1.0f, 1.0f);

NamedStrings Class
------------------

Named string management class providing efficient string storage and query functionality.

.. doxygenclass:: NamedStrings
   :members:
   :undoc-members:

Usage Example
~~~~~~~~~~~~~

.. code-block:: cpp

   // Use NamedStrings to manage strings
   NamedStrings strings;
   strings.AddString("shader_name", "basic_shader");
   std::string shaderName = strings.GetString("shader_name");

Related Resources
-----------------

* :doc:`api` - Back to API Overview
* :doc:`renderer_module` - Renderer Module Documentation
* :doc:`quickstart` - Quick Start Guide