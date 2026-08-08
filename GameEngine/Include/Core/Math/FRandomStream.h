#pragma once

#include "FMath.h"

class FRandomStream
{
public:
	FRandomStream();
	explicit FRandomStream(int32 Seed);

	void Initialize(int32 Seed);
	void Reset(); // m_InitialSeed으로 되돌림

	int32 RandHelper(int32 A); // [0, A)
	int32 RandRange(int32 Min, int32 Max); // [Min, Max] 포함
	float FRand(); // [0.0f, 1.0f)
	float FRandRange(float Min, float Max); // [Min, Max]
	bool  RandBool();

	int32 GetCurrentSeed() const 
	{ 
		return m_Seed; 
	}

private:
	int32 m_InitialSeed;
	int32 m_Seed;

	int32 NextSeed();
};


