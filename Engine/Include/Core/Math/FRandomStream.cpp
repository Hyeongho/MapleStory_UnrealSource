#include "EnginePCH.h"
#include "FRandomStream.h"

FRandomStream::FRandomStream() : m_InitialSeed(0), m_Seed(0)
{
}

FRandomStream::FRandomStream(int32 Seed) : m_InitialSeed(Seed), m_Seed(Seed)
{
}

void FRandomStream::Initialize(int32 Seed)
{
	m_InitialSeed = Seed;
	m_Seed = Seed;
}

void FRandomStream::Reset()
{
	m_Seed = m_InitialSeed;
}

int32 FRandomStream::NextSeed()
{
	m_Seed = (int32)(((uint32)m_Seed) * 196314165u + 907633515u);

	return m_Seed;
}

float FRandomStream::FRand()
{
	uint32 Bits = (uint32)FMath::Abs(NextSeed());

	return (float)(Bits & 0x7FFFFFu) / (float)(1u << 23u);
}

int32 FRandomStream::RandHelper(int32 A)
{
	if (A <= 0) 
	{
		return 0;
	}

	return (int32)((uint32)FMath::Abs(NextSeed()) % (uint32)A);
}

int32 FRandomStream::RandRange(int32 Min, int32 Max)
{
	if (Min >= Max) 
	{
		return Min;
	}

	return Min + RandHelper(Max - Min + 1);
}

float FRandomStream::FRandRange(float Min, float Max)
{
	return Min + FRand() * (Max - Min);
}

bool FRandomStream::RandBool()
{
	return (FMath::Abs(NextSeed()) & 1) != 0;
}