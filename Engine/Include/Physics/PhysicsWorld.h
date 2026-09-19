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
	explicit FPhysicsWorld(UWorld& World);
	~FPhysicsWorld();
	void Tick(float DeltaTime);
	void SetGravity(float Gravity);
	float GetGravity() const;
	bool Raycast(const FVector2D& Start, const FVector2D& End, FHitResult& OutHit,
	    uint32 ObjectMask = AllCollisionChannels, const AActor* IgnoreActor = nullptr) const;
	void FindOverlaps(TArray<FOverlapResult>& OutOverlaps) const;

private:
	static bool IntersectBox(const FVector2D& Start, const FVector2D& Delta, const FRect& Bounds, float& Time,
	    FVector2D& Normal, bool bSweep);
	static bool IsBlocker(const UBoxCollision& Box, const UPrimitiveComponent& Other);
	static void Translate(UBoxCollision& Box, const FVector2D& Delta);
	static FRect Expanded(const FRect& Bounds, const FVector2D& Extent);
	static void ResolveVelocity(URigidbody& Body, const FVector2D& Normal);

	void GatherPrimitives(TArray<UPrimitiveComponent*>& Out) const;
	void SimulateBody(
	    URigidbody& Body, UBoxCollision& Box, float DeltaTime, const TArray<UPrimitiveComponent*>& Primitives);
	UWorld& m_World;
	float m_Gravity = 980.0f;
};
