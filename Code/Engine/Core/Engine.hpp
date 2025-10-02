//----------------------------------------------------------------------------------------------------
// GEngine.hpp
// Global engine singleton providing centralized access to core engine subsystems
//----------------------------------------------------------------------------------------------------

#pragma once

//----------------------------------------------------------------------------------------------------
// Forward Declarations
//----------------------------------------------------------------------------------------------------
class JobSystem;

//----------------------------------------------------------------------------------------------------
/// @brief Global Engine singleton providing centralized access to engine subsystems
///
/// @details GEngine serves as the central access point for core engine systems, replacing
///          scattered global pointers with a clean singleton pattern. This improves testability,
///          reduces global namespace pollution, and provides clear ownership semantics.
///
/// @remark Access via GEngine::Get() singleton instance
/// @remark Subsystems must be initialized before use (call Initialize() during engine startup)
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

    //----------------------------------------------------------------------------------------------------
    /// @brief Initialize the engine singleton with subsystem pointers
    /// @param jobSystem Pointer to the global JobSystem instance
    /// @remark Called during App::Startup() to wire up subsystems
    //----------------------------------------------------------------------------------------------------
    void Initialize(JobSystem* jobSystem);

    //----------------------------------------------------------------------------------------------------
    /// @brief Shutdown and cleanup the engine singleton
    /// @remark Called during App::Shutdown() to clear subsystem references
    //----------------------------------------------------------------------------------------------------
    void Shutdown();

    //----------------------------------------------------------------------------------------------------
    // Subsystem Accessors (Static Methods)
    //----------------------------------------------------------------------------------------------------

    /// @brief Get the global JobSystem instance (static accessor)
    /// @return Pointer to JobSystem (may be nullptr if not initialized)
    /// @remark Thread-safe to call, but subsystem operations may not be thread-safe
    static JobSystem* GetJobSystem() { return Get().m_jobSystem; }

private:
    // Private constructor for singleton pattern
    GEngine() = default;
    ~GEngine() = default;

    // Prevent copying and assignment
    GEngine(GEngine const&) = delete;
    GEngine& operator=(GEngine const&) = delete;
    GEngine(GEngine&&) = delete;
    GEngine& operator=(GEngine&&) = delete;

    //----------------------------------------------------------------------------------------------------
    // Member Variables
    //----------------------------------------------------------------------------------------------------
    JobSystem* m_jobSystem = nullptr;
};
