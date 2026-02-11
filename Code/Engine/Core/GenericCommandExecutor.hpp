//----------------------------------------------------------------------------------------------------
// GenericCommandExecutor.hpp
// GenericCommand System - Command Executor and Handler Registry
//
// Purpose:
//   Dispatches GenericCommands to registered handlers on the main render thread.
//   Manages handler registration (from JS worker thread) and command execution (main thread).
//   Delivers handler results back to JavaScript via CallbackQueue.
//
// Thread Safety Model:
//   - RegisterHandler / UnregisterHandler: Called from JS worker thread during initialization.
//     Protected by std::mutex since registration is infrequent (startup only).
//   - ExecuteCommand: Called from main render thread during ConsumeAll().
//     Lock-free read of handler map (safe because registration completes before game loop).
//   - ExecutePendingCallbacks: Called from main render thread.
//     Enqueues CallbackData to CallbackQueue for JS worker thread consumption.
//
// Callback Lifecycle:
//   1. JS submits GenericCommand with callbackId + callback (std::any)
//   2. GenericCommandScriptInterface stores callback in executor's pending map
//   3. Main thread ExecuteCommand() runs handler, stores HandlerResult
//   4. Main thread ExecutePendingCallbacks() enqueues CallbackData to CallbackQueue
//   5. JS worker thread dequeues and executes callback with result
//
// Author: GenericCommand System - Phase 2
// Date: 2026-02-10
//----------------------------------------------------------------------------------------------------

#pragma once

//----------------------------------------------------------------------------------------------------
#include "Engine/Core/GenericCommand.hpp"
#include "Engine/Core/HandlerResult.hpp"
#include "Engine/Core/StringUtils.hpp"
//----------------------------------------------------------------------------------------------------
#include <any>
#include <functional>
#include <mutex>
#include <unordered_map>
#include <vector>

//----------------------------------------------------------------------------------------------------
// Forward declaration for GetCurrentTimeSeconds()
//----------------------------------------------------------------------------------------------------
double GetCurrentTimeSeconds();

//----------------------------------------------------------------------------------------------------
class CallbackQueue;

//----------------------------------------------------------------------------------------------------
// Handler function signature:
//   Receives type-erased payload (std::any), returns structured result.
//   V8 conversion happens at ScriptInterface boundary, not here.
//----------------------------------------------------------------------------------------------------
using HandlerFunc = std::function<HandlerResult(std::any const&)>;

//----------------------------------------------------------------------------------------------------
// RateLimitState
//
// Per-agent rate limiting using token bucket algorithm.
// O(1) check time (<1µs), no allocations, no system calls.
//
// Token bucket refills at `maxCommandsPerSecond` tokens/sec.
// Each command consumes one token. If no tokens available, command is rejected.
//----------------------------------------------------------------------------------------------------
struct RateLimitState
{
    double   tokens          = 0.0;    // Available tokens (fractional for smooth refill)
    double   lastRefillTime  = 0.0;    // Last time tokens were refilled (wall-clock seconds)
    uint32_t maxTokens       = 100;    // Max tokens (= max commands per second)
    uint32_t rejectedCount   = 0;      // Total rejected commands for this agent

    //------------------------------------------------------------------------------------------------
    // TryConsume - Attempt to consume one token. Returns true if allowed, false if rate limited.
    // Refills tokens based on elapsed time since last check.
    //------------------------------------------------------------------------------------------------
    bool TryConsume(double currentTime)
    {
        // Refill tokens based on elapsed time
        double elapsed = currentTime - lastRefillTime;
        if (elapsed > 0.0)
        {
            tokens = std::min(static_cast<double>(maxTokens), tokens + elapsed * static_cast<double>(maxTokens));
            lastRefillTime = currentTime;
        }

        // Try to consume one token
        if (tokens >= 1.0)
        {
            tokens -= 1.0;
            return true;
        }

        ++rejectedCount;
        return false;
    }
};

//----------------------------------------------------------------------------------------------------
// AgentStatistics
//
// Per-agent command execution statistics for monitoring and debugging.
//----------------------------------------------------------------------------------------------------
struct AgentStatistics
{
    uint64_t submitted    = 0;  // Total commands submitted by this agent
    uint64_t executed     = 0;  // Successfully executed commands
    uint64_t failed       = 0;  // Commands that resulted in handler errors
    uint64_t rateLimited  = 0;  // Commands rejected by rate limiter
    uint64_t unhandled    = 0;  // Commands with no registered handler
};

//----------------------------------------------------------------------------------------------------
// CommandStatistics
//
// Aggregate snapshot of all executor statistics, returned by GetStatistics().
//----------------------------------------------------------------------------------------------------
struct CommandStatistics
{
    // Global totals
    uint64_t totalExecuted     = 0;
    uint64_t totalErrors       = 0;
    uint64_t totalUnhandled    = 0;
    uint64_t totalRateLimited  = 0;

    // Per-agent breakdown
    std::unordered_map<String, AgentStatistics> agentStats;

    // Per-type breakdown (command type → {executed, failed})
    struct TypeStats
    {
        uint64_t executed = 0;
        uint64_t failed   = 0;
    };
    std::unordered_map<String, TypeStats> typeStats;
};

//----------------------------------------------------------------------------------------------------
// GenericCommandExecutor
//
// Dispatches GenericCommands to registered handlers and manages async callback delivery.
//
// Usage:
//   // Registration (JS worker thread, during initialization)
//   executor.RegisterHandler("entity.create", [&entityAPI](std::any const& payload) {
//       auto params = std::any_cast<EntityCreateParams>(payload);
//       auto id = entityAPI.CreateEntity(params);
//       return HandlerResult::Success({{"entityId", std::any(id)}});
//   });
//
//   // Execution (main render thread, during ConsumeAll)
//   executor.ExecuteCommand(command);
//
//   // Callback delivery (main render thread, after command processing)
//   executor.ExecutePendingCallbacks(callbackQueue);
//----------------------------------------------------------------------------------------------------
class GenericCommandExecutor
{
public:
	//------------------------------------------------------------------------------------------------
	// Construction / Destruction
	//------------------------------------------------------------------------------------------------
	GenericCommandExecutor();
	~GenericCommandExecutor();

	// Non-copyable, non-movable
	GenericCommandExecutor(GenericCommandExecutor const&)            = delete;
	GenericCommandExecutor& operator=(GenericCommandExecutor const&) = delete;
	GenericCommandExecutor(GenericCommandExecutor&&)                 = delete;
	GenericCommandExecutor& operator=(GenericCommandExecutor&&)      = delete;

	//------------------------------------------------------------------------------------------------
	// Handler Registration (JS Worker Thread)
	//   Mutex-protected. Called during initialization before game loop starts.
	//------------------------------------------------------------------------------------------------

	// Register a handler for a command type. Returns false if type already registered.
	bool RegisterHandler(String const& commandType, HandlerFunc handler);

	// Remove a handler for a command type. Returns false if type not found.
	bool UnregisterHandler(String const& commandType);

	// Check if a handler is registered for the given command type.
	bool HasHandler(String const& commandType) const;

	// Get list of all registered command type strings.
	std::vector<String> GetRegisteredTypes() const;

	//------------------------------------------------------------------------------------------------
	// Command Execution (Main Render Thread)
	//   Lock-free. Called during GenericCommandQueue::ConsumeAll().
	//------------------------------------------------------------------------------------------------

	// Execute a GenericCommand by dispatching to its registered handler.
	// If the command has a callbackId, stores the result for callback delivery.
	// Unregistered command types are logged as warnings.
	void ExecuteCommand(GenericCommand const& command);

	//------------------------------------------------------------------------------------------------
	// Callback Delivery (Main Render Thread)
	//   Enqueues completed callback results to CallbackQueue for JS worker thread.
	//   Follows EntityAPI::ExecutePendingCallbacks pattern.
	//------------------------------------------------------------------------------------------------

	// Enqueue all pending callback results to the CallbackQueue.
	void ExecutePendingCallbacks(CallbackQueue* callbackQueue);

	//------------------------------------------------------------------------------------------------
	// Pending Callback Storage
	//   Called by GenericCommandScriptInterface to store JS callback before submission.
	//------------------------------------------------------------------------------------------------

	// Store a callback (std::any wrapping v8::Global<v8::Function>*) for later delivery.
	void StoreCallback(uint64_t callbackId, std::any callback);

	// Retrieve and remove a stored callback. Returns empty std::any if not found.
	std::any RetrieveCallback(uint64_t callbackId);

	//------------------------------------------------------------------------------------------------
	// Statistics
	//------------------------------------------------------------------------------------------------
	uint64_t GetTotalExecuted() const { return m_totalExecuted; }
	uint64_t GetTotalErrors() const { return m_totalErrors; }
	uint64_t GetTotalUnhandled() const { return m_totalUnhandled; }
	uint64_t GetTotalRateLimited() const { return m_totalRateLimited; }

	// Get a full statistics snapshot (per-agent and per-type breakdowns).
	CommandStatistics GetStatistics() const;

	//------------------------------------------------------------------------------------------------
	// Audit Logging
	//   When enabled, logs every command execution with timestamp, agentId, type, success/failure.
	//   Disabled by default to avoid log spam. Enable for debugging or monitoring.
	//------------------------------------------------------------------------------------------------
	void SetAuditLoggingEnabled(bool enabled);
	bool IsAuditLoggingEnabled() const { return m_auditLoggingEnabled; }

	//------------------------------------------------------------------------------------------------
	// Rate Limiting Configuration
	//   Token bucket algorithm: O(1) per check, <1µs overhead.
	//   Default: 100 commands/sec per agent. Set to 0 to disable rate limiting.
	//------------------------------------------------------------------------------------------------

	// Set the maximum commands per second allowed per agent. 0 = unlimited.
	void SetRateLimitPerAgent(uint32_t maxCommandsPerSecond);

	// Get the current rate limit setting.
	uint32_t GetRateLimitPerAgent() const { return m_rateLimitPerAgent; }

	// Get rate limit state for a specific agent (for diagnostics).
	// Returns nullptr if agent has no state yet.
	RateLimitState const* GetAgentRateLimitState(String const& agentId) const;

private:
	//------------------------------------------------------------------------------------------------
	// Internal Types
	//------------------------------------------------------------------------------------------------

	// Pending callback result waiting to be enqueued to CallbackQueue
	struct PendingResult
	{
		uint64_t      callbackId;
		HandlerResult result;
		bool          ready;   // True when handler has executed and result is available
	};

	//------------------------------------------------------------------------------------------------
	// Data Members
	//------------------------------------------------------------------------------------------------

	// Handler registry: command type → handler function
	std::unordered_map<String, HandlerFunc> m_handlers;
	mutable std::mutex                      m_handlerMutex;

	// Pending callback storage: callbackId → JS callback (std::any)
	std::unordered_map<uint64_t, std::any>  m_storedCallbacks;

	// Pending results: callbackId → result from handler execution
	std::unordered_map<uint64_t, PendingResult> m_pendingResults;

	// Statistics
	uint64_t m_totalExecuted  = 0;
	uint64_t m_totalErrors    = 0;
	uint64_t m_totalUnhandled = 0;
	uint64_t m_totalRateLimited = 0;

	// Rate limiting: agentId → token bucket state
	std::unordered_map<String, RateLimitState> m_agentRateLimits;
	uint32_t m_rateLimitPerAgent = 100; // Default: 100 commands/sec per agent (0 = disabled)

	// Per-agent statistics: agentId → AgentStatistics
	std::unordered_map<String, AgentStatistics> m_agentStats;

	// Per-type statistics: command type → {executed, failed}
	std::unordered_map<String, CommandStatistics::TypeStats> m_typeStats;

	// Audit logging toggle (disabled by default)
	bool m_auditLoggingEnabled = false;
};
