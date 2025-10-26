//----------------------------------------------------------------------------------------------------
// CameraStateBuffer.hpp
// Engine Renderer Module - Double-Buffered Camera State Container
//
// Purpose:
//   Thread-safe camera state storage for rendering isolation.
//   Allows worker thread to create/update cameras while main thread renders.
//
// Design Rationale:
//   - Double-buffering: Main thread reads front buffer, worker writes back buffer
//   - Active camera tracking: Supports switching between multiple cameras
//   - Camera object cache: Converts CameraState to Camera for rendering
//   - Coordinate system transform: Handles game-to-render coordinate conversion
//
// Thread Safety Model:
//   - Main Thread: Reads front buffer (no locking)
//   - Worker Thread: Writes back buffer (no locking)
//   - Swap Point: Atomic pointer swap at frame boundary (lock-protected)
//
// Author: M4-T8 Engine Refactoring (moved from Game)
// Original: Phase 2b - Camera API Implementation
// Date: 2025-10-26
//----------------------------------------------------------------------------------------------------

#pragma once

//----------------------------------------------------------------------------------------------------
#include "Engine/Renderer/CameraState.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Entity/EntityID.hpp"
#include <mutex>
#include <unordered_map>

//----------------------------------------------------------------------------------------------------
// CameraStateBuffer Class
//
// Double-buffered camera state storage for lock-free rendering.
// Extends basic StateBuffer pattern with camera-specific functionality.
//
// Additional Features Beyond StateBuffer<T>:
//   - Active camera tracking (GetActiveCameraID/SetActiveCameraID)
//   - Camera object cache (CameraState → Camera conversion)
//   - Coordinate system transform (game coordinates → render coordinates)
//   - Camera lookup by ID (GetCameraById)
//
// Usage Pattern:
//
//   Worker Thread (JavaScript):
//     CameraStateMap* backBuffer = buffer->GetBackBuffer();
//     (*backBuffer)[cameraId].position = newPosition;  // Update back buffer
//
//   Main Thread (Rendering):
//     CameraStateMap const* frontBuffer = buffer->GetFrontBuffer();
//     EntityID activeCameraId = buffer->GetActiveCameraID();
//     Camera const* camera = buffer->GetCameraById(activeCameraId);
//     if (camera) {
//         renderer->SetCamera(*camera);  // Use cached Camera object
//     }
//
//   Frame Boundary (Main Thread):
//     buffer->SwapBuffers();  // Swap buffers + rebuild camera cache
//
// Thread Safety:
//   - GetFrontBuffer(): Lock-free read (main thread)
//   - GetBackBuffer(): Lock-free write (worker thread)
//   - SwapBuffers(): Brief locked operation (main thread only)
//   - GetCameraById(): Lock-free read from cache (main thread)
//----------------------------------------------------------------------------------------------------
class CameraStateBuffer
{
public:
	//------------------------------------------------------------------------------------------------
	// Construction / Destruction
	//------------------------------------------------------------------------------------------------
	CameraStateBuffer();
	~CameraStateBuffer();

	// Non-copyable, non-movable (contains mutex)
	CameraStateBuffer(CameraStateBuffer const&)            = delete;
	CameraStateBuffer& operator=(CameraStateBuffer const&) = delete;
	CameraStateBuffer(CameraStateBuffer&&)                 = delete;
	CameraStateBuffer& operator=(CameraStateBuffer&&)      = delete;

	//------------------------------------------------------------------------------------------------
	// Buffer Access (Thread-Safe)
	//------------------------------------------------------------------------------------------------

	// Get front buffer for rendering (Main Thread, lock-free read)
	// Returns: Const pointer to current front buffer (safe for concurrent reads)
	CameraStateMap const* GetFrontBuffer() const;

	// Get back buffer for writing (Worker Thread, lock-free write)
	// Returns: Mutable pointer to current back buffer (single-writer guarantee)
	CameraStateMap* GetBackBuffer();

	//------------------------------------------------------------------------------------------------
	// Active Camera Management
	//------------------------------------------------------------------------------------------------

	// Get active camera ID (Main Thread, for rendering)
	// Returns: EntityID of currently active camera (0 = no active camera)
	EntityID GetActiveCameraID() const { return m_activeCameraID; }

	// Set active camera ID (Worker Thread, via command processing)
	// Parameters: cameraId - ID of camera to make active
	void SetActiveCameraID(EntityID cameraId) { m_activeCameraID = cameraId; }

	//------------------------------------------------------------------------------------------------
	// Buffer Swap (Frame Boundary, Main Thread Only)
	//------------------------------------------------------------------------------------------------

	// Swap front/back buffers and rebuild camera cache (locked operation)
	//
	// Algorithm:
	//   1. Acquire mutex lock
	//   2. Copy back buffer → new front buffer (full deep copy)
	//   3. Swap buffer pointers
	//   4. Rebuild camera cache (CameraState → Camera conversion)
	//   5. Release mutex lock
	//
	// Performance: O(n) where n = number of cameras
	// Thread Safety: Locked operation, call from main thread only
	void SwapBuffers();

	//------------------------------------------------------------------------------------------------
	// Camera Lookup (Main Thread, for rendering)
	//------------------------------------------------------------------------------------------------

	// Get camera by ID from cache (Main Thread, for rendering)
	//
	// Returns:
	//   - Pointer to cached Camera object if found
	//   - nullptr if camera not found or not in cache
	//
	// Thread Safety: Lock-free, main thread only
	// Cache Validity: Valid until next SwapBuffers() call
	Camera const* GetCameraById(EntityID cameraId) const;

	//------------------------------------------------------------------------------------------------
	// Monitoring / Debugging
	//------------------------------------------------------------------------------------------------

	// Get camera count in front buffer (approximate, for monitoring only)
	// Thread Safety: Lock-free read, may race with SwapBuffers()
	size_t GetCameraCount() const;

	// Get total swaps performed (for profiling)
	// Thread Safety: Lock-free read, may race with SwapBuffers()
	uint64_t GetTotalSwaps() const { return m_totalSwaps; }

private:
	//------------------------------------------------------------------------------------------------
	// Double-Buffer Storage
	//------------------------------------------------------------------------------------------------
	CameraStateMap m_bufferA;  // Buffer A (front or back)
	CameraStateMap m_bufferB;  // Buffer B (front or back)

	CameraStateMap* m_frontBuffer;  // Pointer to current front buffer (read by main thread)
	CameraStateMap* m_backBuffer;   // Pointer to current back buffer (written by worker thread)

	//------------------------------------------------------------------------------------------------
	// Active Camera Tracking
	//------------------------------------------------------------------------------------------------
	EntityID m_activeCameraID;  // Currently active camera for rendering (0 = no active camera)

	//------------------------------------------------------------------------------------------------
	// Camera Object Cache (for rendering)
	//------------------------------------------------------------------------------------------------
	// Cache of Camera objects converted from CameraState for rendering
	// Updated during SwapBuffers() to match front buffer
	// Enables efficient Camera object retrieval without repeated conversion
	mutable std::unordered_map<EntityID, Camera> m_cameraCache;

	//------------------------------------------------------------------------------------------------
	// Synchronization
	//------------------------------------------------------------------------------------------------
	mutable std::mutex m_swapMutex;  // Protects buffer swap operation

	//------------------------------------------------------------------------------------------------
	// Statistics
	//------------------------------------------------------------------------------------------------
	uint64_t m_totalSwaps;  // Total buffer swaps performed (profiling counter)
};

//----------------------------------------------------------------------------------------------------
// Design Notes
//
// Why Not Use StateBuffer<CameraStateMap>?
//   - Camera requires additional functionality beyond generic StateBuffer:
//     1. Active camera tracking (m_activeCameraID)
//     2. Camera object cache (CameraState → Camera conversion)
//     3. Coordinate system transform (game → render coordinates)
//   - These features are camera-specific and not needed for generic state buffering
//
// Camera Object Cache Design:
//   - Converts CameraState (POD struct) to Camera (full object) during SwapBuffers()
//   - Avoids repeated conversion during rendering (performance optimization)
//   - Cache rebuilt every frame to match front buffer contents
//   - Memory overhead: O(n) where n = number of cameras
//
// Coordinate System Transform:
//   - Game coordinates: X-forward, Y-left, Z-up (right-handed)
//   - Render coordinates: Implementation-specific transformation
//   - Applied during camera cache rebuild in SwapBuffers()
//   - Separate handling for world (3D) vs screen (2D) cameras
//
// Active Camera Management:
//   - Single active camera ID tracked for rendering
//   - Worker thread can change active camera via SetActiveCameraID()
//   - Main thread reads active camera via GetActiveCameraID()
//   - No locking needed (atomic EntityID read/write on modern platforms)
//
// Future Optimizations (Phase 4):
//   - Dirty tracking: Only rebuild changed cameras in cache
//   - Multi-camera rendering: Support multiple simultaneous active cameras
//   - Camera pools: Preallocated storage to avoid allocation per frame
//----------------------------------------------------------------------------------------------------
