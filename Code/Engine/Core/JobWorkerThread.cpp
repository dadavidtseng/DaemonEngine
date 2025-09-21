//----------------------------------------------------------------------------------------------------
// JobWorkerThread.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Core/JobWorkerThread.hpp"
#include <chrono>
#include "Engine/Core/Job.hpp"
#include "Engine/Core/JobSystem.hpp"

//----------------------------------------------------------------------------------------------------
JobWorkerThread::JobWorkerThread(JobSystem* jobSystem,
                                 int const  workerID)
    : m_jobSystem(jobSystem),
      m_workerID(workerID)
{
    // Constructor just stores the reference and ID
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
    if (m_isRunning.load())
    {
        // Signal the thread to stop
        m_shouldStop.store(true);

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
            // No work available - sleep for 1 microsecond to be cooperative
            // This prevents the thread from consuming excessive CPU while waiting
            std::this_thread::sleep_for(std::chrono::microseconds(1));

            // Yield the current time slice to other threads
            std::this_thread::yield();
        }
    }
}

//----------------------------------------------------------------------------------------------------
bool JobWorkerThread::ClaimJob()
{
    // Attempt to claim a job from the JobSystem's queued jobs
    return m_jobSystem->ClaimJobFromQueue(m_currentJob);
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
