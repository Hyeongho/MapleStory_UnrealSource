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

void URigidbody::SetClimbInput(float Direction)
{
	m_ClimbInput = _finite(Direction) ? FMath::Clamp(Direction, -1.0f, 1.0f) : 0.0f;
}

void URigidbody::SetClimbSpeed(float Speed)
{
	if (_finite(Speed) && Speed >= 0.0f)
	{
		m_ClimbSpeed = Speed;
	}
}

void URigidbody::StopClimbing()
{
	if (m_bIsClimbing)
	{
		m_Velocity = FVector2D::Zero;
	}

	m_bIsClimbing = false;
	m_ClimbInput = 0.0f;
}

bool URigidbody::IsClimbing() const
{
	return m_bIsClimbing;
}

void URigidbody::SetSimulatePhysics(bool bSimulate)
{
	m_bSimulatePhysics = bSimulate;

	if (!bSimulate)
	{
		StopClimbing();
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