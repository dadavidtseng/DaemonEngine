//----------------------------------------------------------------------------------------------------
// KeyButtonState.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once

//----------------------------------------------------------------------------------------------------
/// @brief Tracks the current and previous frame state of a keyboard key or button input
///
/// @details This structure maintains state information for detecting key/button press events,
///          release events, and continuous hold states across frames. Essential for input
///          processing in game loops where "just pressed" and "just released" detection is needed.
///
/// @remark Used extensively by InputSystem for keyboard and controller button tracking.
/// @remark State transitions are managed per-frame: m_wasKeyDownLastFrame tracks previous state.
///
/// @warning Must be updated every frame for correct "just pressed/released" detection logic.
/// @see InputSystem::BeginFrame() for state update pattern
/// @see InputSystem::WasKeyJustPressed() for usage example
//----------------------------------------------------------------------------------------------------
struct sKeyButtonState
{
    bool m_isKeyDown           = false; ///< Current frame key/button state (true = pressed down)
    bool m_wasKeyDownLastFrame = false; ///< Previous frame key/button state for transition detection
};
