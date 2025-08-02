Welcome to DaemonEngine!
========================

.. image:: _static/images/daemon-engine-logo.png
   :width: 300
   :align: center
   :alt: DaemonEngine Logo

DaemonEngine is a powerful, lightweight 2D game engine built with modern C++. 
Designed for performance and ease of use, it provides everything you need to create amazing 2D games.

.. raw:: html

   <div class="engine-feature">
       <h3>🚀 Ready to Get Started?</h3>
       <p>Jump into game development with our comprehensive guides and examples!</p>
   </div>

.. note::
   **Development Status**: DaemonEngine is currently in active development. 
   Features and APIs may change as we work towards the first stable release.

Core Features
-------------

🎨 **High-Performance 2D Rendering**
   Modern DirectX 11 backend with optimized graphics pipeline for smooth 60+ FPS gameplay

🧮 **Complete Math Library**
   Comprehensive mathematical operations including vectors, matrices, and geometric calculations

🎵 **Integrated Audio System**
   Built-in FMOD support for rich, immersive audio experiences

🔧 **Modular Architecture**
   Clean, extensible design that makes it easy to add custom features and systems

⚡ **Cross-Platform Ready**
   Currently supports Windows with plans for Linux and macOS support

🛠️ **Developer Friendly**
   Extensive documentation, examples, and debugging tools to accelerate development

Quick Navigation
----------------

.. raw:: html

   <div style="display: grid; grid-template-columns: repeat(auto-fit, minmax(250px, 1fr)); gap: 20px; margin: 20px 0;">
       <div style="border: 2px solid #3498db; border-radius: 8px; padding: 20px; text-align: center; background: #f8f9fa;">
           <h3 style="color: #2c3e50; margin-top: 0;">🚀 Getting Started</h3>
           <p>New to DaemonEngine? Start here to set up your development environment and create your first project.</p>
           <a href="quickstart.html" style="background: #3498db; color: white; padding: 8px 16px; text-decoration: none; border-radius: 4px; display: inline-block;">Get Started →</a>
       </div>

       <div style="border: 2px solid #27ae60; border-radius: 8px; padding: 20px; text-align: center; background: #f8f9fa;">
           <h3 style="color: #2c3e50; margin-top: 0;">📚 API Reference</h3>
           <p>Complete API documentation overview with links to detailed module documentation and usage guides.</p>
           <a href="api.html" style="background: #27ae60; color: white; padding: 8px 16px; text-decoration: none; border-radius: 4px; display: inline-block;">Browse API →</a>
       </div>

       <div style="border: 2px solid #e74c3c; border-radius: 8px; padding: 20px; text-align: center; background: #f8f9fa;">
           <h3 style="color: #2c3e50; margin-top: 0;">🎮 Tutorials</h3>
           <p>Step-by-step guides to learn DaemonEngine features through practical examples and projects.</p>
           <a href="quickstart.html" style="background: #e74c3c; color: white; padding: 8px 16px; text-decoration: none; border-radius: 4px; display: inline-block;">Learn More →</a>
       </div>
   </div>

.. raw:: html

   <div style="margin: 30px 0;">
       <h3 style="color: #2c3e50; text-align: center; margin-bottom: 20px;">🔧 Module Documentation</h3>
       <div style="display: grid; grid-template-columns: repeat(auto-fit, minmax(280px, 1fr)); gap: 20px;">
           <div style="border: 2px solid #9b59b6; border-radius: 8px; padding: 20px; text-align: center; background: #f8f9fa;">
               <h4 style="color: #2c3e50; margin-top: 0;">⚙️ Core Module</h4>
               <p>Essential data structures, vertex definitions, and string management classes that form the foundation of the engine.</p>
               <a href="core_module.html" style="background: #9b59b6; color: white; padding: 8px 16px; text-decoration: none; border-radius: 4px; display: inline-block;">Core API →</a>
           </div>

           <div style="border: 2px solid #f39c12; border-radius: 8px; padding: 20px; text-align: center; background: #f8f9fa;">
               <h4 style="color: #2c3e50; margin-top: 0;">🎨 Renderer Module</h4>
               <p>DirectX 11 rendering system, graphics pipeline management, and visual effects processing classes.</p>
               <a href="renderer_module.html" style="background: #f39c12; color: white; padding: 8px 16px; text-decoration: none; border-radius: 4px; display: inline-block;">Renderer API →</a>
           </div>
       </div>
   </div>

System Requirements
-------------------

**Minimum Requirements:**

* **Operating System**: Windows 10 (64-bit) or later
* **Compiler**: Visual Studio 2019 or later with C++17 support
* **Graphics**: DirectX 11 compatible graphics card
* **Memory**: 4 GB RAM
* **Storage**: 500 MB available space

**Recommended:**

* **Operating System**: Windows 11 (64-bit)
* **Compiler**: Visual Studio 2022 with latest updates
* **Graphics**: Dedicated GPU with 2+ GB VRAM
* **Memory**: 8 GB RAM or more
* **Storage**: 1 GB available space (for development)

Quick Example
-------------

Here's a simple example of creating a window with DaemonEngine:

.. code-block:: cpp
   :caption: main.cpp - Basic Window Creation
   :linenos:

   #include "Engine/Core/Engine.hpp"

   int main()
   {
       // Initialize the engine
       Engine engine;
       engine.Initialize();

       // Main game loop
       while (engine.IsRunning())
       {
           engine.Update();
           engine.Render();
       }

       // Cleanup
       engine.Shutdown();
       return 0;
   }

.. tip::
   **Pro Tip**: Always call ``engine.Initialize()`` before starting the main loop,
   and ``engine.Shutdown()`` when your application exits for proper resource cleanup.

Latest Updates
--------------

.. todo::
   Add changelog and recent updates section here.

**Version 1.0.0** (Current Development)

* ✅ Core engine architecture
* ✅ DirectX 11 rendering backend
* ✅ Math library implementation
* ✅ FMOD audio integration
* 🚧 Documentation and examples
* 🚧 Cross-platform support

Community & Support
--------------------

.. raw:: html

   <div style="background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; padding: 20px; border-radius: 8px; margin: 20px 0;">
       <h3 style="color: white; margin-top: 0;">🤝 Join the Community</h3>
       <p style="margin-bottom: 15px;">Get help, share your projects, and connect with other developers:</p>
       <div style="display: flex; gap: 15px; flex-wrap: wrap;">
           <a href="https://github.com/dadavidtseng/DaemonEngine" target="_blank"
              style="background: rgba(255,255,255,0.2); color: white; padding: 8px 16px; text-decoration: none; border-radius: 4px; display: inline-block;">
              📁 GitHub Repository
           </a>
           <a href="https://github.com/dadavidtseng/DaemonEngine/issues" target="_blank"
              style="background: rgba(255,255,255,0.2); color: white; padding: 8px 16px; text-decoration: none; border-radius: 4px; display: inline-block;">
              🐛 Report Issues
           </a>
           <a href="https://github.com/dadavidtseng/DaemonEngine/discussions" target="_blank"
              style="background: rgba(255,255,255,0.2); color: white; padding: 8px 16px; text-decoration: none; border-radius: 4px; display: inline-block;">
              💬 Discussions
           </a>
       </div>
   </div>

License
-------

DaemonEngine is released under the MIT License. See the `LICENSE <https://github.com/dadavidtseng/DaemonEngine/blob/main/LICENSE>`_ file for details.

Documentation Contents
----------------------

.. toctree::
   :maxdepth: 2
   :caption: 📖 Documentation
   :hidden:

   quickstart
   api
   core_module
   renderer_module

.. toctree::
   :maxdepth: 1
   :caption: 🔗 External Links
   :hidden:

   GitHub Repository <https://github.com/dadavidtseng/DaemonEngine>
   Issue Tracker <https://github.com/dadavidtseng/DaemonEngine/issues>
   Discussions <https://github.com/dadavidtseng/DaemonEngine/discussions>

Indices and Tables
==================

* :ref:`genindex`
* :ref:`search`

----

.. raw:: html

   <div style="text-align: center; margin: 40px 0; color: #7f8c8d; font-size: 0.9em;">
       <p>Made with ❤️ for the game development community</p>
       <p>© 2025 Yu-Wei Tseng. Documentation powered by 
          <a href="https://www.sphinx-doc.org/" target="_blank" style="color: #3498db;">Sphinx</a> and 
          <a href="https://readthedocs.org/" target="_blank" style="color: #3498db;">Read the Docs</a>
       </p>
   </div>