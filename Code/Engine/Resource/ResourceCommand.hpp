//----------------------------------------------------------------------------------------------------
// ResourceCommand.hpp
// Phase 3: Resource Command Queue - Resource Command Definitions
//
// Purpose:
//   Type-safe command structures for JavaScript → C++ resource loading communication.
//   Uses std::variant for zero-cost, type-safe payload storage.
//   Integrates with JobSystem for asynchronous I/O operations on worker threads.
//
// Design Decisions:
//   - std::variant over std::any: Zero-cost abstraction, compile-time type checking
//   - ResourceID as uint64_t: JavaScript Number type compatibility (53-bit safe integer)
//   - String for path storage: Engine convention for file paths
//   - Priority field: Enables load ordering for time-critical resources
//   - Async flag: Determines whether to use JobSystem or immediate loading
//
// Thread Safety:
//   - Immutable after construction (no mutation after submission to queue)
//   - Copyable and movable for queue operations
//   - ResourceLoadJob processes commands on I/O worker threads
//
// Author: Phase 3 - Resource Command Queue Implementation
// Date: 2025-12-01
//----------------------------------------------------------------------------------------------------

#pragma once

//----------------------------------------------------------------------------------------------------
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Resource/ResourceCommon.hpp"

#include <optional>
#include <variant>

//----------------------------------------------------------------------------------------------------
// ResourceCommandType Enumeration
//
// Defines all async command types supported by the resource command queue.
// Each type maps to a corresponding payload structure.
//
// Command Flow:
//   JavaScript → ResourceCommandQueue → ResourceLoadJob (JobSystem I/O workers) → CallbackQueue
//
// Usage Examples:
//   LOAD_TEXTURE:  Load texture file asynchronously (PNG, TGA, etc.)
//   LOAD_MODEL:    Load 3D model file asynchronously (OBJ, FBX, etc.)
//   LOAD_SHADER:   Load and compile shader file asynchronously (HLSL)
//   LOAD_AUDIO:    Load audio file asynchronously (WAV, MP3, OGG via FMOD)
//   LOAD_FONT:     Load bitmap font file asynchronously
//   UNLOAD_RESOURCE: Remove resource from cache and free memory
//----------------------------------------------------------------------------------------------------
enum class eResourceCommandType : uint8_t
{
	LOAD_TEXTURE,    // Load texture file from disk → GPU upload (main thread)
	LOAD_MODEL,      // Load 3D model geometry from disk
	LOAD_SHADER,     // Load and compile shader code
	LOAD_AUDIO,      // Load audio file via FMOD subsystem
	LOAD_FONT,       // Load bitmap font texture and glyph data
	UNLOAD_RESOURCE  // Remove resource from cache (reference counting)
};

//----------------------------------------------------------------------------------------------------
// Command Payload Structures
//
// Each structure contains the minimum data required for the corresponding command.
// Design: Immutable, value-semantic, POD-like for efficient queue storage.
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
// sTextureLoadData
// Payload for LOAD_TEXTURE command
//
// Usage:
//   JavaScript: resource.loadTexture("Data/Images/TestUV.png", (textureId) => { ... });
//   Result: Texture loaded on I/O thread, GPU upload on main thread, callback with ResourceID
//----------------------------------------------------------------------------------------------------
struct sTextureLoadData
{
	String path;         // File path relative to Run/ directory (e.g., "Data/Images/TestUV.png")
	uint64_t callbackId; // CallbackQueue notification ID
	int priority;        // Load priority (higher = earlier): -100 (low) to 100 (critical)
	bool async;          // Use JobSystem (true) or immediate load (false)
};

//----------------------------------------------------------------------------------------------------
// sModelLoadData
// Payload for LOAD_MODEL command
//
// Usage:
//   JavaScript: resource.loadModel("Data/Models/Cube/Cube_vni.obj", (modelId) => { ... });
//   Result: Model parsed on I/O thread, GPU upload on main thread, callback with ResourceID
//----------------------------------------------------------------------------------------------------
struct sModelLoadData
{
	String path;         // File path relative to Run/ directory (e.g., "Data/Models/Woman/Woman.obj")
	uint64_t callbackId; // CallbackQueue notification ID
	int priority;        // Load priority: -100 (low) to 100 (critical)
	bool async;          // Use JobSystem (true) or immediate load (false)
};

//----------------------------------------------------------------------------------------------------
// sShaderLoadData
// Payload for LOAD_SHADER command
//
// Usage:
//   JavaScript: resource.loadShader("Data/Shaders/BlinnPhong.hlsl", (shaderId) => { ... });
//   Result: Shader compiled on I/O thread (CPU), GPU upload on main thread, callback with ResourceID
//
// Note: DirectX shader compilation is CPU-intensive, benefits from JobSystem offloading
//----------------------------------------------------------------------------------------------------
struct sShaderLoadData
{
	String path;         // File path relative to Run/ directory (e.g., "Data/Shaders/Default.hlsl")
	uint64_t callbackId; // CallbackQueue notification ID
	int priority;        // Load priority: -100 (low) to 100 (critical)
	bool async;          // Use JobSystem (true) or immediate load (false)
};

//----------------------------------------------------------------------------------------------------
// sAudioLoadData
// Payload for LOAD_AUDIO command
//
// Usage:
//   JavaScript: resource.loadAudio("Data/Audio/TestSound.mp3", (audioId) => { ... });
//   Result: Audio file loaded via FMOD on I/O thread, callback with ResourceID
//
// Note: FMOD supports streaming, large audio files should use async loading
//----------------------------------------------------------------------------------------------------
struct sAudioLoadData
{
	String path;         // File path relative to Run/ directory (e.g., "Data/Audio/TestSound.mp3")
	uint64_t callbackId; // CallbackQueue notification ID
	int priority;        // Load priority: -100 (low) to 100 (critical)
	bool async;          // Use JobSystem (true) or immediate load (false)
};

//----------------------------------------------------------------------------------------------------
// sFontLoadData
// Payload for LOAD_FONT command
//
// Usage:
//   JavaScript: resource.loadFont("Data/Fonts/DaemonFont", (fontId) => { ... });
//   Result: Bitmap font texture and glyph data loaded on I/O thread, callback with ResourceID
//
// Note: Path should exclude file extension (e.g., "DaemonFont" not "DaemonFont.png")
//----------------------------------------------------------------------------------------------------
struct sFontLoadData
{
	String path;         // File path relative to Run/ directory WITHOUT extension (e.g., "Data/Fonts/DaemonFont")
	uint64_t callbackId; // CallbackQueue notification ID
	int priority;        // Load priority: -100 (low) to 100 (critical)
	bool async;          // Use JobSystem (true) or immediate load (false)
};

//----------------------------------------------------------------------------------------------------
// sResourceUnloadData
// Payload for UNLOAD_RESOURCE command
//
// Usage:
//   JavaScript: resource.unloadResource(resourceId, (success) => { ... });
//   Result: Resource reference count decremented, memory freed if count reaches zero
//
// Note: Resources use reference counting, safe to "unload" multiple times
//----------------------------------------------------------------------------------------------------
struct sResourceUnloadData
{
	uint64_t resourceId; // ResourceID to unload
	eResourceType type;  // Resource type (for correct cache lookup)
	uint64_t callbackId; // CallbackQueue notification ID
};

//----------------------------------------------------------------------------------------------------
// sResourceCommand
//
// Type-safe command structure using std::variant for payload storage.
// Guarantees zero-cost abstraction (no virtual function overhead).
//
// Memory Layout:
//   - type: 1 byte (enum)
//   - data: ~280 bytes (largest variant = sTextureLoadData with String path)
//   Total: ~280 bytes per command (cache-friendly)
//
// Thread Safety:
//   - Immutable after construction
//   - Safe to copy across thread boundaries
//   - Processed by ResourceLoadJob on I/O worker threads
//
// Performance Characteristics:
//   - Command queue capacity: 200 commands × 280 bytes = ~56 KB memory overhead
//   - Typical load rate: 1-10 commands/second (game runtime)
//   - Burst load scenario: 50-100 commands (level transitions, scene changes)
//   - Maximum: 200 commands (queue full → backpressure)
//----------------------------------------------------------------------------------------------------
struct sResourceCommand
{
	eResourceCommandType type;

	// Type-safe payload using std::variant
	// std::monostate for commands without payload (future extensibility)
	std::variant<std::monostate,
	             sTextureLoadData,
	             sModelLoadData,
	             sShaderLoadData,
	             sAudioLoadData,
	             sFontLoadData,
	             sResourceUnloadData>
	    data;

	// Default constructor (required for array initialization)
	sResourceCommand()
	    : type(eResourceCommandType::LOAD_TEXTURE)
	    , data(std::monostate{})
	{
	}

	// Explicit constructor for type safety
	sResourceCommand(eResourceCommandType cmdType, auto&& payload)
	    : type(cmdType)
	    , data(std::forward<decltype(payload)>(payload))
	{
	}
};

//----------------------------------------------------------------------------------------------------
// Performance Characteristics (Phase 3 Targets)
//
// Command Size: ~280 bytes (String path overhead)
// Queue Capacity: 200 commands × 280 bytes = ~56 KB memory overhead
// Submission Latency: < 0.5ms (lock-free atomic operations)
//
// Expected Command Rates:
//   - Game startup: 50-100 commands (loading initial assets)
//   - Runtime: 1-10 commands/second (dynamic asset loading)
//   - Level transition: 50-200 commands (burst loading)
//   - Maximum: 200 commands (queue full → backpressure)
//
// Priority Guidelines:
//   100: Critical UI assets (loading screens, fonts)
//   50: Player character textures, essential gameplay models
//   0: Standard game assets (default priority)
//   -50: Background decoration, non-critical assets
//   -100: Pre-cached resources, optional content
//
// Async vs Sync Loading:
//   - Async (JobSystem): Default for most resources, non-blocking
//   - Sync (immediate): Only for critical resources needed immediately
//   - GPU uploads always happen on main thread (DirectX requirement)
//----------------------------------------------------------------------------------------------------
