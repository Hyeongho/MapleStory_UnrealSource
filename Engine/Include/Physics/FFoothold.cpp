#include "EnginePCH.h"
#include "Physics/FFoothold.h"

FFoothold::FFoothold() = default;

FFoothold::FFoothold(int32 Id, const FVector2D& Start, const FVector2D& End) : m_Id(Id), m_Start(Start), m_End(End)
{
}

int32 FFoothold::GetId() const
{
	return m_Id;
}

const FVector2D& FFoothold::GetStart() const
{
	return m_Start;
}

const FVector2D& FFoothold::GetEnd() const
{
	return m_End;
}

void FFoothold::SetCollisionMask(uint32 Mask)
{
	m_CollisionMask = Mask & AllCollisionChannels;
}

uint32 FFoothold::GetCollisionMask() const
{
	return m_CollisionMask;
}

bool FFoothold::IsValid() const
{
	return m_Id >= 0 && _finite(m_Start.m_X) && _finite(m_Start.m_Y) && _finite(m_End.m_X) && _finite(m_End.m_Y) && FMath::Abs(m_End.m_X - m_Start.m_X) > FMath::KINDA_SMALL_NUMBER;
}

float FFoothold::GetMinX() const
{
	return FMath::Min(m_Start.m_X, m_End.m_X);
}

float FFoothold::GetMaxX() const
{
	return FMath::Max(m_Start.m_X, m_End.m_X);
}

bool FFoothold::ContainsX(float X) const
{
	return X >= GetMinX() && X <= GetMaxX();
}

float FFoothold::GetSlope() const
{
	const float Width = m_End.m_X - m_Start.m_X;

	if (FMath::Abs(Width) <= FMath::KINDA_SMALL_NUMBER)
	{
		return 0.0f;
	}

	return (m_End.m_Y - m_Start.m_Y) / Width;
}

float FFoothold::GetHeightAtX(float X) const
{
	return m_Start.m_Y + (X - m_Start.m_X) * GetSlope();
}

FVector2D FFoothold::GetNormal() const
{
	return FVector2D(GetSlope(), -1.0f).GetNormalized();
}

bool FFoothold::Raycast(const FVector2D& Start, const FVector2D& End, float& OutTime) const
{
	OutTime = 1.0f;

	if (!IsValid())
	{
		return false;
	}

	const FVector2D Delta = End - Start;
	const float Distance = Start.m_Y - GetHeightAtX(Start.m_X);
	const float Approach = Delta.m_Y - GetSlope() * Delta.m_X;

	if (FMath::Abs(Approach) <= FMath::SMALL_NUMBER)
	{
		if (FMath::Abs(Distance) > FMath::SMALL_NUMBER)
		{
			return false;
		}

		if (ContainsX(Start.m_X))
		{
			OutTime = 0.0f;
			return true;
		}

		if (FMath::Abs(Delta.m_X) <= FMath::SMALL_NUMBER)
		{
			return false;
		}

		const float EdgeX = Delta.m_X > 0.0f ? GetMinX() : GetMaxX();
		OutTime = (EdgeX - Start.m_X) / Delta.m_X;
	}

	else
	{
		OutTime = -Distance / Approach;
	}

	if (OutTime < 0.0f || OutTime > 1.0f)
	{
		return false;
	}

	const float HitX = Start.m_X + Delta.m_X * OutTime;

	return HitX >= GetMinX() - FMath::SMALL_NUMBER && HitX <= GetMaxX() + FMath::SMALL_NUMBER;
}