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
	// 생성 / 소멸
	URigidbody();
	virtual ~URigidbody() override;

	// 이동 속도
	void SetVelocity(const FVector2D& Velocity);
	const FVector2D& GetVelocity() const;

	// 중력 / 낙하 설정
	void SetGravityScale(float Scale);
	void SetMaxFallSpeed(float Speed);

	// 로프·사다리 이동 입력과 속도
	// Direction은 -1(위)부터 1(아래)까지이며, 0이면 영역 안에서 멈춘다.
	void SetClimbInput(float Direction);
	void SetClimbSpeed(float Speed);
	void StopClimbing();
	bool IsClimbing() const;

	// 시뮬레이션 활성 상태
	void SetSimulatePhysics(bool bSimulate);
	bool IsSimulatingPhysics() const;

	// 충돌 결과 조회
	bool IsGrounded() const;
	int32 GetCurrentFootholdId() const;

protected:
	// 파생 강체의 이동 계산에서 확장할 수 있는 상태
	FVector2D m_Velocity;
	float m_GravityScale = 1.0f;
	float m_MaxFallSpeed = 2000.0f;
	float m_ClimbInput = 0.0f;
	float m_ClimbSpeed = 100.0f;

private:
	// 물리 월드는 속도와 접지 상태를 갱신한다.
	friend class FPhysicsWorld;

	// 시뮬레이션 활성 여부와 월드에서 판정한 접지 결과
	bool m_bSimulatePhysics = true;
	bool m_bIsGrounded = false;
	bool m_bIsClimbing = false;
	int32 m_CurrentFootholdId = INDEX_NONE;
};