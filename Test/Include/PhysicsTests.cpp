#include "EnginePCH.h"
#include "PhysicsTests.h"
#include "Physics/UBoxCollision.h"
#include "Physics/UCircleCollision.h"
#include "Physics/URigidbody.h"
#include "World/UWorld.h"

namespace
{
	void SetPosition(USceneComponent& Component, float X, float Y)
	{
		Component.SetRelativeTransform(FTransform2D(FVector2D(X, Y), 0.0f, FVector2D::One));
	}

	UBoxCollision* AddBox(UWorld& World, float X, float Y, float HalfX, float HalfY)
	{
		UBoxCollision* Box = World.SpawnActor<AActor>()->AddComponent<UBoxCollision>();
		Box->SetBoxExtent(FVector2D(HalfX, HalfY));
		SetPosition(*Box, X, Y);
		return Box;
	}

	URigidbody* AddBody(UBoxCollision& Box)
	{
		Box.SetCollisionObjectType(ECollisionChannel::Player);
		return Box.GetOwner()->AddComponent<URigidbody>();
	}

	bool Near(float A, float B)
	{
		return FMath::Abs(A - B) < 0.002f;
	}
}

// check/assert와 별개로 Debug와 Release 모두에서 모든 검사를 실행한다.
#define PHYSICS_CHECK(Expression) \
	do \
	{ \
		if (!(Expression)) \
		{ \
			++Failures; \
			wprintf(L"[Physics] FAILED at line %d\n", __LINE__); \
		} \
	} while (0)

int32 RunPhysicsTests()
{
	int32 Failures = 0;
	{
		UBoxCollision Box;
		Box.SetBoxExtent(FVector2D(5.0f, 5.0f));
		UCircleCollision Circle;
		Circle.SetSphereRadius(2.0f);
		SetPosition(Circle, 7.0f, 0.0f);
		PHYSICS_CHECK(Box.Overlaps(Circle)); // 경계에서 접하는 경우
		PHYSICS_CHECK(Circle.Overlaps(Box));
		SetPosition(Circle, 7.0f, 7.0f);
		PHYSICS_CHECK(!Box.Overlaps(Circle)); // 외접 사각형은 접하지만 실제 원과 Box는 겹치지 않음
		UCircleCollision Second;
		Second.SetSphereRadius(2.0f);
		SetPosition(Second, 11.0f, 7.0f);
		PHYSICS_CHECK(Circle.Overlaps(Second));
		SetPosition(Second, 11.1f, 7.0f);
		PHYSICS_CHECK(!Circle.Overlaps(Second));
		Box.SetCenterOffset(FVector2D(1.0f, 2.0f));
		Box.SetRelativeTransform(FTransform2D(FVector2D(10.0f, 20.0f), 0.0f, FVector2D(-2.0f, 3.0f)));
		const FRect Bounds = Box.GetWorldBounds();
		PHYSICS_CHECK(Near(Bounds.m_Left, -2.0f) && Near(Bounds.m_Bottom, 41.0f));
		Circle.SetCollisionMask(0);
		PHYSICS_CHECK(!Circle.Overlaps(Second));
	}
	{
		UWorld World;
		UBoxCollision* NearBox = AddBox(World, 20, 0, 5, 5);
		UBoxCollision* FarBox = AddBox(World, 50, 0, 5, 5);
		UCircleCollision* Circle = World.SpawnActor<AActor>()->AddComponent<UCircleCollision>();
		Circle->SetSphereRadius(5);
		Circle->SetCollisionObjectType(ECollisionChannel::Enemy);
		SetPosition(*Circle, 80, 0);
		FHitResult Hit;
		bool bHit = World.GetPhysicsWorld().Raycast(FVector2D::Zero, FVector2D(100, 0), Hit);
		PHYSICS_CHECK(bHit && Hit.m_pComponent == NearBox && Near(Hit.m_Time, 0.15f));
		PHYSICS_CHECK(Hit.m_Normal == FVector2D(-1, 0));
		bHit = World.GetPhysicsWorld().Raycast(
		    FVector2D::Zero, FVector2D(100, 0), Hit, AllCollisionChannels, NearBox->GetOwner());
		PHYSICS_CHECK(bHit && Hit.m_pComponent == FarBox && Near(Hit.m_Time, 0.45f));
		bHit = World.GetPhysicsWorld().Raycast(
		    FVector2D::Zero, FVector2D(100, 0), Hit, CollisionChannelMask(ECollisionChannel::Enemy));
		PHYSICS_CHECK(bHit && Hit.m_pComponent == Circle && Near(Hit.m_Time, 0.75f));
		bHit = World.GetPhysicsWorld().Raycast(FVector2D(80, 0), FVector2D(80, 0), Hit);
		PHYSICS_CHECK(bHit && Hit.m_bStartPenetrating && Hit.m_Time == 0.0f);
		bHit = World.GetPhysicsWorld().Raycast(FVector2D(0, 30), FVector2D(100, 30), Hit);
		PHYSICS_CHECK(!bHit && !Hit.m_bBlockingHit && Hit.m_pComponent == nullptr);
		SetPosition(*FarBox, 20, 0);
		TArray<FOverlapResult> Overlaps;
		World.GetPhysicsWorld().FindOverlaps(Overlaps);
		PHYSICS_CHECK(Overlaps.Num() == 1);
		FarBox->SetCollisionMask(0);
		World.GetPhysicsWorld().FindOverlaps(Overlaps);
		PHYSICS_CHECK(Overlaps.Num() == 0);
		World.DestroyActor(NearBox->GetOwner());
		FarBox->SetCollisionEnabled(false);
		bHit = World.GetPhysicsWorld().Raycast(FVector2D::Zero, FVector2D(100, 0), Hit);
		PHYSICS_CHECK(bHit && Hit.m_pComponent == Circle); // 액터 삭제 후 쿼리에 해제된 컴포넌트가 남지 않음
		Circle->GetOwner()->RemoveComponent(Circle);
		bHit = World.GetPhysicsWorld().Raycast(FVector2D::Zero, FVector2D(100, 0), Hit);
		PHYSICS_CHECK(!bHit);
	}
	{
		UWorld World;
		AddBox(World, 0, 100, 100, 2);
		UBoxCollision* Box = AddBox(World, 0, 0, 5, 5);
		URigidbody* Body = AddBody(*Box);
		USceneComponent* Visual = Box->GetOwner()->AddComponent<USceneComponent>();
		Visual->SetAttachParent(Box);
		Body->SetMaxFallSpeed(1000000);
		Body->SetVelocity(FVector2D(0, 1000000));
		World.Tick(0.5f);
		PHYSICS_CHECK(Near(Box->GetWorldCenter().m_Y, 93)); // 빠르게 낙하해도 얇은 발판을 관통하지 않음
		PHYSICS_CHECK(Body->IsGrounded() && Near(Body->GetVelocity().m_Y, 0));
		PHYSICS_CHECK(Visual->GetWorldTransform().m_Location == Box->GetWorldCenter());
		World.Tick(0.25f);
		PHYSICS_CHECK(Near(Box->GetWorldCenter().m_Y, 93) && Body->IsGrounded());
		Body->SetGravityScale(0);
		Body->SetVelocity(FVector2D(0, -200));
		World.Tick(0.1f);
		PHYSICS_CHECK(Near(Box->GetWorldCenter().m_Y, 73) && !Body->IsGrounded());
		Body->SetVelocity(FVector2D::Zero);
		SetPosition(*Box, 0, 96); // 발판과 일부 겹친 위치에 스폰한 상황
		World.Tick(0.01f);
		PHYSICS_CHECK(Near(Box->GetWorldCenter().m_Y, 93) && Body->IsGrounded());
		Body->SetVelocity(FVector2D(1000, 0));
		World.Tick(0.2f);
		PHYSICS_CHECK(Box->GetWorldCenter().m_X > 190 && !Body->IsGrounded());
		Body->SetGravityScale(1);
		Body->SetMaxFallSpeed(100);
		World.Tick(0.5f);
		PHYSICS_CHECK(Box->GetWorldCenter().m_Y > 93 && Near(Body->GetVelocity().m_Y, 100));
		const FVector2D Before = Box->GetWorldCenter();
		World.Tick(0);
		World.Tick(-1);
		PHYSICS_CHECK(Box->GetWorldCenter() == Before);
	}
	{
		UWorld World;
		World.GetPhysicsWorld().SetGravity(0);
		UBoxCollision* Wall = AddBox(World, 30, 0, 2, 100);
		UBoxCollision* Box = AddBox(World, 0, 0, 5, 5);
		URigidbody* Body = AddBody(*Box);
		Body->SetVelocity(FVector2D(10000, 20));
		World.Tick(0.1f);
		PHYSICS_CHECK(Near(Box->GetWorldCenter().m_X, 23));
		PHYSICS_CHECK(Near(Box->GetWorldCenter().m_Y, 2)); // 벽을 따라 수직 이동은 유지
		PHYSICS_CHECK(Near(Body->GetVelocity().m_X, 0) && !Body->IsGrounded());
		Body->SetVelocity(FVector2D(-100, 0));
		World.Tick(0.1f);
		PHYSICS_CHECK(Near(Box->GetWorldCenter().m_X, 13)); // 접촉면에서 멀어지는 이동은 허용
		Wall->SetCollisionMask(CollisionChannelMask(ECollisionChannel::Enemy));
		Body->SetVelocity(FVector2D(1000, 0));
		World.Tick(0.1f);
		PHYSICS_CHECK(Box->GetWorldCenter().m_X > 100); // 양쪽 충돌 마스크를 모두 반영
		AddBox(World, 0, -20, 100, 2);
		SetPosition(*Box, 0, 0);
		Body->SetVelocity(FVector2D(0, -1000));
		World.Tick(0.05f);
		PHYSICS_CHECK(Near(Box->GetWorldCenter().m_Y, -13));
		PHYSICS_CHECK(Near(Body->GetVelocity().m_Y, 0) && !Body->IsGrounded());
		Body->SetSimulatePhysics(false);
		Body->SetVelocity(FVector2D(100, 100));
		World.Tick(1);
		PHYSICS_CHECK(Near(Box->GetWorldCenter().m_Y, -13));
	}
	wprintf(L"[Physics] Phase 10 initial stage: %d failure(s)\n", Failures);
	return Failures;
}

#undef PHYSICS_CHECK
