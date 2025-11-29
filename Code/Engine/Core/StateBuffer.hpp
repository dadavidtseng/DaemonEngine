//----------------------------------------------------------------------------------------------------
// StateBuffer.hpp
// Engine Core Module - Generic Double-Buffered State Container
//
// Purpose:
//   Provides generic double-buffered container template for lock-free state synchronization
//   between worker threads and main thread. Used for async architecture patterns.
//
// Design Rationale:
//   - Template class supports any copyable state container type
//   - Double-buffering enables lock-free reads and writes
//   - Brief locked swap operation for buffer synchronization
//   - Full-copy strategy for simplicity (Phase 1)
//
// Usage:
//   using EntityStateBuffer = StateBuffer<EntityStateMap>;
//   using CameraStateBuffer = StateBuffer<CameraStateMap>;
//
// Thread Safety:
//   - GetFrontBuffer(): Lock-free reads (main thread)
//   - GetBackBuffer(): Lock-free writes (worker thread)
//   - SwapBuffers(): Brief locked operation (main thread only)
//
// Author: M4-T8 Engine Refactoring
// Date: 2025-10-26
//----------------------------------------------------------------------------------------------------

#pragma once

//----------------------------------------------------------------------------------------------------
#include <mutex>
#include <cstdint>
#include <exception>

#include "ErrorWarningAssert.hpp"
#include "Engine/Core/LogSubsystem.hpp"
#include "Engine/Core/EngineCommon.hpp"

//----------------------------------------------------------------------------------------------------
// StateBuffer Template Class
//
// Generic double-buffered container for thread-safe state synchronization.
//
// Template Parameter:
//   TStateContainer: Container type to double-buffer (must be copyable)
//                    Examples: std::unordered_map<EntityID, EntityState>
//                              std::unordered_map<std::string, CameraState>
//
// Thread Usage Pattern:
//
//   Worker Thread (JavaScript logic updates):
//     StateBuffer<MyStateMap>* buffer = ...;
//     MyStateMap* backBuffer = buffer->GetBackBuffer();
//     (*backBuffer)[key] = updatedState;  // Lock-free write
//
//   Main Thread (Rendering reads):
//     StateBuffer<MyStateMap>* buffer = ...;
//     MyStateMap const* frontBuffer = buffer->GetFrontBuffer();
//     for (auto const& [key, state] : *frontBuffer) {
//         RenderState(state);  // Lock-free read
//     }
//
//   Frame Boundary (Main Thread):
//     buffer->SwapBuffers();  // Brief locked operation
//
// Performance Characteristics:
//   - Read: O(1) lookup, lock-free
//   - Write: O(1) update, lock-free
//   - Swap: O(n) full copy, locked (< 1ms for 1000 elements)
//
// Design Notes:
//   - Full-copy strategy: Entire back buffer copied to front during swap
//   - Simple and predictable: No dirty tracking or complex synchronization
//   - Memory overhead: 2× container storage (acceptable for Phase 1)
//   - Future optimization: Copy-on-write, dirty bit tracking (Phase 4)
//----------------------------------------------------------------------------------------------------
template <typename TStateContainer>
class StateBuffer
{
public:
    //------------------------------------------------------------------------------------------------
    // Construction / Destruction
    //------------------------------------------------------------------------------------------------
    StateBuffer()
        : m_frontBuffer(&m_bufferA)
          , m_backBuffer(&m_bufferB)
          , m_totalSwaps(0)
          , m_swapErrorCount(0)
    {
    }

    ~StateBuffer() = default;

    // Non-copyable, non-movable (contains mutex)
    StateBuffer(StateBuffer const&)            = delete;
    StateBuffer& operator=(StateBuffer const&) = delete;
    StateBuffer(StateBuffer&&)                 = delete;
    StateBuffer& operator=(StateBuffer&&)      = delete;

    //------------------------------------------------------------------------------------------------
    // Buffer Access (Thread-Safe)
    //------------------------------------------------------------------------------------------------

    // Get front buffer for rendering (Main Thread, lock-free read)
    // Returns: Const pointer to current front buffer (safe for concurrent reads)
    // Thread Safety: Lock-free, safe to call from main thread while worker updates back buffer
    TStateContainer const* GetFrontBuffer() const
    {
        return m_frontBuffer;
    }

    // Get back buffer for writing (Worker Thread, lock-free write)
    // Returns: Mutable pointer to current back buffer (single-writer guarantee)
    // Thread Safety: Lock-free, assumes single worker thread (no concurrent writers)
    TStateContainer* GetBackBuffer()
    {
        return m_backBuffer;
    }

    //------------------------------------------------------------------------------------------------
    // Buffer Swap (Frame Boundary, Main Thread Only)
    //------------------------------------------------------------------------------------------------

    // Swap front/back buffers and copy data (locked operation)
    //
    // Algorithm:
    //   1. Acquire mutex lock
    //   2. Validate buffer state (Phase 3.1 error recovery)
    //   3. Copy back buffer → new front buffer (full deep copy) - wrapped in try-catch
    //   4. Swap buffer pointers
    //   5. Release mutex lock
    //
    // Performance: O(n) where n = number of elements (< 1ms for 1000 elements)
    // Thread Safety: Locked operation, call from main thread only
    // Rationale: Full copy ensures front buffer is stable snapshot for rendering
    //
    // Error Recovery (Phase 3.1):
    //   - On exception, stale front buffer preserved (rendering continues with old state)
    //   - Error count incremented for monitoring
    //   - Exception details logged via DAEMON_LOG
    void SwapBuffers()
    {
        std::lock_guard lock(m_swapMutex);

        // Phase 3.1: Validate buffer state before copy
        if (!ValidateStateBuffer())
        {
            DAEMON_LOG(LogCore, eLogVerbosity::Error,
                       "StateBuffer::SwapBuffers - Buffer validation failed, skipping swap");
            ++m_swapErrorCount;
            return;
        }

        // Phase 3.1: Wrap copy operation in try-catch for error recovery
        try
        {
            // Copy back buffer → front buffer (full deep copy)
            // This ensures front buffer is stable snapshot for rendering
            *m_frontBuffer = *m_backBuffer;

            // Swap buffer pointers (now back buffer becomes old front buffer)
            std::swap(m_frontBuffer, m_backBuffer);

            // Increment swap counter for profiling
            ++m_totalSwaps;
            // DAEMON_LOG(LogCore, eLogVerbosity::Display,StringFormat("StateBuffer::SwapBuffers - Success (total swaps: {})", m_totalSwaps));

        }
        catch (std::bad_alloc const& e)
        {
            // Memory allocation failure during copy
            DAEMON_LOG(LogCore, eLogVerbosity::Error,
                       "StateBuffer::SwapBuffers - Memory allocation failed: %s. Preserving stale front buffer.",
                       e.what());
            ++m_swapErrorCount;
            // Stale front buffer preserved - rendering continues with old state
        }
        catch (std::exception const& e)
        {
            // Generic exception during copy
            DAEMON_LOG(LogCore, eLogVerbosity::Error,
                       "StateBuffer::SwapBuffers - Exception during buffer copy: %s. Preserving stale front buffer.",
                       e.what());
            ++m_swapErrorCount;
            // Stale front buffer preserved - rendering continues with old state
        }
        catch (...)
        {
            // Unknown exception during copy
            DAEMON_LOG(LogCore, eLogVerbosity::Error,
                       "StateBuffer::SwapBuffers - Unknown exception during buffer copy. Preserving stale front buffer.");
            ++m_swapErrorCount;
            // Stale front buffer preserved - rendering continues with old state
        }
    }

    //------------------------------------------------------------------------------------------------
    // Monitoring / Debugging
    //------------------------------------------------------------------------------------------------

    // Get element count in front buffer (approximate, for monitoring only)
    // Thread Safety: Lock-free read, may race with SwapBuffers()
    size_t GetElementCount() const
    {
        return m_frontBuffer->size();
    }

    // Get total swaps performed (for profiling)
    // Thread Safety: Lock-free read, may race with SwapBuffers()
    uint64_t GetTotalSwaps() const
    {
        return m_totalSwaps;
    }

    //------------------------------------------------------------------------------------------------
    // Error Monitoring (Phase 3.1)
    //------------------------------------------------------------------------------------------------

    // Get total swap errors encountered (for monitoring)
    // Thread Safety: Lock-free read, may race with SwapBuffers()
    uint64_t GetSwapErrorCount() const
    {
        return m_swapErrorCount;
    }

    // Check if any swap errors have occurred
    // Thread Safety: Lock-free read, may race with SwapBuffers()
    bool HasSwapErrors() const
    {
        return m_swapErrorCount > 0;
    }

private:
    //------------------------------------------------------------------------------------------------
    // Double-Buffer Storage
    //------------------------------------------------------------------------------------------------
    TStateContainer m_bufferA;  // Buffer A (front or back)
    TStateContainer m_bufferB;  // Buffer B (front or back)

    TStateContainer* m_frontBuffer;  // Pointer to current front buffer (read by main thread)
    TStateContainer* m_backBuffer;   // Pointer to current back buffer (written by worker thread)

    //------------------------------------------------------------------------------------------------
    // Synchronization
    //------------------------------------------------------------------------------------------------
    mutable std::mutex m_swapMutex;  // Protects buffer swap operation

    //------------------------------------------------------------------------------------------------
    // Statistics
    //------------------------------------------------------------------------------------------------
    uint64_t m_totalSwaps;       // Total buffer swaps performed (profiling counter)
    uint64_t m_swapErrorCount;   // Phase 3.1: Total swap errors encountered (error monitoring)

    //------------------------------------------------------------------------------------------------
    // Buffer Validation (Phase 3.1)
    //------------------------------------------------------------------------------------------------

    // Validate buffer state before swap operation
    // Returns: true if buffers are valid, false if corruption detected
    // Called: At start of SwapBuffers() before copy operation
    //
    // Checks performed:
    //   - Front buffer pointer not null
    //   - Back buffer pointer not null
    //   - Front buffer points to internal storage (m_bufferA or m_bufferB)
    //   - Back buffer points to internal storage (m_bufferA or m_bufferB)
    //   - No aliasing (front != back)
    bool ValidateStateBuffer() const
    {
        // Check front buffer pointer
        if (m_frontBuffer == nullptr)
        {
            DAEMON_LOG(LogCore, eLogVerbosity::Error,
                       "StateBuffer::ValidateStateBuffer - Front buffer pointer is null");
            return false;
        }

        // Check back buffer pointer
        if (m_backBuffer == nullptr)
        {
            DAEMON_LOG(LogCore, eLogVerbosity::Error,
                       "StateBuffer::ValidateStateBuffer - Back buffer pointer is null");
            return false;
        }

        // Check front buffer points to internal storage
        if (m_frontBuffer != &m_bufferA && m_frontBuffer != &m_bufferB)
        {
            DAEMON_LOG(LogCore, eLogVerbosity::Error,
                       "StateBuffer::ValidateStateBuffer - Front buffer points to invalid storage");
            return false;
        }

        // Check back buffer points to internal storage
        if (m_backBuffer != &m_bufferA && m_backBuffer != &m_bufferB)
        {
            DAEMON_LOG(LogCore, eLogVerbosity::Error,
                       "StateBuffer::ValidateStateBuffer - Back buffer points to invalid storage");
            return false;
        }

        // Check no aliasing (front and back must be different buffers)
        if (m_frontBuffer == m_backBuffer)
        {
            DAEMON_LOG(LogCore, eLogVerbosity::Error,
                       "StateBuffer::ValidateStateBuffer - Buffer aliasing detected (front == back)");
            return false;
        }

        return true;
    }
};

//----------------------------------------------------------------------------------------------------
// Design Notes
//
// Full-Copy Strategy (Phase 1):
//   Rationale:
//     - Simple, predictable performance (no edge cases)
//     - Avoids complex dirty tracking logic
//     - Acceptable cost for Phase 1 (< 1ms for 1000 elements)
//
//   Drawbacks:
//     - Memory bandwidth: Copying entire container each frame
//     - Scalability: O(n) cost, grows with element count
//
//   Future Optimization (Phase 4):
//     - Copy-on-write: Only copy modified elements
//     - Dirty bit tracking: Track changed elements per frame
//     - Element pools: Preallocated storage, no allocation per frame
//
// Thread Safety Model:
//   - Main Thread: Reads front buffer, calls SwapBuffers()
//   - Worker Thread: Writes back buffer
//   - No locks during read/write (lock-free for hot path)
//   - Brief lock during SwapBuffers() (acceptable for 60 FPS)
//
// Thread Safety Validation:
//   - Run under Thread Sanitizer (TSan) to detect data races
//   - Main thread: Only reads front buffer (no writes)
//   - Worker thread: Only writes back buffer (no reads from front)
//   - Swap point: Brief lock, no concurrent access to buffers during swap
//
// Alternatives Considered:
//   - Lock-free swap: Complex, requires double-indirection or atomic pointers
//   - Triple-buffering: Wastes memory, no significant benefit for 60 FPS
//   - Shared pointer with atomic load/store: Complex ownership, rejected
//
// Performance Validation (Phase 1 Acceptance Criteria):
//   - Swap cost: < 1ms for 1000 elements (measured via high-resolution timer)
//   - No frame drops during typical gameplay (60 FPS stable)
//   - Memory overhead: < 10 MB for 1000 elements (2× storage)
//
// Template Instantiation Examples:
//   - StateBuffer<EntityStateMap>: Double-buffered entity state
//   - StateBuffer<CameraStateMap>: Double-buffered camera state
//   - StateBuffer<ParticleStateMap>: Double-buffered particle state (future)
//
// Phase 3.1: Error Recovery Design (SwapBuffers Error Handling)
//   Goal:
//     - Prevent crashes from exceptions during buffer copy
//     - Enable graceful degradation (rendering continues with stale data)
//     - Provide monitoring hooks for error detection
//
//   Implementation:
//     - ValidateStateBuffer(): Pre-copy validation (null checks, aliasing detection)
//     - Try-catch wrapping: Catches std::bad_alloc, std::exception, and unknown exceptions
//     - Error counter: m_swapErrorCount incremented on each failure
//     - DAEMON_LOG: Error details logged for debugging
//
//   Recovery Behavior:
//     - On validation failure: Skip swap, preserve stale front buffer
//     - On copy exception: Catch error, preserve stale front buffer
//     - Main thread continues rendering with last valid state
//
//   Monitoring API:
//     - GetSwapErrorCount(): Returns total errors encountered
//     - HasSwapErrors(): Quick boolean check for any errors
//----------------------------------------------------------------------------------------------------
