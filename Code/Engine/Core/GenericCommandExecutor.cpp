//----------------------------------------------------------------------------------------------------
// GenericCommandExecutor.cpp
// GenericCommand System - Command Executor Implementation
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Core/GenericCommandExecutor.hpp"

#include "Engine/Core/CallbackData.hpp"
#include "Engine/Core/CallbackQueue.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/LogSubsystem.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/Time.hpp"

//----------------------------------------------------------------------------------------------------
// Construction / Destruction
//----------------------------------------------------------------------------------------------------

GenericCommandExecutor::GenericCommandExecutor()
{
	DAEMON_LOG(LogCore, eLogVerbosity::Log, "GenericCommandExecutor: Initialized");
}

//----------------------------------------------------------------------------------------------------
GenericCommandExecutor::~GenericCommandExecutor()
{
	// Log final statistics
	DAEMON_LOG(LogCore, eLogVerbosity::Log,
	           Stringf("GenericCommandExecutor: Shutdown - Executed: %llu, Errors: %llu, Unhandled: %llu, RateLimited: %llu, Pending callbacks: %zu",
	               m_totalExecuted,
	               m_totalErrors,
	               m_totalUnhandled,
	               m_totalRateLimited,
	               m_storedCallbacks.size()));

	// Warn about leaked callbacks
	if (!m_storedCallbacks.empty())
	{
		DAEMON_LOG(LogCore, eLogVerbosity::Warning,
		           Stringf("GenericCommandExecutor: %zu stored callbacks not delivered at shutdown",
		               m_storedCallbacks.size()));
	}
}

//----------------------------------------------------------------------------------------------------
// Handler Registration (Mutex-Protected)
//----------------------------------------------------------------------------------------------------

bool GenericCommandExecutor::RegisterHandler(String const& commandType, HandlerFunc handler)
{
	std::lock_guard<std::mutex> lock(m_handlerMutex);

	if (m_handlers.find(commandType) != m_handlers.end())
	{
		DAEMON_LOG(LogCore, eLogVerbosity::Warning,
		           Stringf("GenericCommandExecutor: Handler already registered for type '%s'",
		               commandType.c_str()));
		return false;
	}

	m_handlers[commandType] = std::move(handler);

	DAEMON_LOG(LogCore, eLogVerbosity::Log,
	           Stringf("GenericCommandExecutor: Registered handler for '%s' (total: %zu)",
	               commandType.c_str(), m_handlers.size()));
	return true;
}

//----------------------------------------------------------------------------------------------------
bool GenericCommandExecutor::UnregisterHandler(String const& commandType)
{
	std::lock_guard<std::mutex> lock(m_handlerMutex);

	auto it = m_handlers.find(commandType);
	if (it == m_handlers.end())
	{
		DAEMON_LOG(LogCore, eLogVerbosity::Warning,
		           Stringf("GenericCommandExecutor: No handler registered for type '%s'",
		               commandType.c_str()));
		return false;
	}

	m_handlers.erase(it);

	DAEMON_LOG(LogCore, eLogVerbosity::Log,
	           Stringf("GenericCommandExecutor: Unregistered handler for '%s' (remaining: %zu)",
	               commandType.c_str(), m_handlers.size()));
	return true;
}

//----------------------------------------------------------------------------------------------------
bool GenericCommandExecutor::HasHandler(String const& commandType) const
{
	std::lock_guard<std::mutex> lock(m_handlerMutex);
	return m_handlers.find(commandType) != m_handlers.end();
}

//----------------------------------------------------------------------------------------------------
std::vector<String> GenericCommandExecutor::GetRegisteredTypes() const
{
	std::lock_guard<std::mutex> lock(m_handlerMutex);

	std::vector<String> types;
	types.reserve(m_handlers.size());

	for (auto const& pair : m_handlers)
	{
		types.push_back(pair.first);
	}

	return types;
}

//----------------------------------------------------------------------------------------------------
// Command Execution (Main Render Thread - Lock-Free)
//----------------------------------------------------------------------------------------------------

void GenericCommandExecutor::ExecuteCommand(GenericCommand const& command)
{
	// Track per-agent submission count
	AgentStatistics& agentStats = m_agentStats[command.agentId];
	++agentStats.submitted;

	//------------------------------------------------------------------------------------------------
	// Rate Limit Check (token bucket, O(1), <1µs)
	//------------------------------------------------------------------------------------------------
	if (m_rateLimitPerAgent > 0 && !command.agentId.empty())
	{
		auto& state = m_agentRateLimits[command.agentId];

		// Initialize new agent state
		if (state.lastRefillTime == 0.0)
		{
			state.tokens         = static_cast<double>(m_rateLimitPerAgent);
			state.lastRefillTime = GetCurrentTimeSeconds();
			state.maxTokens      = m_rateLimitPerAgent;
		}

		if (!state.TryConsume(GetCurrentTimeSeconds()))
		{
			++m_totalRateLimited;
			++agentStats.rateLimited;

			// Log first rejection per agent, then every 100th to avoid log spam
			if (state.rejectedCount == 1 || state.rejectedCount % 100 == 0)
			{
				DAEMON_LOG(LogCore, eLogVerbosity::Warning,
				           Stringf("GenericCommandExecutor: Rate limited agent '%s' (rejected: %u, limit: %u/sec)",
				               command.agentId.c_str(), state.rejectedCount, m_rateLimitPerAgent));
			}

			// If command has a callback, deliver error result
			if (command.callbackId != 0)
			{
				PendingResult pending;
				pending.callbackId = command.callbackId;
				pending.result     = HandlerResult::Error("ERR_RATE_LIMITED");
				pending.ready      = true;
				m_pendingResults[command.callbackId] = std::move(pending);
			}
			return;
		}
	}

	// Look up handler (lock-free read - safe because registration completes before game loop)
	auto it = m_handlers.find(command.type);

	if (it == m_handlers.end())
	{
		++m_totalUnhandled;
		++agentStats.unhandled;

		DAEMON_LOG(LogCore, eLogVerbosity::Warning,
		           Stringf("GenericCommandExecutor: No handler for command type '%s' from agent '%s'",
		               command.type.c_str(), command.agentId.c_str()));

		// Deliver error callback so JS caller gets notified
		if (command.callbackId != 0)
		{
			PendingResult pending;
			pending.callbackId = command.callbackId;
			pending.result     = HandlerResult::Error("ERR_NO_HANDLER");
			pending.ready      = true;
			m_pendingResults[command.callbackId] = std::move(pending);
		}
		return;
	}

	// Execute handler with error isolation
	HandlerResult result;
	bool success = false;
	try
	{
		result = it->second(command.payload);
		++m_totalExecuted;
		++agentStats.executed;
		++m_typeStats[command.type].executed;
		success = true;
	}
	catch (std::bad_any_cast const& e)
	{
		++m_totalErrors;
		++agentStats.failed;
		++m_typeStats[command.type].failed;
		result = HandlerResult::Error(
			Stringf("Bad payload cast for '%s': %s", command.type.c_str(), e.what()));

		DAEMON_LOG(LogCore, eLogVerbosity::Error,
		           Stringf("GenericCommandExecutor: Bad payload cast for '%s' from agent '%s': %s",
		               command.type.c_str(), command.agentId.c_str(), e.what()));
	}
	catch (std::exception const& e)
	{
		++m_totalErrors;
		++agentStats.failed;
		++m_typeStats[command.type].failed;
		result = HandlerResult::Error(
			Stringf("Handler exception for '%s': %s", command.type.c_str(), e.what()));

		DAEMON_LOG(LogCore, eLogVerbosity::Error,
		           Stringf("GenericCommandExecutor: Handler exception for '%s' from agent '%s': %s",
		               command.type.c_str(), command.agentId.c_str(), e.what()));
	}
	catch (...)
	{
		++m_totalErrors;
		++agentStats.failed;
		++m_typeStats[command.type].failed;
		result = HandlerResult::Error(
			Stringf("Unknown exception in handler for '%s'", command.type.c_str()));

		DAEMON_LOG(LogCore, eLogVerbosity::Error,
		           Stringf("GenericCommandExecutor: Unknown exception for '%s' from agent '%s'",
		               command.type.c_str(), command.agentId.c_str()));
	}

	// Audit logging (when enabled)
	if (m_auditLoggingEnabled)
	{
		DAEMON_LOG(LogCore, eLogVerbosity::Log,
		           Stringf("AUDIT: agent='%s' type='%s' callbackId=%llu result=%s%s",
		               command.agentId.c_str(),
		               command.type.c_str(),
		               command.callbackId,
		               success ? "SUCCESS" : "FAILED",
		               success ? "" : Stringf(" error='%s'", result.error.c_str()).c_str()));
	}

	// If command has a callback, store the result for delivery
	if (command.callbackId != 0)
	{
		PendingResult pending;
		pending.callbackId = command.callbackId;
		pending.result     = std::move(result);
		pending.ready      = true;

		m_pendingResults[command.callbackId] = std::move(pending);
	}
}

//----------------------------------------------------------------------------------------------------
// Callback Delivery (Main Render Thread)
//----------------------------------------------------------------------------------------------------

void GenericCommandExecutor::ExecutePendingCallbacks(CallbackQueue* callbackQueue)
{
	GUARANTEE_OR_DIE(callbackQueue != nullptr,
	                 "GenericCommandExecutor::ExecutePendingCallbacks - CallbackQueue is nullptr!");

	// Collect completed callback IDs for cleanup after iteration
	std::vector<uint64_t> completedIds;

	for (auto& pair : m_pendingResults)
	{
		uint64_t       callbackId = pair.first;
		PendingResult& pending    = pair.second;

		if (!pending.ready)
		{
			continue;
		}

		// Create CallbackData for the CallbackQueue
		CallbackData data;
		data.callbackId   = callbackId;
		data.errorMessage = pending.result.error;
		data.type         = CallbackType::GENERIC;

		// Extract resultId from HandlerResult.data if present (used by migrated ScriptInterface handlers)
		data.resultId = 0;
		if (pending.result.IsSuccess())
		{
			auto it2 = pending.result.data.find("resultId");
			if (it2 != pending.result.data.end())
			{
				try
				{
					data.resultId = std::any_cast<uint64_t>(it2->second);
				}
				catch (std::bad_any_cast const&)
				{
					// Try double (JavaScript numbers are doubles)
					try
					{
						data.resultId = static_cast<uint64_t>(std::any_cast<double>(it2->second));
					}
					catch (std::bad_any_cast const&)
					{
						DAEMON_LOG(LogCore, eLogVerbosity::Warning,
						           Stringf("GenericCommandExecutor: resultId in HandlerResult.data has unsupported type for callback %llu", callbackId));
					}
				}
			}

			// Extract resultJson from HandlerResult.data if present (rich JSON payload for GENERIC handlers)
			auto itJson = pending.result.data.find("resultJson");
			if (itJson != pending.result.data.end())
			{
				try
				{
					data.resultJson = std::any_cast<std::string>(itJson->second);
				}
				catch (std::bad_any_cast const&)
				{
					DAEMON_LOG(LogCore, eLogVerbosity::Warning,
					           Stringf("GenericCommandExecutor: resultJson in HandlerResult.data has unsupported type for callback %llu", callbackId));
				}
			}
		}

		bool enqueued = callbackQueue->Enqueue(data);

		if (enqueued)
		{
			completedIds.push_back(callbackId);
		}
		else
		{
			DAEMON_LOG(LogCore, eLogVerbosity::Warning,
			           Stringf("GenericCommandExecutor: CallbackQueue full! Callback %llu deferred",
			               callbackId));
			// Don't remove - will retry next frame
		}
	}

	// Clean up delivered results
	for (uint64_t id : completedIds)
	{
		m_pendingResults.erase(id);
	}
}

//----------------------------------------------------------------------------------------------------
// Pending Callback Storage
//----------------------------------------------------------------------------------------------------

void GenericCommandExecutor::StoreCallback(uint64_t callbackId, std::any callback)
{
	m_storedCallbacks[callbackId] = std::move(callback);
}

//----------------------------------------------------------------------------------------------------
std::any GenericCommandExecutor::RetrieveCallback(uint64_t callbackId)
{
	auto it = m_storedCallbacks.find(callbackId);
	if (it == m_storedCallbacks.end())
	{
		DAEMON_LOG(LogCore, eLogVerbosity::Warning,
		           Stringf("GenericCommandExecutor: Callback %llu not found in stored callbacks",
		               callbackId));
		return {};
	}

	std::any callback = std::move(it->second);
	m_storedCallbacks.erase(it);
	return callback;
}

//----------------------------------------------------------------------------------------------------
// Rate Limiting Configuration
//----------------------------------------------------------------------------------------------------

void GenericCommandExecutor::SetRateLimitPerAgent(uint32_t maxCommandsPerSecond)
{
	m_rateLimitPerAgent = maxCommandsPerSecond;

	// Update existing agent states with new limit
	for (auto& pair : m_agentRateLimits)
	{
		pair.second.maxTokens = maxCommandsPerSecond;
	}

	DAEMON_LOG(LogCore, eLogVerbosity::Log,
	           Stringf("GenericCommandExecutor: Rate limit set to %u commands/sec per agent%s",
	               maxCommandsPerSecond,
	               maxCommandsPerSecond == 0 ? " (DISABLED)" : ""));
}

//----------------------------------------------------------------------------------------------------
RateLimitState const* GenericCommandExecutor::GetAgentRateLimitState(String const& agentId) const
{
	auto it = m_agentRateLimits.find(agentId);
	if (it == m_agentRateLimits.end())
	{
		return nullptr;
	}
	return &it->second;
}

//----------------------------------------------------------------------------------------------------
// Statistics Snapshot
//----------------------------------------------------------------------------------------------------

CommandStatistics GenericCommandExecutor::GetStatistics() const
{
	CommandStatistics stats;

	// Global totals
	stats.totalExecuted    = m_totalExecuted;
	stats.totalErrors      = m_totalErrors;
	stats.totalUnhandled   = m_totalUnhandled;
	stats.totalRateLimited = m_totalRateLimited;

	// Per-agent breakdown (copy)
	stats.agentStats = m_agentStats;

	// Per-type breakdown (copy)
	stats.typeStats = m_typeStats;

	return stats;
}

//----------------------------------------------------------------------------------------------------
// Audit Logging Configuration
//----------------------------------------------------------------------------------------------------

void GenericCommandExecutor::SetAuditLoggingEnabled(bool enabled)
{
	m_auditLoggingEnabled = enabled;

	DAEMON_LOG(LogCore, eLogVerbosity::Log,
	           Stringf("GenericCommandExecutor: Audit logging %s",
	               enabled ? "ENABLED" : "DISABLED"));
}
