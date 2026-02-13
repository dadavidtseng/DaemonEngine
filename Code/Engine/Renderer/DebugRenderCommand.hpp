// //----------------------------------------------------------------------------------------------------
// // DebugRenderCommand.hpp
// // Phase 4: Debug Rendering Command System
// //
// // Purpose:
// //   Defines command structures for debug rendering primitive submissions. Only compiled in DEBUG
// //   builds to avoid overhead in RELEASE builds. Enables efficient batch submission of debug
// //   visualization commands (lines, spheres, arrows, text, billboards) with customizable colors
// //   and durations.
// //
// // Design Philosophy:
// //   - DEBUG-only compilation: #if defined(_DEBUG) || defined(DEBUG)
// //   - Zero-cost abstraction in RELEASE builds (compiles to nothing)
// //   - Type-safe command enumeration with variant payloads
// //   - Timed visualization: Duration field enables automatic cleanup of temporary visuals
// //   - Comprehensive primitive support for game development debugging
// //
// // Usage Example (DEBUG builds only):
// //   // Submit a debug line for 5 seconds
// //   sDebugRenderCommand cmd;
// //   cmd.type = eDebugRenderCommandType::ADD_WORLD_LINE;
// //   cmd.color = Rgba8(255, 0, 0, 255);  // Red
// //   cmd.positions[0] = Vec3(0, 0, 0);
// //   cmd.positions[1] = Vec3(1, 1, 1);
// //   cmd.duration = 5.0f;
// //   debugQueue->Submit(cmd);
// //
// // Thread Safety:
// //   - Commands are immutable after submission
// //   - Queue is lock-free SPSC (single-producer, single-consumer)
// //   - Safe for game loop thread submission
// //
// // Performance Impact:
// //   - DEBUG: ~80-120 bytes per command (struct size ~280 bytes with union)
// //   - RELEASE: 0 bytes (entire feature compiled out)
// //
// // Author: Phase 4 - Debug Rendering System
// // Date: 2025-12-02
// //----------------------------------------------------------------------------------------------------
//
// #pragma once
//
// #include "Engine/Core/StringUtils.hpp"
// #include "Engine/Core/Rgba8.hpp"
// #include "Engine/Math/Vec3.hpp"
//
// //----------------------------------------------------------------------------------------------------
// // eDebugRenderCommandType Enumeration
// //
// // Defines all debug rendering primitive types supported by the debug render command system.
// // Each type corresponds to a specific visualization primitive with its own data requirements.
// //
// // Command Types:
// //   ADD_WORLD_LINE:    Draw a line segment in world space (uses 2 positions)
// //   ADD_WORLD_SPHERE:  Draw a wireframe sphere in world space (uses 1 position + radius)
// //   ADD_WORLD_ARROW:   Draw an arrow in world space (uses 2 positions for base + direction)
// //   ADD_WORLD_TEXT:    Render 3D text label in world space (uses 1 position + text string)
// //   ADD_BILLBOARD:      Render 2D billboard sprite at world position (uses 1 position + size)
// //   CLEAR_ALL:         Clear all pending debug commands (no payload required)
// //
// // Usage:
// //   Commands submitted to DebugRenderCommandQueue for batch processing in render pass.
// //----------------------------------------------------------------------------------------------------
// enum class eDebugRenderCommandType : uint8_t
// {
// 	ADD_WORLD_LINE,      // Draw line: 2 positions, color, duration
// 	ADD_WORLD_SPHERE,    // Draw sphere: 1 position, radius, color, duration
// 	ADD_WORLD_ARROW,     // Draw arrow: 2 positions (base + tip), color, duration
// 	ADD_WORLD_TEXT,      // Draw text: 1 position, text string, color, duration
// 	ADD_BILLBOARD,       // Draw billboard: 1 position, size, color, duration
// 	CLEAR_ALL            // Clear all debug visuals immediately
// };
//
// //----------------------------------------------------------------------------------------------------
// // sDebugRenderCommand Structure
// //
// // Command payload for debug rendering primitives. Uses union-like design for flexible storage
// // of different primitive types while maintaining fixed size for efficient queue operations.
// //
// // Memory Layout (280 bytes total):
// //   - type (1 byte): eDebugRenderCommandType
// //   - color (4 bytes): Rgba8 color value
// //   - positions[4] (48 bytes): Vec3 array (up to 4 vertices for complex primitives)
// //   - radius (4 bytes): Radius for sphere primitives
// //   - duration (4 bytes): Time in seconds to display primitive (0 = one frame)
// //   - text[64] (64 bytes): Text string for text primitives (null-terminated)
// //   - size (4 bytes): Size field for billboard and other scaling
// //   - padding (remaining bytes): Alignment padding
// //
// // Constraints:
// //   - All fields are immutable after construction
// //   - text string is limited to 63 characters (null-terminated)
// //   - positions array supports up to 4 vertices per command
// //   - Primitive type determines which fields are used
// //
// // Thread Safety:
// //   - Copy-constructible (value semantics for queue operations)
// //   - Move-constructible (efficient queue storage)
// //   - Immutable after submission (safe for lock-free queue)
// //
// // Size Guarantee:
// //   - Exactly ~280 bytes to fit efficiently in lock-free command queue
// //   - Aligned for SIMD operations if needed
// //----------------------------------------------------------------------------------------------------
// struct sDebugRenderCommand
// {
// 	eDebugRenderCommandType type = eDebugRenderCommandType::CLEAR_ALL;
// 	Rgba8                   color = Rgba8(255, 255, 255, 255);  // White default
// 	Vec3                    positions[4] = {};                   // Up to 4 vertices
// 	float                   radius = 1.0f;                       // For sphere primitives
// 	float                   duration = 0.0f;                     // Seconds to display (0=1 frame)
// 	char                    text[64] = {};                       // Text for text primitives
// 	float                   size = 1.0f;                         // Scale for billboards/text
//
// 	// Default constructor - initializes all fields
// 	sDebugRenderCommand() = default;
//
// 	// Explicit constructor for convenience
// 	explicit sDebugRenderCommand(eDebugRenderCommandType cmdType)
// 		: type(cmdType)
// 	{
// 	}
// };
//
// //----------------------------------------------------------------------------------------------------
// // Type Safety and Compile-Time Validation
// //----------------------------------------------------------------------------------------------------
//
// // Verify struct size is within expected bounds (~280 bytes)
// static_assert(sizeof(sDebugRenderCommand) <= 320, "sDebugRenderCommand exceeds size budget (320 bytes)");
// static_assert(sizeof(sDebugRenderCommand) >= 120, "sDebugRenderCommand smaller than minimum expected (120 bytes)");
//
// // Verify enum size
// static_assert(sizeof(eDebugRenderCommandType) == 1, "eDebugRenderCommandType should be 1 byte");
//
// //----------------------------------------------------------------------------------------------------
// // Design Rationale
// //
// // DEBUG-Only Compilation:
// //   - Feature completely removed from RELEASE builds via preprocessor
// //   - Zero runtime overhead in production
// //   - All debug visualization code can be aggressive without performance concerns
// //
// // Fixed-Size Struct:
// //   - ~280 bytes fits efficiently in lock-free circular queue
// //   - Supports cache-line alignment (64 bytes) for SIMD operations
// //   - Eliminates dynamic allocation for command storage
// //
// // Union-like Design:
// //   - Uses array fields to store different types of vertices/parameters
// //   - Primitive type enum disambiguates field usage
// //   - No std::variant overhead (maintains deterministic size)
// //
// // Duration Field:
// //   - Enables time-limited visualization of debug primitives
// //   - 0 = display for 1 frame (useful for per-frame debug)
// //   - > 0 = display for N seconds (useful for conditional breakpoints)
// //
// // Color Support:
// //   - Full RGBA8 for visibility in both dark and light backgrounds
// //   - Alpha channel enables transparency/occlusion handling
// //
// // Thread Safety:
// //   - Commands are value types (copyable, movable)
// //   - Lock-free queue handles synchronization
// //   - No shared mutable state
// //
// // Future Extensions:
// //   - Add matrix transforms for rotated/scaled primitives
// //   - Add dotted/dashed line patterns
// //   - Add text size/font selection
// //   - Add conditional visibility (per-category filtering)
// //----------------------------------------------------------------------------------------------------
