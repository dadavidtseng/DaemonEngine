//----------------------------------------------------------------------------------------------------
// JobSystem.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
//----------------------------------------------------------------------------------------------------
#include "Engine/Core/Job.hpp"  // Need JobType and WorkerThreadType definitions
//----------------------------------------------------------------------------------------------------
#include <condition_variable>  // For efficient worker thread sleeping
#include <deque>
#include <vector>

//----------------------------------------------------------------------------------------------------
class Job;
class JobWorkerThread;

//----------------------------------------------------------------------------------------------------
// JobSystem - Central coordinator for multi-threaded job processing
//
// The JobSystem manages:
// 1. A pool of N worker threads (typically hardware_concurrency() - 1)
// 2. Three job queues: queued (waiting), executing (in progress), completed (finished)
// 3. Thread-safe job submission and retrieval operations
// 4. Worker thread lifecycle management
//
// Job Flow:
// 1. Client code creates Job and calls SubmitJob() -> queued queue
// 2. Worker thread claims job: queued -> executing queue
// 3. Worker executes Job::Execute() method
// 4. Worker moves job: executing -> completed queue
// 5. Client code calls RetrieveCompletedJob() to get finished job
// 6. Client code deletes the job
//
// Thread Safety:
// - All job queue operations are protected by mutexes
// - Job submission: Main thread only
// - Job execution: Worker threads only
// - Job retrieval: Main thread only
//----------------------------------------------------------------------------------------------------
class JobSystem
{
public:
    // Constructor - prepares job system but doesn't start threads
    JobSystem();

    // Destructor - ensures proper cleanup of all threads and jobs
    ~JobSystem();

    // Prevent copying and assignment
    JobSystem(JobSystem const&)            = delete;
    JobSystem& operator=(JobSystem const&) = delete;
    JobSystem(JobSystem&&)                 = delete;
    JobSystem& operator=(JobSystem&&)      = delete;

    // Initialize and start worker threads with specified types
    // numGenericThreads: Number of general-purpose worker threads (for terrain generation, etc.)
    // numIOThreads: Number of dedicated I/O worker threads (for load/save operations, default 1)
    void StartUp(int numGenericThreads, int numIOThreads = 1);

    // Stop all worker threads and clean up resources
    void ShutDown();

    // Submit a job to be processed by worker threads
    // The job will be added to the queued jobs and claimed by next available worker
    // Job ownership transfers to JobSystem until retrieved
    void SubmitJob(Job* job);

    // Retrieve one completed job (if available)
    // Returns nullptr if no completed jobs are available
    // Caller takes ownership and is responsible for deleting the job
    Job* RetrieveCompletedJob();

    // Retrieve all completed jobs
    // Returns vector of completed jobs (may be empty)
    // Caller takes ownership and is responsible for deleting all jobs
    std::vector<Job*> RetrieveAllCompletedJobs();

    // Get the number of jobs in each queue (for debugging/monitoring)
    int GetQueuedJobCount() const;
    int GetExecutingJobCount() const;
    int GetCompletedJobCount() const;

    // Get total number of worker threads
    int GetWorkerThreadCount() const { return static_cast<int>(m_workerThreads.size()); }

private:
    // Friend class allows JobWorkerThread to access private queue methods
    friend class JobWorkerThread;

    // Thread-safe job queue operations (called by worker threads)
    bool ClaimJobFromQueue(Job*& outJob, WorkerThreadType workerType);      // Move job: queued -> executing (filtered by type)
    void MoveJobToCompleted(Job* job);         // Move job: executing -> completed

    // Job queues protected by mutex
    std::deque<Job*> m_queuedJobs;            // Jobs waiting to be claimed
    std::deque<Job*> m_executingJobs;         // Jobs currently being processed
    std::deque<Job*> m_completedJobs;         // Jobs finished and ready for retrieval

    // Mutex for protecting all job queue operations
    mutable std::mutex m_jobQueuesMutex;

    // Condition variable for efficient worker thread waiting
    // Workers sleep on this and are notified when jobs are submitted
    std::condition_variable m_jobAvailableCondition;
    std::mutex m_conditionMutex;  // Separate mutex for condition variable

    // Worker thread management
    std::vector<std::unique_ptr<JobWorkerThread>> m_workerThreads;

    // System state
    bool m_isRunning = false;
};
