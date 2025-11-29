//----------------------------------------------------------------------------------------------------
// CameraStateBuffer.cpp
// Engine Renderer Module - Double-Buffered Camera State Implementation
//
// Author: M4-T8 Engine Refactoring (moved from Game)
// Original: Phase 2b - Camera API Implementation
// Date: 2025-10-26
//----------------------------------------------------------------------------------------------------

#include "Engine/Renderer/CameraStateBuffer.hpp"

#include "Engine/Core/LogSubsystem.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Math/Mat44.hpp"

//----------------------------------------------------------------------------------------------------
// Construction / Destruction
//----------------------------------------------------------------------------------------------------

CameraStateBuffer::CameraStateBuffer()
    : m_frontBuffer(&m_bufferA)
      , m_backBuffer(&m_bufferB)
      , m_activeCameraID(0)  // No active camera initially
      , m_totalSwaps(0)
{
    // Double-buffered storage initialized
    // Front buffer starts empty, back buffer ready for writes

    // DIAGNOSTIC: Verify buffer initialization
    DAEMON_LOG(LogScript, eLogVerbosity::Display,
               StringFormat("[DIAGNOSTIC] CameraStateBuffer::Constructor - this={}", static_cast<void*>(this)));
    DAEMON_LOG(LogScript, eLogVerbosity::Display,
               StringFormat("[DIAGNOSTIC] CameraStateBuffer::Constructor - m_bufferA address={}, size={}",
                   static_cast<void*>(&m_bufferA), m_bufferA.size()));
    DAEMON_LOG(LogScript, eLogVerbosity::Display,
               StringFormat("[DIAGNOSTIC] CameraStateBuffer::Constructor - m_bufferB address={}, size={}",
                   static_cast<void*>(&m_bufferB), m_bufferB.size()));
    DAEMON_LOG(LogScript, eLogVerbosity::Display,
               StringFormat("[DIAGNOSTIC] CameraStateBuffer::Constructor - m_frontBuffer={} (points to m_bufferA={})",
                   static_cast<void*>(m_frontBuffer), static_cast<void*>(&m_bufferA)));
    DAEMON_LOG(LogScript, eLogVerbosity::Display,
               StringFormat("[DIAGNOSTIC] CameraStateBuffer::Constructor - m_backBuffer={} (points to m_bufferB={})",
                   static_cast<void*>(m_backBuffer), static_cast<void*>(&m_bufferB)));
    DAEMON_LOG(LogScript, eLogVerbosity::Display,
               StringFormat("[DIAGNOSTIC] CameraStateBuffer::Constructor - frontBuffer->size()={}, backBuffer->size()={}",
                   m_frontBuffer->size(), m_backBuffer->size()));
}

//----------------------------------------------------------------------------------------------------
CameraStateBuffer::~CameraStateBuffer()
{
    // std::unordered_map cleanup handled automatically
    // No manual resource deallocation needed
}

//----------------------------------------------------------------------------------------------------
// Buffer Access (Thread-Safe)
//----------------------------------------------------------------------------------------------------

CameraStateMap const* CameraStateBuffer::GetFrontBuffer() const
{
    // Lock-free read from main thread
    // Returns const pointer to prevent accidental modification
    return m_frontBuffer;
}

//----------------------------------------------------------------------------------------------------
CameraStateMap* CameraStateBuffer::GetBackBuffer()
{
    // Phase 4.1: Mark buffer as dirty when worker requests write access
    m_isDirty.store(true, std::memory_order_release);

    // Lock-free write from worker thread
    // Returns mutable pointer for single-writer pattern
    return m_backBuffer;
}

//----------------------------------------------------------------------------------------------------
// Buffer Swap (Frame Boundary, Main Thread Only)
//----------------------------------------------------------------------------------------------------

void CameraStateBuffer::SwapBuffers()
{
    // Phase 4.1: Check dirty flag before acquiring lock (optimization)
    // If buffer hasn't been modified, skip the expensive copy operation
    if (!m_isDirty.load(std::memory_order_acquire))
    {
        ++m_skippedSwaps;
        return;
    }

    // Lock during swap to prevent concurrent access
    std::lock_guard lock(m_swapMutex);

    // DIAGNOSTIC: Log buffer contents before swap (every 60 swaps ~1 second)
    if (m_totalSwaps % 60 == 0)
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Display,
                   StringFormat("[DIAGNOSTIC] SwapBuffers BEFORE: frontBuffer has {} cameras, backBuffer has {} cameras, activeCameraId={}",
                       m_frontBuffer->size(), m_backBuffer->size(), m_activeCameraID));

        // Log back buffer contents (what we're about to swap in)
        for (auto const& [cameraId, cameraState] : *m_backBuffer)
        {
            DAEMON_LOG(LogScript, eLogVerbosity::Display,
                       StringFormat("[DIAGNOSTIC] SwapBuffers - BackBuffer Camera {}: pos=({:.2f}, {:.2f}, {:.2f}), orient=(yaw={:.2f}, pitch={:.2f}, roll={:.2f})",
                           cameraId, cameraState.position.x, cameraState.position.y, cameraState.position.z,
                           cameraState.orientation.m_yawDegrees, cameraState.orientation.m_pitchDegrees, cameraState.orientation.m_rollDegrees));
        }
    }

    // Full copy: back buffer → new front buffer
    // Phase 2b: Simple strategy, optimize in Phase 4 if profiling shows need
    *m_frontBuffer = *m_backBuffer;

    // Swap pointers (cheap, no data copy)
    std::swap(m_frontBuffer, m_backBuffer);

    // Rebuild camera cache from new front buffer
    // Convert CameraState structs to Camera objects for rendering
    m_cameraCache.clear();
    for (auto const& [cameraId, cameraState] : *m_frontBuffer)
    {
        // Create Camera object from CameraState
        Camera camera;

        // Set camera position and orientation
        camera.SetPosition(cameraState.position);
        camera.SetOrientation(cameraState.orientation);

        // Configure camera mode (perspective vs orthographic)
        if (cameraState.type == "world")
        {
            // Perspective camera for 3D world rendering
            camera.SetPerspectiveGraphicView(
                cameraState.perspectiveAspect,
                cameraState.perspectiveFOV,
                cameraState.perspectiveNear,
                cameraState.perspectiveFar
            );
        }
        else if (cameraState.type == "screen")
        {
            // Orthographic camera for 2D UI/HUD rendering
            camera.SetOrthoGraphicView(
                Vec2(cameraState.orthoLeft, cameraState.orthoBottom),
                Vec2(cameraState.orthoRight, cameraState.orthoTop),
                cameraState.orthoNear,
                cameraState.orthoFar
            );
        }

        // Set viewport (normalized coordinates)
        // Phase 2b: Fix for rendering issue - viewport must be set for proper rendering
        camera.SetNormalizedViewport(cameraState.viewport);

        // Set camera-to-render transform based on camera type
        if (cameraState.type == "world")
        {
            // 3D world cameras: Apply coordinate system transform
            // Converts from game coordinates (X-forward, Y-left, Z-up) to render coordinates
            Mat44 cameraToRender;
            cameraToRender.m_values[Mat44::Ix] = 0.0f;
            cameraToRender.m_values[Mat44::Iy] = 0.0f;
            cameraToRender.m_values[Mat44::Iz] = 1.0f;
            cameraToRender.m_values[Mat44::Iw] = 0.0f;
            cameraToRender.m_values[Mat44::Jx] = -1.0f;
            cameraToRender.m_values[Mat44::Jy] = 0.0f;
            cameraToRender.m_values[Mat44::Jz] = 0.0f;
            cameraToRender.m_values[Mat44::Jw] = 0.0f;
            cameraToRender.m_values[Mat44::Kx] = 0.0f;
            cameraToRender.m_values[Mat44::Ky] = 1.0f;
            cameraToRender.m_values[Mat44::Kz] = 0.0f;
            cameraToRender.m_values[Mat44::Kw] = 0.0f;
            cameraToRender.m_values[Mat44::Tx] = 0.0f;
            cameraToRender.m_values[Mat44::Ty] = 0.0f;
            cameraToRender.m_values[Mat44::Tz] = 0.0f;
            cameraToRender.m_values[Mat44::Tw] = 1.0f;
            camera.SetCameraToRenderTransform(cameraToRender);
        }
        else if (cameraState.type == "screen")
        {
            // 2D screen cameras: Use identity transform (no rotation)
            // Screen coordinates are already in correct screen space (X-right, Y-up)
            // camera.SetCameraToRenderTransform(Mat44::IDENTITY);
        }

        // Store in cache
        m_cameraCache[cameraId] = camera;
    }

    // Increment swap counter for profiling
    ++m_totalSwaps;

    // Phase 4.1: Reset dirty flag after successful copy
    m_isDirty.store(false, std::memory_order_release);
}

//----------------------------------------------------------------------------------------------------
// Camera Lookup (Main Thread, for rendering)
//----------------------------------------------------------------------------------------------------

Camera const* CameraStateBuffer::GetCameraById(EntityID cameraId) const
{
    // Look up camera in cache (lock-free, main thread only)
    // Cache is updated during SwapBuffers(), so it matches front buffer
    auto it = m_cameraCache.find(cameraId);
    if (it != m_cameraCache.end())
    {
        return &it->second;  // Return pointer to cached Camera object
    }

    // Camera not found
    return nullptr;
}

//----------------------------------------------------------------------------------------------------
// Monitoring / Debugging
//----------------------------------------------------------------------------------------------------

size_t CameraStateBuffer::GetCameraCount() const
{
    // Lock-free read (approximate count, may be stale)
    // Used for monitoring only, not critical for correctness
    return m_frontBuffer->size();
}
