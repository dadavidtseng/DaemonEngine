//----------------------------------------------------------------------------------------------------
// Job.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Core/Job.hpp"

//----------------------------------------------------------------------------------------------------
// Job implementation
//
// Since Job is an abstract base class with a defaulted virtual destructor,
// this file primarily serves to:
// 1. Ensure proper linkage for the abstract class
// 2. House any future common functionality for all job types
//
// Note: The virtual destructor is defaulted in the header file,
// so no implementation is needed here.
//----------------------------------------------------------------------------------------------------