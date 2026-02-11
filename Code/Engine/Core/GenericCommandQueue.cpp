//----------------------------------------------------------------------------------------------------
// GenericCommandQueue.cpp
// GenericCommand System - Command Queue Implementation
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Core/GenericCommandQueue.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/LogSubsystem.hpp"
#include "Engine/Core/EngineCommon.hpp"

//----------------------------------------------------------------------------------------------------
// Constructor
//
// Initializes CommandQueueBase with specified capacity.
// Logs queue initialization for monitoring.
//----------------------------------------------------------------------------------------------------
GenericCommandQueue::GenericCommandQueue(size_t const capacity)
	: CommandQueueBase<GenericCommand>(capacity)
{
	if (capacity == 0)
	{
		ERROR_AND_DIE("GenericCommandQueue: Capacity must be greater than zero");
	}

	DAEMON_LOG(LogCore, eLogVerbosity::Log,
	           Stringf("GenericCommandQueue: Initialized with capacity %llu (%.2f KB)",
	               static_cast<uint64_t>(capacity),
	               (capacity * sizeof(GenericCommand)) / 1024.f));
}

//----------------------------------------------------------------------------------------------------
// Destructor
//
// Logs final statistics for debugging/profiling.
// Base class (CommandQueueBase) handles buffer deallocation.
//----------------------------------------------------------------------------------------------------
GenericCommandQueue::~GenericCommandQueue()
{
	uint64_t totalSubmitted = GetTotalSubmitted();
	uint64_t totalConsumed  = GetTotalConsumed();

	DAEMON_LOG(LogCore, eLogVerbosity::Log,
	           Stringf("GenericCommandQueue: Shutdown - Total submitted: %llu, Total consumed: %llu, Lost: %llu",
	               totalSubmitted,
	               totalConsumed,
	               totalSubmitted - totalConsumed));
}

//----------------------------------------------------------------------------------------------------
// OnQueueFull (Virtual Hook Override)
//
// Called by CommandQueueBase::Submit() when queue is full.
// Logs warning for monitoring/debugging.
//----------------------------------------------------------------------------------------------------
void GenericCommandQueue::OnQueueFull()
{
	DAEMON_LOG(LogCore, eLogVerbosity::Warning,
	           Stringf("GenericCommandQueue: Queue full! Capacity: %llu, Submitted: %llu, Consumed: %llu",
	               static_cast<uint64_t>(GetCapacity()),
	               GetTotalSubmitted(),
	               GetTotalConsumed()));
}
