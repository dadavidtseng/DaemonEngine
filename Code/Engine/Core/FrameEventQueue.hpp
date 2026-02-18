//----------------------------------------------------------------------------------------------------
// FrameEventQueue.hpp
// Unified C++ → JavaScript Frame Event Channel
//
// Purpose:
//   Lock-free SPSC queue carrying all high-frequency data from C++ main thread to JavaScript
//   worker thread. Replaces the race-prone synchronous InputScriptInterface reads with an
//   event-driven model where C++ pushes state changes and JS maintains local state.
//
// Event Types:
//   - KeyDown / KeyUp: Discrete keyboard events (enqueued on WM_KEYDOWN / WM_KEYUP)
//   - MouseButtonDown / MouseButtonUp: Discrete mouse button events
//   - CursorUpdate: Per-frame cursor position and delta (enqueued in InputSystem::BeginFrame)
//
// Thread Safety:
//   - Producer: Main thread (InputSystem::HandleKeyPressed/Released, BeginFrame)
//   - Consumer: Worker thread (JavaScript drains via FrameEventQueueScriptInterface)
//   - Inherits lock-free SPSC guarantees from CommandQueueBase<T>
//
// Author: FrameEventQueue - Input Refactoring
// Date: 2026-02-17
//----------------------------------------------------------------------------------------------------

#pragma once

//----------------------------------------------------------------------------------------------------
#include "Engine/Core/CommandQueueBase.hpp"
//----------------------------------------------------------------------------------------------------
#include <cstdint>

//----------------------------------------------------------------------------------------------------
// eFrameEventType
//
// Tagged union discriminator for FrameEvent.
// Each value corresponds to one variant of the FrameEvent union.
//----------------------------------------------------------------------------------------------------
enum class eFrameEventType : uint8_t
{
    KeyDown,          // Keyboard key pressed
    KeyUp,            // Keyboard key released
    MouseButtonDown,  // Mouse button pressed (reuses keyCode field)
    MouseButtonUp,    // Mouse button released (reuses keyCode field)
    CursorUpdate      // Per-frame cursor position and delta
};

//----------------------------------------------------------------------------------------------------
// FrameEvent
//
// Lightweight tagged union for C++ → JS event delivery.
// Designed for minimal memory footprint in the SPSC ring buffer.
//
// Size: ~20 bytes per event (type + largest union member)
// Typical frame: 1 CursorUpdate + 0-5 key events = ~120 bytes/frame
//----------------------------------------------------------------------------------------------------
struct FrameEvent
{
    eFrameEventType type;

    union
    {
        // KeyDown, KeyUp, MouseButtonDown, MouseButtonUp
        struct
        {
            uint8_t keyCode;
        } key;

        // CursorUpdate (per-frame cursor state)
        struct
        {
            float x;   // Client position X
            float y;   // Client position Y
            float dx;  // Delta X since last frame
            float dy;  // Delta Y since last frame
        } cursor;
    };

    // Default constructor (required for CommandQueueBase ring buffer)
    FrameEvent()
        : type(eFrameEventType::KeyDown)
    {
        key.keyCode = 0;
    }
};

//----------------------------------------------------------------------------------------------------
// FrameEventQueue
//
// SPSC ring buffer specialized for FrameEvent delivery.
// Capacity: 256 events (generous for typical input rates).
//
// Usage:
//   Producer (main thread):
//     FrameEvent evt;
//     evt.type = eFrameEventType::KeyDown;
//     evt.key.keyCode = keyCode;
//     queue->Submit(evt);
//
//   Consumer (worker thread via ScriptInterface):
//     queue->ConsumeAll([&](FrameEvent const& evt) { ... });
//----------------------------------------------------------------------------------------------------
class FrameEventQueue : public CommandQueueBase<FrameEvent>
{
public:
    static constexpr size_t FRAME_EVENT_CAPACITY = 256;

    FrameEventQueue()
        : CommandQueueBase<FrameEvent>(FRAME_EVENT_CAPACITY)
    {
    }

    ~FrameEventQueue() override = default;
};
