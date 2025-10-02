//----------------------------------------------------------------------------------------------------
// RandomNumberGenerator.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Math/RandomNumberGenerator.hpp"
//----------------------------------------------------------------------------------------------------
#include <cstdlib>

//----------------------------------------------------------------------------------------------------
int RandomNumberGenerator::RollRandomIntLessThan(int const maxNotInclusive) const
{
    return rand() % maxNotInclusive;
}

//----------------------------------------------------------------------------------------------------
int RandomNumberGenerator::RollRandomIntInRange(int const minInclusive,
                                                int const maxInclusive) const
{
    return rand() % (maxInclusive - minInclusive + 1) + minInclusive;
}

//----------------------------------------------------------------------------------------------------
float RandomNumberGenerator::RollRandomFloatZeroToOne() const
{
    return static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
}

//----------------------------------------------------------------------------------------------------
float RandomNumberGenerator::RollRandomFloatInRange(float const minInclusive,
                                                    float const maxInclusive) const
{
    float const randomZeroToOne = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);

    return minInclusive + randomZeroToOne * (maxInclusive - minInclusive);
}
