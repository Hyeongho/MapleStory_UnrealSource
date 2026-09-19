#pragma once
#include "Physics/CollisionTypes.h"
#include "Core/Containers/TArray.h"
#include "Physics/FFoothold.h"

class UWorld;
class AActor;
class URigidbody;
class UBoxCollision;
class UClimbableComponent;

class FPhysicsWorld
{
public:
	// 생성 / 소멸
	explicit FPhysicsWorld(UWorld& World);
	~FPhysicsWorld();

	// 월드 시뮬레이션 갱신
	void Tick(float DeltaTime);

	// 월드 중력 설정 / 조회
	void SetGravity(float Gravity);
	float GetGravity() const;

	// 정적 선분 발판 등록 / 제거
	// 중복 ID와 수직·무효 선분은 거부한다. 파싱 계층은 검증된 선분을 이 API로 전달한다.
	bool AddFoothold(const FFoothold& Foothold);
	bool RemoveFoothold(int32 Id);
	void ClearFootholds();

	// 발판 조회 — 반환 포인터는 발판 목록 변경 전까지만 유효
	const FFoothold* FindFoothold(int32 Id) const;

	// 충돌 쿼리
	bool Raycast(const FVector2D& Start, const FVector2D& End, FHitResult& OutHit,
	    uint32 ObjectMask = AllCollisionChannels, const AActor* IgnoreActor = nullptr) const;
	void FindOverlaps(TArray<FOverlapResult>& OutOverlaps) const;

private:
	// 차단 대상 필터링
	static bool IsBlocker(const UBoxCollision& Box, const UPrimitiveComponent& Other);

	// 충돌에 따른 위치 / 속도 보정
	static void Translate(UBoxCollision& Box, const FVector2D& Delta);
	static void ResolveVelocity(URigidbody& Body, const FVector2D& Normal);

	// 아래쪽 지지점의 단방향 착지 / 경사면 지지 판정
	static FVector2D GetFootPosition(const UBoxCollision& Box);
	static bool CanUseFoothold(const UBoxCollision& Box, const FFoothold& Foothold);
	static bool SweepFoothold(const FFoothold& Foothold, const FVector2D& Start,
		const FVector2D& Delta, float& OutTime);
	const FFoothold* FindSupportingFoothold(const UBoxCollision& Box, float DirectionX) const;
	void InvalidateFootholdSupport(int32 Id);

	// 로프·사다리 Trigger와 강체의 겹침 판정
	static const UClimbableComponent* FindClimbable(
		const UBoxCollision& Box, const TArray<UPrimitiveComponent*>& Primitives);

	// 도형 수집 / 개별 강체 시뮬레이션
	void GatherPrimitives(TArray<UPrimitiveComponent*>& Out) const;
	void SimulateBody(
	    URigidbody& Body, UBoxCollision& Box, float DeltaTime, const TArray<UPrimitiveComponent*>& Primitives);

	// 소유 월드 참조
	UWorld& m_World;

	// 월드 물리 설정
	float m_Gravity = 980.0f;

	// 월드가 소유하는 정적 발판 데이터
	TArray<FFoothold> m_Footholds;
};
