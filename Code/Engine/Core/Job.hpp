//----------------------------------------------------------------------------------------------------
// Job.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
//----------------------------------------------------------------------------------------------------
#include <cstdint>

//----------------------------------------------------------------------------------------------------
// Job type bitfield - allows categorizing jobs for worker thread specialization
// Workers filter jobs based on their assigned type(s)
//----------------------------------------------------------------------------------------------------
using JobType          = uint32_t;
using WorkerThreadType = JobType;

// Standard job types
constexpr JobType JOB_TYPE_GENERIC = 0x01;  // General computation jobs (terrain generation, etc.)
constexpr JobType JOB_TYPE_IO      = 0x02;  // File I/O jobs (load/save chunks)
constexpr JobType JOB_TYPE_ALL     = 0xFF;  // Worker accepts any job type

//----------------------------------------------------------------------------------------------------
// Job - Abstract base class for all job types in the JobSystem
//
// This class defines the interface that all concrete job implementations must follow.
// Jobs are created by client code (typically the main thread), executed by worker threads,
// and deleted by client code after completion and retrieval.
//
// Usage:
//   1. Derive from Job and implement the Execute() method
//   2. Create job instance and submit to JobSystem
//   3. JobSystem will move job through queued -> executing -> completed states
//   4. Retrieve completed job from JobSystem and delete it
//
// Thread Safety:
//   - Job creation: Main thread only
//   - Job execution: Worker threads only (via Execute() method)
//   - Job deletion: Main thread only (after retrieval)
//----------------------------------------------------------------------------------------------------
class Job
{
public:
    // Constructor: Allows derived classes to specify job type
    explicit Job(JobType jobType = JOB_TYPE_GENERIC)
        : m_jobType(jobType)
    {
    }

    // Virtual destructor ensures proper cleanup of derived classes
    virtual ~Job() = default;

    // Pure virtual Execute method - must be implemented by derived classes
    // This method contains the actual work to be performed by the worker thread
    // Should be thread-safe and not access main-thread-only resources (e.g., DirectX)
    virtual void Execute() = 0;

    // Get the job type (used by workers to filter claimable jobs)
    JobType GetJobType() const { return m_jobType; }

    // Prevent copying and assignment (jobs should be unique)
    Job(Job const&)            = delete;
    Job& operator=(Job const&) = delete;
    Job(Job&&)                 = delete;
    Job& operator=(Job&&)      = delete;

private:
    // Job type determines which workers can claim this job
    JobType m_jobType = JOB_TYPE_GENERIC;
};
