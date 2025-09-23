//----------------------------------------------------------------------------------------------------
// Job.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once

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
    // Virtual destructor ensures proper cleanup of derived classes
    virtual ~Job() = default;

    // Pure virtual Execute method - must be implemented by derived classes
    // This method contains the actual work to be performed by the worker thread
    // Should be thread-safe and not access main-thread-only resources (e.g., DirectX)
    virtual void Execute() = 0;

    // Prevent copying and assignment (jobs should be unique)
    Job(Job const&)            = delete;
    Job& operator=(Job const&) = delete;
    Job(Job&&)                 = delete;
    Job& operator=(Job&&)      = delete;

protected:
    // Protected constructor prevents direct instantiation of abstract base class
    Job() = default;
};
