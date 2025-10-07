//----------------------------------------------------------------------------------------------------
// GEngine.hpp
// Global engine singleton providing centralized access to core engine subsystems
//----------------------------------------------------------------------------------------------------

#pragma once
#include "EngineCommon.hpp"

class Camera;

//----------------------------------------------------------------------------------------------------
/// @brief Global Engine singleton providing centralized access to engine subsystems
///
/// @details GEngine serves as the central access point for core engine systems, replacing
///          scattered global pointers with a clean singleton pattern. This improves testability,
///          reduces global namespace pollution, and provides clear ownership semantics.
///
/// @remark Access via GEngine::Get() singleton instance
/// @remark Subsystems must be initialized before use (call Initialize() during engine startup)
/// @remark Some subsystems are optional (AudioSystem, InputSystem) and may be nullptr
///
/// @see App::Startup() for initialization sequence
//----------------------------------------------------------------------------------------------------
class GEngine
{
public:
    //----------------------------------------------------------------------------------------------------
    /// @brief Get the global engine singleton instance
    /// @return Reference to the GEngine singleton
    //----------------------------------------------------------------------------------------------------
    static GEngine& Get();

    void Construct();
    void Destruct();

    //----------------------------------------------------------------------------------------------------
    /// @brief Initialize the engine singleton with subsystem pointers
    /// @remark Called during App::Startup() to wire up subsystems
    //----------------------------------------------------------------------------------------------------
    void Startup();

    //----------------------------------------------------------------------------------------------------
    /// @brief Shutdown and cleanup the engine singleton
    /// @remark Called during App::Shutdown() to clear subsystem references
    //----------------------------------------------------------------------------------------------------
    void Shutdown();

    // Prevent copying and assignment
    GEngine(GEngine const&)            = delete;
    GEngine& operator=(GEngine const&) = delete;
    GEngine(GEngine&&)                 = delete;
    GEngine& operator=(GEngine&&)      = delete;

private:
    // Private constructor for singleton pattern
    GEngine()  = default;
    ~GEngine() = default;
};

