//----------------------------------------------------------------------------------------------------
// JobWorkerThread.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Core/JobWorkerThread.hpp"
#include <chrono>
#include "Engine/Core/Job.hpp"
#include "Engine/Core/JobSystem.hpp"

//----------------------------------------------------------------------------------------------------
JobWorkerThread::JobWorkerThread(JobSystem*             jobSystem,
                                 int const              workerID,
                                 WorkerThreadType const workerType)
    : m_jobSystem(jobSystem),
      m_workerID(workerID),
      m_workerType(workerType)
{
    // Constructor stores the reference, ID, and worker type
    // Thread is not started until StartThread() is called
}

//----------------------------------------------------------------------------------------------------
JobWorkerThread::~JobWorkerThread()
{
    // Ensure thread is properly stopped before destruction
    if (m_isRunning.load())
    {
        StopAndJoin();
    }
}

//----------------------------------------------------------------------------------------------------
void JobWorkerThread::StartThread()
{
    if (!m_isRunning.load())
    {
        m_shouldStop.store(false);
        m_thread = std::thread(&JobWorkerThread::ThreadMain, this);
        m_isRunning.store(true);
    }
}

//----------------------------------------------------------------------------------------------------
void JobWorkerThread::StopAndJoin()
{
    RequestStop();
    Join();
}

//----------------------------------------------------------------------------------------------------
void JobWorkerThread::RequestStop()
{
    // Just set the stop flag without joining
    m_shouldStop.store(true);
}

//----------------------------------------------------------------------------------------------------
void JobWorkerThread::Join()
{
    if (m_isRunning.load())
    {
        // Wait for the thread to finish
        if (m_thread.joinable())
        {
            m_thread.join();
        }

        m_isRunning.store(false);
    }
}

//----------------------------------------------------------------------------------------------------
void JobWorkerThread::ThreadMain()
{
    // Continuous job processing loop
    while (!m_shouldStop.load())
    {
        // Try to claim a job from the queued jobs
        if (ClaimJob())
        {
            // If we successfully claimed a job, execute it
            ExecuteAndCompleteJob();
        }
        else
        {
            // No work available - wait on condition variable instead of spinning
            // This is much more efficient than sleeping/yielding in a tight loop
            std::unique_lock<std::mutex> lock(m_jobSystem->m_conditionMutex);
            m_jobSystem->m_jobAvailableCondition.wait_for(
                lock,
                std::chrono::milliseconds(10),  // Timeout after 10ms to check m_shouldStop
                [this]
                {
                    // Predicate: wake up if stopping or if there are jobs available
                    return m_shouldStop.load() || m_jobSystem->GetQueuedJobCount() > 0;
                }
            );
        }
    }
}

//----------------------------------------------------------------------------------------------------
bool JobWorkerThread::ClaimJob()
{
    // Attempt to claim a job from the JobSystem's queued jobs
    // Pass our worker type so we only claim jobs matching our type
    return m_jobSystem->ClaimJobFromQueue(m_currentJob, m_workerType);
}

//----------------------------------------------------------------------------------------------------
void JobWorkerThread::ExecuteAndCompleteJob()
{
    if (m_currentJob != nullptr)
    {
        // Execute the job's work (potentially slow operation)
        // This is not protected by any mutex since each worker has its own job
        m_currentJob->Execute();

        // After execution, move the job to the completed queue
        m_jobSystem->MoveJobToCompleted(m_currentJob);

        // Clear the current job pointer
        m_currentJob = nullptr;
    }
}
