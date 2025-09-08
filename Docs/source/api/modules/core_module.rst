Core Module
===========

The Core Module contains Daemon Engine's core data structures and basic functionality classes. These classes provide the fundamental components required for engine operation.

Overview
--------

The Core Module primarily provides the following functionality:

* **Data Structure Definitions** - Defines basic data types used by the engine
* **String Management** - Provides efficient string processing capabilities
* **Basic Utility Classes** - Supporting auxiliary functions for other modules

StringUtils Utility Functions
------------------------------

Comprehensive string manipulation utilities providing printf-style formatting, modern C++20 format support, and string splitting functionality.

Key Features:

* **Printf-style Formatting** - Traditional C-style string formatting with automatic memory management
* **C++20 Format Support** - Modern type-safe formatting with compile-time validation
* **String Splitting** - Advanced delimiter-based and line-based string parsing
* **Cross-platform Compatibility** - Handles different line ending formats

.. doxygenfile:: StringUtils.hpp

Usage Examples
~~~~~~~~~~~~~~

.. code-block:: cpp

   #include "Engine/Core/StringUtils.hpp"
   
   // Printf-style formatting
   String message = Stringf("Player has %d health", playerHealth);
   
   // Modern C++20 formatting (type-safe)
   String modernMsg = StringFormat("Player {} has {} health", playerName, playerHealth);
   
   // String splitting
   StringList tokens = SplitStringOnDelimiter("apple,banana,orange", ',');
   
   // Line-based splitting
   StringList lines;
   int lineCount = SplitStringIntoLines(lines, multiLineText);

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

* :doc:`../api` - Back to API Overview
* :doc:`render_module` - Renderer Module Documentation
* :doc:`../quickstart` - Quick Start Guide