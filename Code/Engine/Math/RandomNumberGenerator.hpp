#pragma once

//-----------------------------------------------------------------------------------------------

class RandomNumberGenerator
{
public:
	int   RollRandomIntLessThan(int maxNotInclusive) const;
	int   RollRandomIntInRange(int minInclusive, int maxInclusive) const;
	float RollRandomFloatZeroToOne() const;
	float RollRandomFloatInRange(float minInclusive, float maxInclusive) const;

private:
	//unsigned int m_speed = 0;		// We will use these later on...
	//int m_position = 0;			// ...when we replace rand() with noise
};