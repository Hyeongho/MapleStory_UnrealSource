#pragma once
#include "Physics/CollisionTypes.h"
#include "Core/Containers/TArray.h"

class UWorld;
class AActor;
class URigidbody;
class UBoxCollision;
class FRect;

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

	// 충돌 쿼리
	bool Raycast(const FVector2D& Start, const FVector2D& End, FHitResult& OutHit,
	    uint32 ObjectMask = AllCollisionChannels, const AActor* IgnoreActor = nullptr) const;
	void FindOverlaps(TArray<FOverlapResult>& OutOverlaps) const;

private:
	// 교차 검사 / 도형 확장
	static bool IntersectBox(const FVector2D& Start, const FVector2D& Delta, const FRect& Bounds, float& Time,
	    FVector2D& Normal, bool bSweep);
	static FRect Expanded(const FRect& Bounds, const FVector2D& Extent);

	// 차단 대상 필터링
	static bool IsBlocker(const UBoxCollision& Box, const UPrimitiveComponent& Other);

	// 충돌에 따른 위치 / 속도 보정
	static void Translate(UBoxCollision& Box, const FVector2D& Delta);
	static void ResolveVelocity(URigidbody& Body, const FVector2D& Normal);

	// 도형 수집 / 개별 강체 시뮬레이션
	void GatherPrimitives(TArray<UPrimitiveComponent*>& Out) const;
	void SimulateBody(
	    URigidbody& Body, UBoxCollision& Box, float DeltaTime, const TArray<UPrimitiveComponent*>& Primitives);

	// 소유 월드 참조
	UWorld& m_World;

	// 월드 물리 설정
	float m_Gravity = 980.0f;
};
