//----------------------------------------------------------------------------------------------------
// InputSystem.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Input/InputCommon.hpp"
#include "Engine/Input/XboxController.hpp"
#include <chrono>
#include <vector>
#include "ThirdParty/json/json.hpp"

#include "Engine/Resource/MaterialResource.hpp"


//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
// Phase 6a: Deferred key release for InjectKeyHold
struct sDeferredKeyRelease
{
    unsigned char keyCode;
    WORD scanCode;
    std::chrono::steady_clock::time_point releaseTime;
};


//----------------------------------------------------------------------------------------------------
// Phase 6a: Generic Tool Job System for Async Operations
enum class eToolJobStatus
{
    Active,
    Completed,
    Cancelled,
    NotFound
};

struct sToolJobStatus
{
    uint32_t jobId;
    std::string toolType;
    eToolJobStatus status;
    std::map<std::string, std::string> metadata;

    // Convert to JSON string for JavaScript integration
    std::string ToJson() const
    {
        nlohmann::json j;
        j["jobId"] = jobId;
        j["toolType"] = toolType;
        j["status"] = StatusToString();

        nlohmann::json metadataJson;
        for (const auto& pair : metadata)
        {
            metadataJson[pair.first] = pair.second;
        }
        j["metadata"] = metadataJson;

        return j.dump();
    }

private:
    std::string StatusToString() const
    {
        switch (status)
        {
            case eToolJobStatus::Active: return "active";
            case eToolJobStatus::Completed: return "completed";
            case eToolJobStatus::Cancelled: return "cancelled";
            case eToolJobStatus::NotFound: return "not_found";
            default: return "unknown";
        }
    }
};

// Enhanced key hold job for tracking
struct sKeyHoldJob
{
    uint32_t jobId;
    unsigned char keyCode;
    WORD scanCode;
    std::chrono::steady_clock::time_point startTime;
    std::chrono::steady_clock::time_point releaseTime;
    std::chrono::steady_clock::time_point completedTime;
    int totalDurationMs;
    bool isCancelled;
    bool hasPressed;  // Track if KEY_DOWN has been sent yet
};

//----------------------------------------------------------------------------------------------------
// Phase 6a: Multi-key sequence support for advanced input injection
struct sKeySequenceItem
{
    unsigned char keyCode;
    WORD scanCode;
    int delayMs;           // Delay before pressing this key (relative to sequence start)
    int durationMs;        // Duration to hold this key
};

struct sInputSystemConfig
{
};

//----------------------------------------------------------------------------------------------------
class InputSystem
{
public:
    explicit InputSystem(sInputSystemConfig const& config);
    ~InputSystem() = default;

    void Startup();
    void Shutdown();
    void BeginFrame();
    void EndFrame();

    bool WasKeyJustPressed(unsigned char keyCode) const;
    bool WasKeyJustReleased(unsigned char keyCode) const;
    bool IsKeyDown(unsigned char keyCode) const;

    void HandleKeyPressed(unsigned char keyCode);
    void HandleKeyReleased(unsigned char keyCode);

    // Phase 6a: KADI Development Tools - Input Injection
    void InjectKeyPress(unsigned char keyCode, int durationMs = 50);
    uint32_t InjectKeyHold(unsigned char keyCode, int durationMs, bool repeat = false);
    uint32_t InjectKeySequence(std::vector<sKeySequenceItem> keySequence);

    XboxController const& GetController(int controllerID);
    Vec2                  GetCursorClientDelta() const;
    Vec2                  GetCursorClientPosition() const;
    Vec2                  GetCursorNormalizedPosition() const;

    void SetCursorMode(eCursorMode mode);

    static bool OnWindowKeyPressed(EventArgs& args);
    static bool OnWindowKeyReleased(EventArgs& args);
    // Phase 6a: Job tracking and status queries
    sToolJobStatus GetKeyHoldStatus(uint32_t jobId) const;
    bool CancelKeyHold(uint32_t jobId);
    std::vector<sToolJobStatus> ListActiveKeyHolds() const;

protected:
    sKeyButtonState m_keyStates[NUM_KEYCODES];
    XboxController  m_controllers[NUM_XBOX_CONTROLLERS];
    sCursorState    m_cursorState;

private:
    sInputSystemConfig m_config;
    // Phase 6a: Job tracking system
    std::map<uint32_t, sKeyHoldJob> m_activeKeyHolds;
    std::map<uint32_t, std::chrono::steady_clock::time_point> m_completedJobs;
    uint32_t m_nextJobId;
};

