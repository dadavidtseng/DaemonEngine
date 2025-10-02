//----------------------------------------------------------------------------------------------------
// GEngine.cpp
// Global engine singleton implementation
//----------------------------------------------------------------------------------------------------

#include "Engine/Core/Engine.hpp"
#include "Engine/Core/JobSystem.hpp"

//----------------------------------------------------------------------------------------------------
GEngine& GEngine::Get()
{
    static GEngine instance;
    return instance;
}

//----------------------------------------------------------------------------------------------------
void GEngine::Initialize(JobSystem* jobSystem)
{
    m_jobSystem = jobSystem;
}

//----------------------------------------------------------------------------------------------------
void GEngine::Shutdown()
{
    m_jobSystem = nullptr;
}
