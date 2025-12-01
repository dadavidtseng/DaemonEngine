//----------------------------------------------------------------------------------------------------
// AudioTypes.hpp
// Shared Audio Type Definitions
//
// Purpose:
//   Centralized type definitions for audio system to ensure consistency across:
//   - AudioSystem (FMOD-based legacy audio)
//   - AudioCommandQueue (async JavaScript → C++ audio commands)
//   - Future audio subsystems
//
// Design Rationale:
//   - Single source of truth prevents type definition conflicts
//   - uint64_t ensures cross-platform consistency (32-bit/64-bit)
//   - JavaScript Number compatibility (53-bit safe integer range)
//   - Explicit 64-bit width vs size_t (platform-dependent)
//
// Author: Phase 2 - Audio Command Queue Implementation
// Date: 2025-11-30
//----------------------------------------------------------------------------------------------------

#pragma once

//----------------------------------------------------------------------------------------------------
#include <cstdint>

//----------------------------------------------------------------------------------------------------
/// @brief Unique identifier for loaded sound resources in the audio system
///
/// @remark Used as a handle to reference loaded sound files. Obtained from CreateOrGetSound().
/// @remark Fixed 64-bit width ensures cross-platform consistency and JavaScript interop safety.
/// @remark JavaScript Number type has 53-bit precision - uint64_t IDs within safe integer range.
///
/// @see MISSING_SOUND_ID for invalid sound identification
//----------------------------------------------------------------------------------------------------
using SoundID = uint64_t;

//----------------------------------------------------------------------------------------------------
/// @brief Unique identifier for active sound playback instances
///
/// @remark Tracks individual playing sound instances. Multiple playbacks can use the same SoundID.
/// @remark Obtained from StartSound() or StartSoundAt() calls for playback control.
/// @remark Fixed 64-bit width for consistency with SoundID type.
///
/// @see MISSING_SOUND_ID for invalid playback identification
//----------------------------------------------------------------------------------------------------
using SoundPlaybackID = uint64_t;

//----------------------------------------------------------------------------------------------------
/// @brief Sentinel value indicating invalid or missing sound/playback IDs
///
/// @remark Used for error checking and initialization of sound ID variables.
/// @remark Equivalent to UINT64_MAX (18,446,744,073,709,551,615) as invalid marker.
/// @remark Consistent with size_t(-1) pattern but with explicit 64-bit semantics.
///
/// @warning Do not use as valid ID - reserved for error indication only.
//----------------------------------------------------------------------------------------------------
uint64_t constexpr MISSING_SOUND_ID = static_cast<uint64_t>(-1);

//----------------------------------------------------------------------------------------------------
// Design Notes
//
// Why uint64_t Instead of size_t:
//   1. Semantic Clarity: IDs are not sizes/counts - they're opaque identifiers
//   2. Cross-Platform Consistency: size_t varies (32-bit vs 64-bit), uint64_t always 64-bit
//   3. JavaScript Interop: JavaScript Number type requires fixed-width integers for reliable FFI
//   4. Future-Proofing: No truncation issues when serializing/deserializing across platforms
//
// JavaScript Safe Integer Range:
//   - JavaScript Number uses IEEE 754 double-precision (53-bit significand)
//   - Safe integer range: [0, 2^53 - 1] = [0, 9,007,199,254,740,991]
//   - uint64_t max: 2^64 - 1 = 18,446,744,073,709,551,615
//   - Audio system should keep IDs within safe range (< 2^53) for JavaScript compatibility
//
// MISSING_SOUND_ID Value:
//   - Using UINT64_MAX (all bits set) as sentinel
//   - Outside JavaScript safe range - command queue should validate IDs before submission
//   - Consistent with existing size_t(-1) pattern in legacy AudioSystem code
//
// Migration from size_t:
//   - Legacy AudioSystem.hpp used size_t for SoundID/SoundPlaybackID
//   - Changed to uint64_t for consistency with AudioCommand.hpp
//   - No binary compatibility issues (all code recompiles together)
//   - Vector indexing auto-converts uint64_t → size_t safely (sound count << 2^64)
//----------------------------------------------------------------------------------------------------
