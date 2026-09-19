#include "EnginePCH.h"
#include "Physics/PhysicsWorld.h"
#include "Physics/UBoxCollision.h"
#include "Physics/UCircleCollision.h"
#include "Physics/URigidbody.h"
#include "World/UWorld.h"
#include <float.h>

// 축별 구간 교차를 검사한다. Sweep은 도형 내부로 진입할 때만 충돌로 판단하므로,
// 접촉면에서 멀어지거나 평행하게 이동하는 Box가 표면에 걸리지 않는다.
bool FPhysicsWorld::IntersectBox(
    const FVector2D& Start, const FVector2D& Delta, const FRect& Bounds, float& Time, FVector2D& Normal, bool bSweep)
{
	float Enter = -FLT_MAX, Exit = FLT_MAX;
	Normal = FVector2D::Zero;
	for (int32 Axis = 0; Axis < 2; Axis++)
	{
		const float Position = Axis == 0 ? Start.m_X : Start.m_Y;
		const float Direction = Axis == 0 ? Delta.m_X : Delta.m_Y;
		const float Minimum = Axis == 0 ? Bounds.m_Left : Bounds.m_Top;
		const float Maximum = Axis == 0 ? Bounds.m_Right : Bounds.m_Bottom;
		if (FMath::Abs(Direction) < FMath::SMALL_NUMBER)
		{
			if (Position < Minimum || Position > Maximum)
			{
				return false;
			}
			if (bSweep && (Position <= Minimum || Position >= Maximum))
			{
				return false;
			}
			continue;
		}
		float Near = (Minimum - Position) / Direction;
		float Far = (Maximum - Position) / Direction;
		const float Sign = Direction > 0.0f ? -1.0f : 1.0f;
		if (Near > Far)
		{
			const float Swap = Near;
			Near = Far;
			Far = Swap;
		}
		if (Near > Enter)
		{
			Enter = Near;
			Normal = Axis == 0 ? FVector2D(Sign, 0.0f) : FVector2D(0.0f, Sign);
		}
		Exit = FMath::Min(Exit, Far);
		if (Enter > Exit)
		{
			return false;
		}
	}
	if (Exit < 0.0f || Enter > 1.0f)
	{
		return false;
	}
	if (bSweep && (Enter < -FMath::SMALL_NUMBER || Enter >= Exit))
	{
		return false;
	}
	Time = FMath::Max(0.0f, Enter);
	return true;
}

bool FPhysicsWorld::IsBlocker(const UBoxCollision& Box, const UPrimitiveComponent& Other)
{
	if (&Box == &Other || Box.GetOwner() == Other.GetOwner() || !Box.CanCollideWith(Other)
	    || Other.GetShapeType() != ECollisionShape::Box || Other.GetCollisionObjectType() == ECollisionChannel::Trigger)
	{
		return false;
	}
	const URigidbody* Body = Other.GetOwner()->GetComponent<URigidbody>();
	return !Body || !Body->IsSimulatingPhysics();
}

void FPhysicsWorld::Translate(UBoxCollision& Box, const FVector2D& Delta)
{
	FTransform2D Transform = Box.GetRelativeTransform();
	Transform.m_Location += Delta;
	Box.SetRelativeTransform(Transform);
}

FRect FPhysicsWorld::Expanded(const FRect& Bounds, const FVector2D& Extent)
{
	return FRect(Bounds.m_Left - Extent.m_X, Bounds.m_Top - Extent.m_Y, Bounds.m_Right + Extent.m_X,
	    Bounds.m_Bottom + Extent.m_Y);
}

FPhysicsWorld::FPhysicsWorld(UWorld& World) : m_World(World)
{
}

FPhysicsWorld::~FPhysicsWorld() = default;

void FPhysicsWorld::SetGravity(float Gravity)
{
	m_Gravity = FMath::Max(0.0f, Gravity);
}

float FPhysicsWorld::GetGravity() const
{
	return m_Gravity;
}

void FPhysicsWorld::GatherPrimitives(TArray<UPrimitiveComponent*>& Out) const
{
	Out.Reset();
	const TArray<AActor*>& Actors = m_World.GetActors();
	for (int32 i = 0; i < Actors.Num(); i++)
	{
		Actors[i]->GetComponents<UPrimitiveComponent>(Out);
	}
}

void FPhysicsWorld::Tick(float DeltaTime)
{
	if (!(DeltaTime > 0.0f) || !_finite(DeltaTime))
	{
		return;
	}
	TArray<UPrimitiveComponent*> Primitives;
	GatherPrimitives(Primitives);
	// 경과 시간을 버리지 않으면서 서브스텝 횟수를 제한한다.
	// 빠른 이동의 관통은 서브스텝 크기에만 의존하지 않고 Sweep으로 방지한다.
	const int32 Steps = static_cast<int32>(FMath::Clamp(FMath::Ceil(DeltaTime * 120.0f), 1.0f, 120.0f));
	const float StepTime = DeltaTime / Steps;
	const TArray<AActor*>& Actors = m_World.GetActors();
	for (int32 Step = 0; Step < Steps; Step++)
	{
		for (int32 i = 0; i < Actors.Num(); i++)
		{
			URigidbody* Body = Actors[i]->GetComponent<URigidbody>();
			if (!Body || !Body->IsSimulatingPhysics())
			{
				continue;
			}
			UBoxCollision* Box = Actors[i]->GetComponent<UBoxCollision>();
			if (!Box || Box->GetAttachParent() || !Box->IsCollisionEnabled())
			{
				Body->m_bIsGrounded = false;
				continue;
			}
			SimulateBody(*Body, *Box, StepTime, Primitives);
		}
	}
}

void FPhysicsWorld::ResolveVelocity(URigidbody& Body, const FVector2D& Normal)
{
	const float IntoSurface = Body.m_Velocity.Dot(Normal);
	if (IntoSurface < 0.0f)
	{
		Body.m_Velocity -= Normal * IntoSurface;
	}
	if (Normal.m_Y < -0.5f)
	{
		Body.m_bIsGrounded = true;
	}
}

void FPhysicsWorld::SimulateBody(
    URigidbody& Body, UBoxCollision& Box, float DeltaTime, const TArray<UPrimitiveComponent*>& Primitives)
{
	Body.m_bIsGrounded = false;
	Body.m_Velocity.m_Y =
	    FMath::Min(Body.m_MaxFallSpeed, Body.m_Velocity.m_Y + m_Gravity * Body.m_GravityScale * DeltaTime);

	// 스폰이나 순간 이동으로 생긴 초기 겹침을 제한된 횟수의 최소 이동으로 해소한다.
	for (int32 Pass = 0; Pass < 8; Pass++)
	{
		bool bMoved = false;
		for (int32 i = 0; i < Primitives.Num(); i++)
		{
			if (!IsBlocker(Box, *Primitives[i]))
			{
				continue;
			}
			const FRect A = Box.GetWorldBounds(), B = Primitives[i]->GetWorldBounds();
			if (A.m_Right <= B.m_Left || A.m_Left >= B.m_Right || A.m_Bottom <= B.m_Top || A.m_Top >= B.m_Bottom)
			{
				continue;
			}
			FVector2D Correction(B.m_Left - A.m_Right, 0.0f);
			const FVector2D Candidates[] = {FVector2D(B.m_Right - A.m_Left, 0.0f),
			    FVector2D(0.0f, B.m_Top - A.m_Bottom), FVector2D(0.0f, B.m_Bottom - A.m_Top)};
			for (const FVector2D& Candidate : Candidates)
			{
				if (Candidate.SizeSquared() < Correction.SizeSquared())
				{
					Correction = Candidate;
				}
			}
			Translate(Box, Correction);
			ResolveVelocity(Body, Correction.GetNormalized());
			bMoved = true;
		}
		if (!bMoved)
		{
			break;
		}
	}

	float RemainingTime = DeltaTime;
	for (int32 Pass = 0; Pass < 4 && RemainingTime > 0.0f; Pass++)
	{
		const FVector2D Delta = Body.m_Velocity * RemainingTime;
		if (Delta.IsNearlyZero(FMath::SMALL_NUMBER))
		{
			break;
		}
		float Earliest = 1.0f;
		FVector2D Normal;
		bool bHit = false;
		for (int32 i = 0; i < Primitives.Num(); i++)
		{
			if (!IsBlocker(Box, *Primitives[i]))
			{
				continue;
			}
			float Time;
			FVector2D CandidateNormal;
			if (IntersectBox(Box.GetWorldCenter(), Delta,
			        Expanded(Primitives[i]->GetWorldBounds(), Box.GetScaledBoxExtent()), Time, CandidateNormal, true)
			    && Time <= Earliest)
			{
				Earliest = Time;
				Normal = CandidateNormal;
				bHit = true;
			}
		}
		Translate(Box, Delta * Earliest);
		if (!bHit)
		{
			break;
		}
		ResolveVelocity(Body, Normal);
		RemainingTime *= 1.0f - Earliest;
	}

	// 중력이 0이어도 정지 접촉을 확인한다. 위로 이동 중이면 지면에 서 있는 상태가 아니다.
	Body.m_bIsGrounded = false;
	if (Body.m_Velocity.m_Y >= 0.0f)
	{
		const FRect A = Box.GetWorldBounds();
		for (int32 i = 0; i < Primitives.Num(); i++)
		{
			if (!IsBlocker(Box, *Primitives[i]))
			{
				continue;
			}
			const FRect B = Primitives[i]->GetWorldBounds();
			if (A.m_Right > B.m_Left && A.m_Left < B.m_Right && FMath::Abs(A.m_Bottom - B.m_Top) <= 0.001f)
			{
				Body.m_bIsGrounded = true;
				break;
			}
		}
	}
}

bool FPhysicsWorld::Raycast(const FVector2D& Start, const FVector2D& End, FHitResult& OutHit, uint32 ObjectMask,
    const AActor* IgnoreActor) const
{
	OutHit = FHitResult();
	TArray<UPrimitiveComponent*> Primitives;
	GatherPrimitives(Primitives);
	const FVector2D Delta = End - Start;
	for (int32 i = 0; i < Primitives.Num(); i++)
	{
		UPrimitiveComponent& Shape = *Primitives[i];
		if (!Shape.IsCollisionEnabled() || Shape.GetOwner() == IgnoreActor
		    || !(ObjectMask & CollisionChannelMask(Shape.GetCollisionObjectType())))
		{
			continue;
		}
		float Time = 0.0f;
		FVector2D Normal;
		bool bInside;
		if (Shape.GetShapeType() == ECollisionShape::Box)
		{
			const FRect Bounds = Shape.GetWorldBounds();
			bInside = Start.m_X > Bounds.m_Left && Start.m_X < Bounds.m_Right && Start.m_Y > Bounds.m_Top
			          && Start.m_Y < Bounds.m_Bottom;
			if (!IntersectBox(Start, Delta, Bounds, Time, Normal, false))
			{
				continue;
			}
		}
		else
		{
			const float Radius = static_cast<const UCircleCollision&>(Shape).GetWorldRadius();
			const FVector2D Offset = Start - Shape.GetWorldCenter();
			const float C = Offset.SizeSquared() - Radius * Radius;
			bInside = C < 0.0f;
			if (C > 0.0f)
			{
				const float A = Delta.SizeSquared(), B = Offset.Dot(Delta);
				const float Discriminant = B * B - A * C;
				if (A <= FMath::SMALL_NUMBER || Discriminant < 0.0f)
				{
					continue;
				}
				Time = (-B - FMath::Sqrt(Discriminant)) / A;
				if (Time < 0.0f || Time > 1.0f)
				{
					continue;
				}
			}
			Normal = (Start + Delta * Time - Shape.GetWorldCenter()).GetNormalized();
		}
		if (bInside)
		{
			Normal = FVector2D::Zero;
		}
		if (Time > OutHit.m_Time)
		{
			continue;
		}
		OutHit.m_bBlockingHit = true;
		OutHit.m_bStartPenetrating = bInside;
		OutHit.m_Time = Time;
		OutHit.m_Normal = Normal;
		OutHit.m_Point = Start + Delta * Time;
		OutHit.m_pComponent = &Shape;
	}
	return OutHit.m_bBlockingHit;
}

void FPhysicsWorld::FindOverlaps(TArray<FOverlapResult>& OutOverlaps) const
{
	OutOverlaps.Reset();
	TArray<UPrimitiveComponent*> Primitives;
	GatherPrimitives(Primitives);
	for (int32 i = 0; i < Primitives.Num(); i++)
	{
		for (int32 j = i + 1; j < Primitives.Num(); j++)
		{
			if (Primitives[i]->GetOwner() != Primitives[j]->GetOwner() && Primitives[i]->Overlaps(*Primitives[j]))
			{
				OutOverlaps.Emplace(Primitives[i], Primitives[j]);
			}
		}
	}
}
