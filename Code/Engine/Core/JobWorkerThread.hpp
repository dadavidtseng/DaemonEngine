//----------------------------------------------------------------------------------------------------
// JobWorkerThread.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include <thread>
#include <atomic>
#include "Engine/Core/Job.hpp"  // Need JobType and WorkerThreadType definitions

//-Forward-Declaration--------------------------------------------------------------------------------
class Job;
class JobSystem;

//----------------------------------------------------------------------------------------------------
// JobWorkerThread - Individual worker thread that processes jobs from the JobSystem
//
// Each worker thread:
// 1. Continuously looks for available jobs in the JobSystem's queued jobs
// 2. Claims jobs in a thread-safe manner, moving them to executing queue
// 3. Only claims jobs matching its worker type (e.g., I/O thread only claims I/O jobs)
// 4. Executes the job's Execute() method (potentially slow operation)
// 5. Moves completed jobs to the completed queue in a thread-safe manner
// 6. Repeats until signaled to stop
//
// Thread Safety:
// - All job queue operations are protected by mutexes in JobSystem
// - Uses atomic boolean for thread shutdown signaling
// - Spins with 1μs sleep when no work is available (cooperative multitasking)
//----------------------------------------------------------------------------------------------------
class JobWorkerThread
{
public:
    // Constructor: associates worker with JobSystem, assigns unique ID and worker type
    explicit JobWorkerThread(JobSystem* jobSystem, int workerID, WorkerThreadType workerType = JOB_TYPE_GENERIC);

    // Destructor: ensures proper thread cleanup
    ~JobWorkerThread();

    // Prevent copying and assignment
    JobWorkerThread(JobWorkerThread const&)            = delete;
    JobWorkerThread& operator=(JobWorkerThread const&) = delete;
    JobWorkerThread(JobWorkerThread&&)                 = delete;
    JobWorkerThread& operator=(JobWorkerThread&&)      = delete;

    // Start the worker thread (creates std::thread and begins ThreadMain)
    void StartThread();

    // Signal the worker thread to stop and wait for it to finish
    void StopAndJoin();

    // Get this worker's unique ID
    int GetWorkerID() const { return m_workerID; }

    // Get this worker's type (which jobs it can claim)
    WorkerThreadType GetWorkerType() const { return m_workerType; }

    // Check if the worker thread is currently running
    bool IsRunning() const { return m_isRunning.load(); }

private:
    // Main thread entry point - continuous job processing loop
    void ThreadMain();

    // Attempt to claim a job from the JobSystem's queued jobs
    // Returns true if a job was successfully claimed and is ready for execution
    bool ClaimJob();

    // Execute the currently claimed job and move it to completed queue
    void ExecuteAndCompleteJob();

    // JobSystem reference for accessing job queues
    JobSystem* m_jobSystem = nullptr;

    // Unique identifier for this worker thread
    int m_workerID = -1;

    // Worker type determines which jobs this worker can claim
    WorkerThreadType m_workerType = JOB_TYPE_GENERIC;

    // The actual std::thread object
    std::thread m_thread;

    // Atomic flag to signal thread shutdown
    std::atomic<bool> m_shouldStop{false};

    // Atomic flag indicating if thread is currently running
    std::atomic<bool> m_isRunning{false};

    // Currently claimed job (only accessed by this worker thread)
    Job* m_currentJob = nullptr;
};
