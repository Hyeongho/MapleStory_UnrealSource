#include "EnginePCH.h"
#include "Physics/URigidbody.h"

URigidbody::URigidbody() = default;
URigidbody::~URigidbody() = default;

void URigidbody::SetVelocity(const FVector2D& Velocity)
{
	m_Velocity = Velocity;
}

const FVector2D& URigidbody::GetVelocity() const
{
	return m_Velocity;
}

void URigidbody::SetGravityScale(float Scale)
{
	m_GravityScale = FMath::Max(0.0f, Scale);
}

void URigidbody::SetMaxFallSpeed(float Speed)
{
	m_MaxFallSpeed = FMath::Max(0.0f, Speed);
}

void URigidbody::SetSimulatePhysics(bool bSimulate)
{
	m_bSimulatePhysics = bSimulate;
	if (!bSimulate)
	{
		m_bIsGrounded = false;
		m_CurrentFootholdId = INDEX_NONE;
	}
}

bool URigidbody::IsSimulatingPhysics() const
{
	return m_bSimulatePhysics;
}

bool URigidbody::IsGrounded() const
{
	return m_bIsGrounded;
}

int32 URigidbody::GetCurrentFootholdId() const
{
	return m_CurrentFootholdId;
}