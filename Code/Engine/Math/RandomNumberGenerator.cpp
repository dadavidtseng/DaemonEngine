#include "Engine/Math/RandomNumberGenerator.hpp"
#include <cstdlib>

//-----------------------------------------------------------------------------------------------
int RandomNumberGenerator::RollRandomIntLessThan(int maxNotInclusive) const
{
	return rand() % maxNotInclusive;
}

int RandomNumberGenerator::RollRandomIntInRange(int minInclusive, int maxInclusive) const
{
	return rand() % (maxInclusive - minInclusive + 1) + minInclusive;
}

float RandomNumberGenerator::RollRandomFloatZeroToOne() const
{
	return static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
}

float RandomNumberGenerator::RollRandomFloatInRange(float minInclusive, float maxInclusive) const
{
	float randomZeroToOne = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);

	return minInclusive + randomZeroToOne * (maxInclusive - minInclusive);
}
