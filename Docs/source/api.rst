API Reference
======================================================================================================
This section contains the complete API reference for the Engine framework.

Core Module
------------------------------------------------------------------------------------------------------

The core module provides fundamental functionality for the engine.

NamedStrings Class
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The ``NamedStrings`` class manages key-value pairs, commonly used for configuration files.

**Example Usage:**

.. code-block:: cpp

   NamedStrings config;
   config.SetValue("window_width", "1920");
   config.SetValue("window_height", "1080");

   int width = config.GetValue("window_width", 800);
   int height = config.GetValue("window_height", 600);

**Methods:**

* ``SetValue(const String& key, const String& value)`` - Sets a key-value pair
* ``GetValue(const String& key, const String& defaultValue)`` - Gets a string value
* ``GetValue(const String& key, int defaultValue)`` - Gets an integer value
* ``GetValue(const String& key, bool defaultValue)`` - Gets a boolean value

Vertex_PCUTBN Structure
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

A vertex structure containing position, color, UV coordinates, tangent, bitangent, and normal vectors.

**Members:**

* ``Vec3 m_position`` - 3D position vector
* ``Rgba8 m_color`` - RGBA color value
* ``Vec2 m_uvTexCoords`` - Texture coordinates
* ``Vec3 m_tangent`` - Tangent vector
* ``Vec3 m_bitangent`` - Bitangent vector
* ``Vec3 m_normal`` - Normal vector

**Example Usage:**

.. code-block:: cpp

   Vertex_PCUTBN vertex;
   vertex.m_position = Vec3(0.0f, 0.0f, 0.0f);
   vertex.m_color = Rgba8::WHITE;
   vertex.m_uvTexCoords = Vec2(0.0f, 0.0f);

Renderer Module
------------------------------------------------------------------------------------------------------

The renderer module handles all graphics and rendering operations.

Key Features
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* DirectX 11 backend support
* Texture management
* Shader system
* Window management
* Viewport rendering

**Main Classes:**

* ``Renderer`` - Main rendering class
* ``Texture`` - Texture resource management
* ``Shader`` - Shader compilation and management
* ``Window`` - Window creation and management

Math Module
------------------------------------------------------------------------------------------------------
The math module provides mathematical operations and data structures.

Vector Classes
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* ``Vec2`` - 2D vector operations
* ``Vec3`` - 3D vector operations

**Common Operations:**

.. code-block:: cpp

   Vec3 a(1.0f, 2.0f, 3.0f);
   Vec3 b(4.0f, 5.0f, 6.0f);

   Vec3 sum = a + b;
   float dot = DotProduct3D(a, b);
   Vec3 cross = CrossProduct3D(a, b);

Utility Functions
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* ``DotProduct3D()`` - Calculate dot product
* ``CrossProduct3D()`` - Calculate cross product
* Mathematical constants and helper functions