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

bool URigidbody::TryJump(float Speed)
{
	if (!m_bSimulatePhysics || (!m_bIsGrounded && !m_bIsClimbing && m_CoyoteTimeRemaining <= 0.0f)
		|| !_finite(Speed) || Speed <= 0.0f)
	{
		return false;
	}

	if (m_bIsClimbing)
	{
		// 해제 시 속도가 초기화되므로 점프 속도는 StopClimbing 이후에 적용한다.
		StopClimbing();
		m_ClimbReentryRemaining = m_ClimbReentryDelay;
		m_LastFootholdId = INDEX_NONE;
	}

	// 월드 Y는 아래쪽이 양수다. 입력 프레임에 접지를 해제해 연속 입력도 막는다.
	m_Velocity.m_Y = -Speed;
	m_bIsGrounded = false;
	m_CoyoteTimeRemaining = 0.0f;
	m_CurrentFootholdId = INDEX_NONE;
	m_IgnoredFootholdId = INDEX_NONE;
	m_bDropThroughRequested = false;
	return true;
}

void URigidbody::RequestDropThroughFoothold()
{
	if (!m_bSimulatePhysics || !m_bIsGrounded || m_bIsClimbing || m_CurrentFootholdId == INDEX_NONE)
	{
		return;
	}

	m_bDropThroughRequested = true;
}

void URigidbody::SetGravityScale(float Scale)
{
	m_GravityScale = FMath::Max(0.0f, Scale);
}

void URigidbody::SetMaxFallSpeed(float Speed)
{
	m_MaxFallSpeed = FMath::Max(0.0f, Speed);
}

void URigidbody::SetMaxDropHeight(float Height)
{
	if (_finite(Height) && Height >= 0.0f)
	{
		m_MaxDropHeight = Height;
	}
}

void URigidbody::SetCoyoteTime(float Time)
{
	if (_finite(Time) && Time >= 0.0f)
	{
		m_CoyoteTime = Time;
		m_CoyoteTimeRemaining = FMath::Min(m_CoyoteTimeRemaining, Time);
	}
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

void URigidbody::SetClimbReentryDelay(float Delay)
{
	if (_finite(Delay) && Delay >= 0.0f)
	{
		m_ClimbReentryDelay = Delay;
	}
}

void URigidbody::StopClimbing()
{
	if (m_bIsClimbing)
	{
		m_Velocity = FVector2D::Zero;
	}

	m_bIsClimbing = false;
	m_ClimbableActorId = 0;
	m_ClimbInput = 0.0f;
}

bool URigidbody::IsClimbing() const
{
	return m_bIsClimbing;
}

uint32 URigidbody::GetClimbableActorId() const
{
	return m_ClimbableActorId;
}

EClimbableType URigidbody::GetClimbableType() const
{
	return m_ClimbableType;
}

void URigidbody::SetSimulatePhysics(bool bSimulate)
{
	m_bSimulatePhysics = bSimulate;

	if (!bSimulate)
	{
		StopClimbing();
		m_ClimbReentryRemaining = 0.0f;
		m_bIsGrounded = false;
		m_CoyoteTimeRemaining = 0.0f;
		m_CurrentFootholdId = INDEX_NONE;
		m_LastFootholdId = INDEX_NONE;
		m_IgnoredFootholdId = INDEX_NONE;
		m_bDropThroughRequested = false;
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
