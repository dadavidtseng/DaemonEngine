//----------------------------------------------------------------------------------------------------
// JobSystem.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Core/JobSystem.hpp"
#include "Engine/Core/JobWorkerThread.hpp"
#include "Engine/Core/Job.hpp"
#include <thread>
#include <algorithm>

//----------------------------------------------------------------------------------------------------
// JobSystem Implementation
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
JobSystem::JobSystem()
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
        ShutDown();
    }
}

//----------------------------------------------------------------------------------------------------
void JobSystem::StartUp(int numThreads)
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
    
    // Clamp to reasonable bounds
    numThreads = std::max(1, std::min(numThreads, hardwareConcurrency - 1));

    // Create and start worker threads
    m_workerThreads.reserve(numThreads);
    for (int i = 0; i < numThreads; ++i)
    {
        auto workerThread = std::make_unique<JobWorkerThread>(this, i);
        workerThread->StartThread();
        m_workerThreads.push_back(std::move(workerThread));
    }

    m_isRunning = true;
}

//----------------------------------------------------------------------------------------------------
void JobSystem::ShutDown()
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
    std::lock_guard<std::mutex> lock(m_jobQueuesMutex);
    return static_cast<int>(m_queuedJobs.size());
}

//----------------------------------------------------------------------------------------------------
int JobSystem::GetExecutingJobCount() const
{
    std::lock_guard<std::mutex> lock(m_jobQueuesMutex);
    return static_cast<int>(m_executingJobs.size());
}

//----------------------------------------------------------------------------------------------------
int JobSystem::GetCompletedJobCount() const
{
    std::lock_guard<std::mutex> lock(m_jobQueuesMutex);
    return static_cast<int>(m_completedJobs.size());
}

//----------------------------------------------------------------------------------------------------
// Private methods called by JobWorkerThread (friend class)
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
bool JobSystem::ClaimJobFromQueue(Job*& outJob)
{
    std::lock_guard<std::mutex> lock(m_jobQueuesMutex);
    
    if (m_queuedJobs.empty())
    {
        outJob = nullptr;
        return false; // No jobs available to claim
    }

    // Remove job from queued jobs
    Job* claimedJob = m_queuedJobs.front();
    m_queuedJobs.pop_front();
    
    // Add job to executing jobs
    m_executingJobs.push_back(claimedJob);
    
    // Return the claimed job
    outJob = claimedJob;
    return true;
}

//----------------------------------------------------------------------------------------------------
void JobSystem::MoveJobToCompleted(Job* job)
{
    if (!job)
    {
        return; // Invalid job
    }

    std::lock_guard<std::mutex> lock(m_jobQueuesMutex);
    
    // Find and remove the job from executing jobs
    auto it = std::find(m_executingJobs.begin(), m_executingJobs.end(), job);
    if (it != m_executingJobs.end())
    {
        m_executingJobs.erase(it);
        
        // Add job to completed jobs
        m_completedJobs.push_back(job);
    }
}