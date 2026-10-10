#pragma once
#include "Object/UActorComponent.h"
#include "Core/Math/FVector2D.h"
#include "Physics/CollisionTypes.h"

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
	// 지상·코요테 타임 안에서는 첫 점프, 매달린 상태에서는 점프 이탈한다.
	// 성공하면 접지와 남은 점프 허용 시간을 즉시 해제해 공중 추가 점프를 막는다.
	bool TryJump(float Speed);
	// 하단 점프 요청. 실제 착지 가능 높이는 물리 월드가 다음 Tick에서 검사한다.
	void RequestDropThroughFoothold();

	// 중력 / 낙하 설정
	void SetGravityScale(float Scale);
	void SetMaxFallSpeed(float Speed);
	void SetMaxDropHeight(float Height);
	// 지면에서 떨어진 뒤 첫 점프를 허용하는 시간(초). 0이면 즉시 차단한다.
	void SetCoyoteTime(float Time);

	// 로프·사다리 이동 입력과 속도
	// Direction은 -1(위)부터 1(아래)까지이며, 0이면 영역 안에서 멈춘다.
	void SetClimbInput(float Direction);
	void SetClimbSpeed(float Speed);
	// 점프 이탈 후 다시 잡을 수 있을 때까지의 시간(초)을 설정한다.
	void SetClimbReentryDelay(float Delay);
	void StopClimbing();
	bool IsClimbing() const;
	// 매달린 영역의 소유 액터 ID. 매달려 있지 않으면 0을 반환한다.
	uint32 GetClimbableActorId() const;
	// IsClimbing()이 참일 때 현재 매달린 영역의 종류를 반환한다.
	EClimbableType GetClimbableType() const;

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
	float m_MaxDropHeight = 300.0f;
	float m_CoyoteTime = 0.1f;
	float m_ClimbInput = 0.0f;
	float m_ClimbSpeed = 100.0f;
	float m_ClimbReentryDelay = 0.2f;

private:
	// 물리 월드는 속도와 접지 상태를 갱신한다.
	friend class FPhysicsWorld;

	// 시뮬레이션 활성 여부와 월드에서 판정한 접지 결과
	bool m_bSimulatePhysics = true;
	bool m_bIsGrounded = false;
	float m_CoyoteTimeRemaining = 0.0f;
	bool m_bIsClimbing = false;
	// 매달린 영역을 액터 ID로 추적한다. 겹친 다른 사다리로 임의 전환하지 않는다.
	uint32 m_ClimbableActorId = 0;
	EClimbableType m_ClimbableType = EClimbableType::Ladder;
	float m_ClimbReentryRemaining = 0.0f;
	int32 m_CurrentFootholdId = INDEX_NONE;
	// 점프 중에도 직전에 밟은 발판 그룹을 우선 판정하는 데 사용한다.
	int32 m_LastFootholdId = INDEX_NONE;
	// 하단 점프 중에는 이 단방향 발판의 지지·충돌 판정만 건너뛴다.
	int32 m_IgnoredFootholdId = INDEX_NONE;
	bool m_bDropThroughRequested = false;
};
