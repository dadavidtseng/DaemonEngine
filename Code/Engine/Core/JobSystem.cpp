//----------------------------------------------------------------------------------------------------
// JobSystem.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Core/JobSystem.hpp"
#include "Engine/Core/JobWorkerThread.hpp"
#include "Engine/Core/Job.hpp"
#include <algorithm>
#include <thread>
#include <algorithm>

//----------------------------------------------------------------------------------------------------
// Global JobSystem instance
//----------------------------------------------------------------------------------------------------
// JobSystem* g_jobSystem = nullptr;  // Created and owned by App

//----------------------------------------------------------------------------------------------------
// JobSystem Implementation
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
JobSystem::JobSystem(sJobSubsystemConfig const& config)
    : m_config(config)
{
    // Constructor just initializes member variables
    // Actual thread creation happens in StartUp()
}

//----------------------------------------------------------------------------------------------------
JobSystem::~JobSystem()
{
    // Ensure proper cleanup
    if (m_isRunning)
    {
        Shutdown();
    }
}

//----------------------------------------------------------------------------------------------------
void JobSystem::Startup()
{
    if (m_isRunning)
    {
        return; // Already started
    }

    // Ensure we have at least 1 thread, but not more than hardware supports
    int hardwareConcurrency = static_cast<int>(std::thread::hardware_concurrency());
    if (hardwareConcurrency <= 1)
    {
        hardwareConcurrency = 2; // Fallback if hardware_concurrency() returns 0 or 1
    }

    // Calculate total threads needed
    int totalThreads = m_config.m_genericThreadNum + m_config.m_ioThreadNum;

    // Ensure we don't exceed hardware capabilities (reserve 1 core for main thread)
    if (totalThreads > hardwareConcurrency - 1)
    {
        // Reduce generic threads first, but keep at least 1
        m_config.m_genericThreadNum = hardwareConcurrency - 1 - m_config.m_ioThreadNum;
        m_config.m_genericThreadNum = std::max(m_config.m_genericThreadNum, 1);
        totalThreads      = m_config.m_genericThreadNum + m_config.m_ioThreadNum;
    }

    // Reserve space for all worker threads
    m_workerThreads.reserve(totalThreads);

    // Create I/O worker threads first (dedicated to file operations)
    for (int i = 0; i < m_config.m_ioThreadNum; ++i)
    {
        auto workerThread = std::make_unique<JobWorkerThread>(this, i, JOB_TYPE_IO);
        workerThread->StartThread();
        m_workerThreads.push_back(std::move(workerThread));
    }

    // Create generic worker threads (for terrain generation, etc.)
    for (int i = 0; i < m_config.m_genericThreadNum; ++i)
    {
        int  workerID     = m_config.m_ioThreadNum + i;  // Offset ID after I/O threads
        auto workerThread = std::make_unique<JobWorkerThread>(this, workerID, JOB_TYPE_GENERIC);
        workerThread->StartThread();
        m_workerThreads.push_back(std::move(workerThread));
    }

    m_isRunning = true;
}

//----------------------------------------------------------------------------------------------------
void JobSystem::Shutdown()
{
    if (!m_isRunning)
    {
        return; // Already shut down
    }

    // Signal all worker threads to stop and wait for them to finish
    for (auto& workerThread : m_workerThreads)
    {
        if (workerThread)
        {
            workerThread->StopAndJoin();
        }
    }

    // Clear the worker thread vector
    m_workerThreads.clear();

    // Clean up any remaining jobs in the queues
    {
        std::lock_guard<std::mutex> lock(m_jobQueuesMutex);

        // Delete any jobs still in queues (client should have retrieved completed jobs)
        for (Job* job : m_queuedJobs)
        {
            delete job;
        }
        for (Job* job : m_executingJobs)
        {
            delete job;
        }
        for (Job* job : m_completedJobs)
        {
            delete job;
        }

        // Clear all queues
        m_queuedJobs.clear();
        m_executingJobs.clear();
        m_completedJobs.clear();
    }

    m_isRunning = false;
}

//----------------------------------------------------------------------------------------------------
void JobSystem::SubmitJob(Job* job)
{
    if (!job || !m_isRunning)
    {
        return; // Invalid job or system not running
    }

    // Add job to the queued jobs (thread-safe)
    {
        std::lock_guard<std::mutex> lock(m_jobQueuesMutex);
        m_queuedJobs.push_back(job);
    }

    // Notify one waiting worker thread that work is available
    // This wakes up sleeping workers efficiently instead of spin-waiting
    m_jobAvailableCondition.notify_one();
}

//----------------------------------------------------------------------------------------------------
Job* JobSystem::RetrieveCompletedJob()
{
    if (!m_isRunning)
    {
        return nullptr;
    }

    std::lock_guard<std::mutex> lock(m_jobQueuesMutex);

    if (m_completedJobs.empty())
    {
        return nullptr; // No completed jobs available
    }

    // Remove and return the first completed job
    Job* completedJob = m_completedJobs.front();
    m_completedJobs.pop_front();
    return completedJob;
}

//----------------------------------------------------------------------------------------------------
std::vector<Job*> JobSystem::RetrieveAllCompletedJobs()
{
    std::vector<Job*> allCompletedJobs;

    if (!m_isRunning)
    {
        return allCompletedJobs; // Return empty vector
    }

    std::lock_guard<std::mutex> lock(m_jobQueuesMutex);

    // Move all completed jobs to the return vector
    allCompletedJobs.reserve(m_completedJobs.size());
    for (Job* job : m_completedJobs)
    {
        allCompletedJobs.push_back(job);
    }

    // Clear the completed jobs queue
    m_completedJobs.clear();

    return allCompletedJobs;
}

//----------------------------------------------------------------------------------------------------
int JobSystem::GetQueuedJobCount() const
{
    std::scoped_lock lock(m_jobQueuesMutex);

    return static_cast<int>(m_queuedJobs.size());
}

//----------------------------------------------------------------------------------------------------
int JobSystem::GetExecutingJobCount() const
{
    std::scoped_lock lock(m_jobQueuesMutex);

    return static_cast<int>(m_executingJobs.size());
}

//----------------------------------------------------------------------------------------------------
int JobSystem::GetCompletedJobCount() const
{
    std::scoped_lock lock(m_jobQueuesMutex);

    return static_cast<int>(m_completedJobs.size());
}

//----------------------------------------------------------------------------------------------------
// Private methods called by JobWorkerThread (friend class)
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
bool JobSystem::ClaimJobFromQueue(Job*&                  out_job,
                                  WorkerThreadType const workerType)
{
    std::scoped_lock lock(m_jobQueuesMutex);

    // Find first job that matches worker type (bitfield check)
    for (auto it = m_queuedJobs.begin(); it != m_queuedJobs.end(); ++it)
    {
        Job* job = *it;

        // Check if worker can handle this job type (bitwise AND)
        // Example: If worker is JOB_TYPE_IO (0x02) and job is JOB_TYPE_IO (0x02), then (0x02 & 0x02) = 0x02 != 0
        // Example: If worker is JOB_TYPE_GENERIC (0x01) and job is JOB_TYPE_IO (0x02), then (0x01 & 0x02) = 0x00 == 0
        if ((job->GetJobType() & workerType) != 0)
        {
            // Remove job from queued jobs
            m_queuedJobs.erase(it);

            // Add job to executing jobs
            m_executingJobs.push_back(job);

            // Return the claimed job
            out_job = job;
            return true;
        }
    }

    // No matching job found
    out_job = nullptr;
    return false;
}

//----------------------------------------------------------------------------------------------------
void JobSystem::MoveJobToCompleted(Job* job)
{
    if (job == nullptr) return; // Invalid job

    std::scoped_lock lock(m_jobQueuesMutex);

    // Find and remove the job from executing jobs
    auto const it = std::ranges::find(m_executingJobs, job);

    if (it != m_executingJobs.end())
    {
        m_executingJobs.erase(it);

        // Add job to completed jobs
        m_completedJobs.push_back(job);
    }
}
