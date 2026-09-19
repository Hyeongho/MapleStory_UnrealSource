#pragma once
#include "Object/UActorComponent.h"
#include "Core/Math/FVector2D.h"

class FPhysicsWorld;

// 속도와 중력 상태를 보관하며 UWorld의 물리 단계에서 갱신한다.
// 소유 액터의 첫 번째 UBoxCollision을 이동시킨다. 해당 Box에는 부모가 없어야 하며,
// 시각 컴포넌트를 Box에 부착하면 물리 위치를 따라간다.
// 액터당 하나의 강체/Box를 시뮬레이션하며 동적 강체끼리의 충돌 반응은 아직 지원하지 않는다.
class URigidbody : public UActorComponent
{
	DECLARE_CLASS(URigidbody, UActorComponent)
public:
	URigidbody();
	virtual ~URigidbody() override;
	void SetVelocity(const FVector2D& Velocity);
	const FVector2D& GetVelocity() const;
	void SetGravityScale(float Scale);
	void SetMaxFallSpeed(float Speed);
	void SetSimulatePhysics(bool bSimulate);
	bool IsSimulatingPhysics() const;
	bool IsGrounded() const;

private:
	friend class FPhysicsWorld;
	FVector2D m_Velocity;
	float m_GravityScale = 1.0f;
	float m_MaxFallSpeed = 2000.0f;
	bool m_bSimulatePhysics = true;
	bool m_bIsGrounded = false;
};
