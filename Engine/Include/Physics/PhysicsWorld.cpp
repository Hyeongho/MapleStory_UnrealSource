#include "EnginePCH.h"
#include "Physics/PhysicsWorld.h"
#include "Physics/UBoxCollision.h"
#include "Physics/UCircleCollision.h"
#include "Physics/UClimbableComponent.h"
#include "Physics/URigidbody.h"
#include "World/UWorld.h"

#ifdef _DEBUG
#include "Render/SpriteBatch.h"
#include "Render/FCamera2D.h"
#endif

bool FPhysicsWorld::IsBlocker(const UBoxCollision& Box, const UPrimitiveComponent& Other)
{
	if (&Box == &Other || Box.GetOwner() == Other.GetOwner() || !Box.CanCollideWith(Other) || Other.GetShapeType() != ECollisionShape::Box || Other.GetCollisionObjectType() == ECollisionChannel::Trigger)
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

bool FPhysicsWorld::AddFoothold(const FFoothold& Foothold)
{
	if (!Foothold.IsValid() || FindFoothold(Foothold.GetId()))
	{
		UE_LOG(LogPhysics, Warning, L"AddFoothold: invalid segment or duplicate ID (%d)", Foothold.GetId());
		return false;
	}

	m_Footholds.Add(Foothold);
	return true;
}

bool FPhysicsWorld::RemoveFoothold(int32 Id)
{
	for (int32 i = 0; i < m_Footholds.Num(); i++)
	{
		if (m_Footholds[i].GetId() == Id)
		{
			m_Footholds.RemoveAtSwap(i);
			InvalidateFootholdSupport(Id);
			return true;
		}
	}

	return false;
}

void FPhysicsWorld::ClearFootholds()
{
	m_Footholds.Empty();
	m_VerticalFootholds.Empty();
	InvalidateFootholdSupport(INDEX_NONE);
}

bool FPhysicsWorld::AddVerticalFoothold(int32 Id, int32 Layer, int32 Group, const FVector2D& Start, const FVector2D& End, uint32 CollisionMask)
{
	if (Id < 0 || Layer < 0 || Group < 0 || !_finite(Start.m_X) || !_finite(Start.m_Y)
		|| !_finite(End.m_X) || !_finite(End.m_Y) || FMath::Abs(Start.m_X - End.m_X) > FMath::KINDA_SMALL_NUMBER
		|| FMath::Abs(Start.m_Y - End.m_Y) <= FMath::KINDA_SMALL_NUMBER || FindFoothold(Id))
	{
		return false;
	}

	for (int32 i = 0; i < m_VerticalFootholds.Num(); i++)
	{
		if (m_VerticalFootholds[i].m_Id == Id)
		{
			return false;
		}
	}

	FVerticalFoothold Foothold;

	Foothold.m_Id = Id;
	Foothold.m_Layer = Layer;
	Foothold.m_Group = Group;
	Foothold.m_Start = Start;
	Foothold.m_End = End;
	Foothold.m_CollisionMask = CollisionMask & AllCollisionChannels;

	m_VerticalFootholds.Add(Foothold);

	return true;
}

bool FPhysicsWorld::RemoveVerticalFoothold(int32 Id)
{
	for (int32 i = 0; i < m_VerticalFootholds.Num(); i++)
	{
		if (m_VerticalFootholds[i].m_Id == Id)
		{
			m_VerticalFootholds.RemoveAtSwap(i);

			return true;
		}
	}

	return false;

}

const FFoothold* FPhysicsWorld::FindFoothold(int32 Id) const
{
	for (int32 i = 0; i < m_Footholds.Num(); i++)
	{
		if (m_Footholds[i].GetId() == Id)
		{
			return &m_Footholds[i];
		}
	}

	return nullptr;
}

void FPhysicsWorld::InvalidateFootholdSupport(int32 Id)
{
	const TArray<AActor*>& Actors = m_World.GetActors();

	for (int32 i = 0; i < Actors.Num(); i++)
	{
		URigidbody* Body = Actors[i]->GetComponent<URigidbody>();

		if (!Body)
		{
			continue;
		}

		if (Body->m_CurrentFootholdId != INDEX_NONE && (Id == INDEX_NONE || Body->m_CurrentFootholdId == Id))
		{
			Body->m_CurrentFootholdId = INDEX_NONE;
			Body->m_bIsGrounded = false;
			Body->m_bDropThroughRequested = false;
		}

		if (Id == INDEX_NONE || Body->m_IgnoredFootholdId == Id)
		{
			Body->m_IgnoredFootholdId = INDEX_NONE;
		}

		if (Id == INDEX_NONE)
		{
			// 맵 발판을 전부 비울 때 이전 맵의 점프 허용 시간을 남기지 않는다.
			Body->m_CoyoteTimeRemaining = 0.0f;
			Body->m_bDropThroughRequested = false;
		}
	}
}

const UClimbableComponent* FPhysicsWorld::FindClimbable(const UBoxCollision& Box, const URigidbody& Body, const TArray<UPrimitiveComponent*>& Primitives)
{
	if (!Body.m_bIsClimbing && Body.m_ClimbReentryRemaining > 0.0f)
	{
		return nullptr;
	}

	const FVector2D Foot = GetFootPosition(Box);
	const UClimbableComponent* Result = nullptr;
	float NearestX = FLT_MAX;

	for (int32 i = 0; i < Primitives.Num(); i++)
	{
		const UClimbableComponent* Climbable = Cast<UClimbableComponent>(Primitives[i]);
		if (!Climbable || Climbable->GetOwner() == Box.GetOwner() || Climbable->GetCollisionObjectType() != ECollisionChannel::Trigger || !Box.CanCollideWith(*Climbable))
		{
			continue;
		}

		const FRect Bounds = Climbable->GetWorldBounds();

		// 상단 발판이 끝점보다 조금 위에 있어도 아래 입력으로 사다리에 다시 진입할 수 있다.
		// 외접 AABB 대신 실제 회전 도형과 발 사이의 거리를 사용한다.
		const bool bTouchingTop = (Body.m_bIsClimbing || Body.m_ClimbInput > FMath::SMALL_NUMBER) && FMath::Abs(Foot.m_Y - Bounds.m_Top) <= CLIMB_ENDPOINT_TOLERANCE && FVector2D::DistanceSquared(Foot, Climbable->GetWorldBox().GetClosestPoint(Foot)) <= CLIMB_ENDPOINT_TOLERANCE * CLIMB_ENDPOINT_TOLERANCE;

		if (!Box.Overlaps(*Climbable) && !bTouchingTop)
		{
			continue;
		}

		// 하단은 몸이 겹치면 잡을 수 있다. 발이 아래에 있다는 이유로 진입을 막지 않는다.
		if (Foot.m_Y < Bounds.m_Top - CLIMB_ENDPOINT_TOLERANCE)
		{
			continue;
		}

		if (Body.m_bIsClimbing)
		{
			if (Climbable->GetOwner()->GetActorId() == Body.m_ClimbableActorId)
			{
				return Climbable;
			}

			continue;
		}

		// 끝에서 같은 방향을 계속 눌러도 다시 매달리지 않는다. 반대 방향은 진입 가능하다.
		if (FMath::Abs(Body.m_ClimbInput) <= FMath::SMALL_NUMBER || (Body.m_ClimbInput < 0.0f && Foot.m_Y <= Bounds.m_Top + CLIMB_ENDPOINT_TOLERANCE) || (Body.m_ClimbInput > 0.0f && Foot.m_Y >= Bounds.m_Bottom - CLIMB_ENDPOINT_TOLERANCE))
		{
			continue;
		}

		const float DistanceX = FMath::Abs(Box.GetWorldCenter().m_X - Climbable->GetWorldCenter().m_X);

		if (DistanceX < NearestX)
		{
			NearestX = DistanceX;
			Result = Climbable;
		}
	}

	return Result;
}

bool FPhysicsWorld::CanTranslateBox(const UBoxCollision& Box, const FVector2D& Delta, const TArray<UPrimitiveComponent*>& Primitives)
{
	const FOrientedBox2D Target(Box.GetWorldCenter() + Delta, Box.GetScaledBoxExtent(), Box.GetWorldTransform().m_Rotation);
	for (int32 i = 0; i < Primitives.Num(); i++)
	{
		if (!IsBlocker(Box, *Primitives[i]))
		{
			continue;
		}

		const FOrientedBox2D Other = static_cast<const UBoxCollision*>(Primitives[i])->GetWorldBox();

		float Time, Depth;

		FVector2D Normal;

		if ((Box.GetWorldBox().Sweep(Other, Delta, Time, Normal) && Time < 1.0f - FMath::KINDA_SMALL_NUMBER) || (Target.FindContact(Other, Normal, Depth) && Depth > FMath::KINDA_SMALL_NUMBER))
		{
			return false;
		}
	}

	return true;
}

bool FPhysicsWorld::HasClimbLandingBelow(const UBoxCollision& Box, float BottomY, const TArray<UPrimitiveComponent*>& Primitives) const
{
	const FVector2D Foot = GetFootPosition(Box);
	for (int32 i = 0; i < m_Footholds.Num(); i++)
	{
		const FFoothold& Foothold = m_Footholds[i];
		if (CanUseFoothold(Box, Foothold) && Foothold.GetHeightAtX(Foot.m_X) > BottomY + CLIMB_ENDPOINT_TOLERANCE && Foothold.GetHeightAtX(Foot.m_X) >= Foot.m_Y - 0.001f)
		{
			return true;
		}
	}

	// 발판까지의 거리를 하단 점프 제한과 묶지 않는다. 실제 맵 안의 바닥 존재 여부를 확인한다.
	// // 하단 아래에서 잡은 경우에는 현재 발보다 위에 있는 바닥을 낙하 착지면으로 세지 않는다.
	const FVector2D Start(Foot.m_X, FMath::Max(BottomY + CLIMB_ENDPOINT_TOLERANCE, Foot.m_Y - 0.001f));
	for (int32 i = 0; i < Primitives.Num(); i++)
	{
		if (!IsBlocker(Box, *Primitives[i]))
		{
			continue;
		}

		const UBoxCollision& Floor = static_cast<const UBoxCollision&>(*Primitives[i]);
		const float Bottom = Floor.GetWorldBounds().m_Bottom;
		if (Bottom <= Start.m_Y)
		{
			continue;
		}

		float Time;
		FVector2D Normal;
		bool bInside;
		if (Floor.GetWorldBox().Raycast(Start, FVector2D(Foot.m_X, Bottom + CLIMB_ENDPOINT_TOLERANCE), Time, Normal, bInside) && !bInside && Normal.m_Y < -0.5f)
		{
			return true;
		}
	}

	return false;
}

bool FPhysicsWorld::TryExitClimbable(URigidbody& Body, UBoxCollision& Box, const UClimbableComponent& Climbable, const TArray<UPrimitiveComponent*>& Primitives)
{
	const bool bGoingUp = Body.m_ClimbInput < -FMath::SMALL_NUMBER;
	const bool bGoingDown = Body.m_ClimbInput > FMath::SMALL_NUMBER;

	const FRect Bounds = Climbable.GetWorldBounds();
	const FVector2D Foot = GetFootPosition(Box);

	const float EndpointY = bGoingUp ? Bounds.m_Top : Bounds.m_Bottom;

	if ((!bGoingUp && !bGoingDown) || (bGoingUp && !Climbable.CanExitAtTop()) || (bGoingUp && FMath::Abs(Foot.m_Y - EndpointY) > CLIMB_ENDPOINT_TOLERANCE) || (bGoingDown && Foot.m_Y < EndpointY - CLIMB_ENDPOINT_TOLERANCE))
	{
		return false;
	}

	float LandingY = EndpointY;
	float NearestDistance = CLIMB_ENDPOINT_TOLERANCE + FMath::KINDA_SMALL_NUMBER;
	int32 FootholdId = INDEX_NONE;
	bool bFound = false;

	for (int32 i = 0; i < m_Footholds.Num(); i++)
	{
		const FFoothold& Foothold = m_Footholds[i];

		if (!CanUseFoothold(Box, Foothold) || !Foothold.ContainsX(Foot.m_X))
		{
			continue;
		}

		const float Height = Foothold.GetHeightAtX(Foot.m_X);

		// 하단 아래에서 내려오려 할 때 위쪽 발판으로 순간 이동하지 않는다.
		if (bGoingDown && Height < Foot.m_Y - CLIMB_ENDPOINT_TOLERANCE)
		{
			continue;
		}

		const float Distance = FMath::Abs(Height - EndpointY);

		if (Distance <= NearestDistance && CanTranslateBox(Box, FVector2D(0.0f, Height - Foot.m_Y), Primitives))
		{
			bFound = true;
			NearestDistance = Distance;
			LandingY = Height;
			FootholdId = Foothold.GetId();
		}
	}

	// 일반 정적 Box 위에서도 하단 이탈 후 서 있을 수 있다. 내부·옆면은 지지면으로 쓰지 않는다.
	const FVector2D Start(Foot.m_X, EndpointY - CLIMB_ENDPOINT_TOLERANCE);
	const FVector2D End(Foot.m_X, EndpointY + CLIMB_ENDPOINT_TOLERANCE);
	for (int32 i = 0; i < Primitives.Num(); i++)
	{
		if (!IsBlocker(Box, *Primitives[i]))
		{
			continue;
		}

		float Time;
		FVector2D Normal;
		bool bInside;

		if (!static_cast<const UBoxCollision*>(Primitives[i])->GetWorldBox().Raycast(Start, End, Time, Normal, bInside) || bInside || Normal.m_Y >= -0.5f)
		{
			continue;
		}

		const float Height = Start.m_Y + (End.m_Y - Start.m_Y) * Time;

		if (bGoingDown && Height < Foot.m_Y - CLIMB_ENDPOINT_TOLERANCE)
		{
			continue;
		}

		const float Distance = FMath::Abs(Height - EndpointY);

		if (Distance < NearestDistance && CanTranslateBox(Box, FVector2D(0.0f, Height - Foot.m_Y), Primitives))
		{
			bFound = true;
			NearestDistance = Distance;
			LandingY = Height;
			FootholdId = INDEX_NONE;
		}
	}

	if (!bFound)
	{
		// 하단이 공중에서 끊어져 있어도 아래에 바닥이 있으면 사다리를 놓고 자연 낙하한다.
		// 하단 아래에서 잡은 상태도 아래 입력으로 놓을 수 있다. 바닥이 없으면 매달린 상태를 유지한다.
		if (!bGoingDown || Foot.m_Y < Bounds.m_Bottom - FMath::KINDA_SMALL_NUMBER || !HasClimbLandingBelow(Box, Bounds.m_Bottom, Primitives))
		{
			return false;
		}

		Body.StopClimbing();
		Body.m_bIsGrounded = false;
		Body.m_CurrentFootholdId = INDEX_NONE;
		Body.m_LastFootholdId = INDEX_NONE;
		Body.m_IgnoredFootholdId = INDEX_NONE;
		Body.m_bDropThroughRequested = false;

		return true;
	}

	Translate(Box, FVector2D(0.0f, LandingY - Foot.m_Y));

	Body.StopClimbing();
	Body.m_bIsGrounded = true;
	Body.m_CurrentFootholdId = FootholdId;
	Body.m_LastFootholdId = FootholdId;
	Body.m_IgnoredFootholdId = INDEX_NONE;
	Body.m_bDropThroughRequested = false;

	return true;
}

FVector2D FPhysicsWorld::GetFootPosition(const UBoxCollision& Box)
{
	return Box.GetWorldBox().GetSupportPoint(FVector2D(0, 1));
}

bool FPhysicsWorld::CanUseFoothold(const UBoxCollision& Box, const FFoothold& Foothold)
{
	return Box.IsCollisionEnabled() && (Box.GetCollisionMask() & CollisionChannelMask(ECollisionChannel::WorldStatic)) && (Foothold.GetCollisionMask() & CollisionChannelMask(Box.GetCollisionObjectType()));
}

bool FPhysicsWorld::SweepFoothold(const FFoothold& Foothold, const FVector2D& Start, const FVector2D& Delta, float& OutTime)
{
	// 위로 점프하거나 발판 아래에서 접근하면 통과한다.
	const float Distance = Start.m_Y - Foothold.GetHeightAtX(Start.m_X);
	const float Approach = Delta.m_Y - Foothold.GetSlope() * Delta.m_X;

	if (Delta.m_Y < 0.0f || Distance > 0.001f || Approach <= FMath::SMALL_NUMBER)
	{
		return false;
	}

	OutTime = FMath::Max(0.0f, -Distance / Approach);

	if (OutTime > 1.0f)
	{
		return false;
	}

	const float HitX = Start.m_X + Delta.m_X * OutTime;

	// 끝점에서 선분 바깥으로 나가는 접촉은 막지 않는다.
	// 연결된 다음 경사면으로 이동할 때 이전 선분에 다시 걸리는 것을 방지한다.
	if ((Delta.m_X > 0.0f && HitX >= Foothold.GetMaxX()) || (Delta.m_X < 0.0f && HitX <= Foothold.GetMinX()))
	{
		return false;
	}

	return Foothold.ContainsX(HitX);
}

const FFoothold* FPhysicsWorld::FindWallGroupFoothold(const UBoxCollision& Box, const FFoothold* Support, int32 LastFootholdId) const
{
	if (Support && Support->GetLayer() != INDEX_NONE && Support->GetGroup() != INDEX_NONE)
	{
		return Support;
	}

	const FVector2D Foot = GetFootPosition(Box);
	const FFoothold* Previous = FindFoothold(LastFootholdId);
	if (Previous && Previous->GetLayer() != INDEX_NONE && Previous->GetGroup() != INDEX_NONE)
	{
		// 점프 중에도 이전 그룹 안에 발밑 발판이 있으면 그 그룹을 유지한다.
		for (int32 i = 0; i < m_Footholds.Num(); i++)
		{
			const FFoothold& Foothold = m_Footholds[i];
			if (Foothold.GetLayer() == Previous->GetLayer() && Foothold.GetGroup() == Previous->GetGroup()
				&& CanUseFoothold(Box, Foothold) && Foothold.ContainsX(Foot.m_X)
				&& Foothold.GetHeightAtX(Foot.m_X) >= Foot.m_Y)
			{
				return &Foothold;
			}
		}
	}

	// 이전 그룹에서 벗어났으면 발밑에서 가장 가까운 그룹을 고른다.
	// 발밑에 없다면 위쪽에서 가장 가까운 그룹을 사용한다.
	const FFoothold* Below = nullptr;
	const FFoothold* Above = nullptr;
	float BelowY = FLT_MAX;
	float AboveY = -FLT_MAX;

	for (int32 i = 0; i < m_Footholds.Num(); i++)
	{
		const FFoothold& Foothold = m_Footholds[i];
		if (Foothold.GetLayer() == INDEX_NONE || Foothold.GetGroup() == INDEX_NONE || !CanUseFoothold(Box, Foothold) || !Foothold.ContainsX(Foot.m_X))
		{
			continue;
		}

		const float Y = Foothold.GetHeightAtX(Foot.m_X);

		if (Y >= Foot.m_Y && Y < BelowY)
		{
			Below = &Foothold;
			BelowY = Y;
		}

		else if (Y <= Foot.m_Y && Y > AboveY)
		{
			Above = &Foothold;
			AboveY = Y;
		}
	}

	return Below ? Below : Above;
}

const FFoothold* FPhysicsWorld::FindSupportingFoothold(const UBoxCollision& Box, float DirectionX, int32 IgnoredFootholdId) const
{
	const FVector2D Foot = GetFootPosition(Box);
	const FFoothold* Result = nullptr;

	for (int32 i = 0; i < m_Footholds.Num(); i++)
	{
		const FFoothold& Foothold = m_Footholds[i];
		if (Foothold.GetId() == IgnoredFootholdId || !CanUseFoothold(Box, Foothold) || !Foothold.ContainsX(Foot.m_X) || FMath::Abs(Foot.m_Y - Foothold.GetHeightAtX(Foot.m_X)) > 0.001f)
		{
			continue;
		}

		// 끝점에 도달하면 이동 방향 쪽으로 이어진 선분을 선택한다.
		if ((DirectionX > 0.0f && Foothold.GetMaxX() - Foot.m_X <= FMath::SMALL_NUMBER) || (DirectionX < 0.0f && Foot.m_X - Foothold.GetMinX() <= FMath::SMALL_NUMBER))
		{
			continue;
		}

		if (!Result || Foothold.GetId() < Result->GetId())
		{
			Result = &Foothold;
		}
	}

	return Result;
}

const FFoothold* FPhysicsWorld::FindDropLandingFoothold(const UBoxCollision& Box, int32 CurrentFootholdId, float MaxDropHeight) const
{
	if (!_finite(MaxDropHeight) || MaxDropHeight <= 1.0f)
	{
		return nullptr;
	}

	const FVector2D Foot = GetFootPosition(Box);
	const FFoothold* Current = FindFoothold(CurrentFootholdId);
	if (!Current || !CanUseFoothold(Box, *Current) || !Current->ContainsX(Foot.m_X) || FMath::Abs(Foot.m_Y - Current->GetHeightAtX(Foot.m_X)) > 0.001f)
	{
		return nullptr;
	}

	const FFoothold* Result = nullptr;
	float NearestHeight = MaxDropHeight;
	for (int32 i = 0; i < m_Footholds.Num(); i++)
	{
		const FFoothold& Foothold = m_Footholds[i];
		if (Foothold.GetId() == CurrentFootholdId || !CanUseFoothold(Box, Foothold) || !Foothold.ContainsX(Foot.m_X))
		{
			continue;
		}

		const float DropHeight = Foothold.GetHeightAtX(Foot.m_X) - Foot.m_Y;
		if (DropHeight > 1.0f && DropHeight <= NearestHeight)
		{
			Result = &Foothold;
			NearestHeight = DropHeight;
		}
	}
	return Result;
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
				Body->StopClimbing();

				Body->m_bIsGrounded = false;
				Body->m_CoyoteTimeRemaining = 0.0f;
				Body->m_CurrentFootholdId = INDEX_NONE;
				Body->m_LastFootholdId = INDEX_NONE;
				Body->m_IgnoredFootholdId = INDEX_NONE;
				Body->m_bDropThroughRequested = false;

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
		// 고속 정면 충돌에서 큰 수끼리 빼며 잔여 법선 속도가 생기지 않도록 접선에 투영한다.
		const FVector2D Tangent(-Normal.m_Y, Normal.m_X);
		Body.m_Velocity = Tangent * Body.m_Velocity.Dot(Tangent);
	}

	if (Normal.m_Y < -0.5f)
	{
		Body.m_bIsGrounded = true;
	}
}

void FPhysicsWorld::SimulateBody(URigidbody& Body, UBoxCollision& Box, float DeltaTime, const TArray<UPrimitiveComponent*>& Primitives)
{
	// 상하 입력을 유지한 채 점프해도 재진입 지연이 끝날 때까지 중력 이동을 유지한다.
	Body.m_ClimbReentryRemaining = FMath::Max(0.0f, Body.m_ClimbReentryRemaining - DeltaTime);

	// 직전 서브스텝의 실제 접지를 기준으로 허용 시간을 갱신한다. 접지 상태를 꾸미지 않는다.
	Body.m_CoyoteTimeRemaining = Body.m_bIsGrounded ? Body.m_CoyoteTime : FMath::Max(0.0f, Body.m_CoyoteTimeRemaining - DeltaTime);

	if (Body.m_bDropThroughRequested)
	{
		Body.m_bDropThroughRequested = false;

		if (Body.m_bIsGrounded && !Body.m_bIsClimbing && FindDropLandingFoothold(Box, Body.m_CurrentFootholdId, Body.m_MaxDropHeight))
		{
			// 아래에 도달 가능한 발판이 있을 때만 현재 발판을 통과한다.
			Body.m_IgnoredFootholdId = Body.m_CurrentFootholdId;
			Body.m_CoyoteTimeRemaining = 0.0f;
			Body.m_Velocity.m_Y = FMath::Max(Body.m_Velocity.m_Y, 100.0f);
		}
	}

	Body.m_bIsGrounded = false;
	Body.m_CurrentFootholdId = INDEX_NONE;
	if (Body.m_IgnoredFootholdId != INDEX_NONE)
	{
		// 발이 원래 발판 아래로 내려가면 다시 충돌할 수 있게 한다.
		const FFoothold* Ignored = FindFoothold(Body.m_IgnoredFootholdId);
		const FVector2D Foot = GetFootPosition(Box);

		if (!Ignored || !Ignored->ContainsX(Foot.m_X) || Foot.m_Y > Ignored->GetHeightAtX(Foot.m_X) + 1.0f)
		{
			Body.m_IgnoredFootholdId = INDEX_NONE;
		}
	}

	const UClimbableComponent* Climbable = FindClimbable(Box, Body, Primitives);

	if (Body.m_bIsClimbing && !Climbable)
	{
		Body.StopClimbing();
	}

	if (!Body.m_bIsClimbing && Climbable && FMath::Abs(Body.m_ClimbInput) > FMath::SMALL_NUMBER)
	{
		// 중심으로 붙일 때 벽을 통과하지 않도록 짧은 보정 이동도 검사한다.
		const FVector2D AlignDelta(Climbable->GetWorldCenter().m_X - Box.GetWorldCenter().m_X, 0.0f);
		if (CanTranslateBox(Box, AlignDelta, Primitives))
		{
			Translate(Box, AlignDelta);
			Body.m_bIsClimbing = true;
			Body.m_ClimbableActorId = Climbable->GetOwner()->GetActorId();
			Body.m_ClimbableType = Climbable->GetClimbableType();
			Body.m_CoyoteTimeRemaining = 0.0f;
			Body.m_IgnoredFootholdId = INDEX_NONE;

			UE_LOG(Log, Log, L"[MapLadderRope] ClimbableType=%d ", Climbable->GetClimbableType());
		}
	}

	if (Body.m_bIsClimbing)
	{
		// 하단 아래에서 잡으면 현재 발 위치부터 속도대로 올라간다. 끝점으로 순간 이동하지 않는다.
		// 하단 아래에서 아래 입력을 누르면 끝점 이탈 검사로 바닥에 내려오고, 바닥이 없으면 정지한다.
		const FRect Bounds = Climbable->GetWorldBounds();
		const float FootY = GetFootPosition(Box).m_Y;
		const float TargetY = FMath::Clamp(FootY + Body.m_ClimbInput * Body.m_ClimbSpeed * DeltaTime, Bounds.m_Top, FMath::Max(FootY, Bounds.m_Bottom));
		Body.m_Velocity = FVector2D(0.0f, (TargetY - FootY) / DeltaTime);
	}

	else
	{
		Body.m_Velocity.m_Y = FMath::Min(Body.m_MaxFallSpeed, Body.m_Velocity.m_Y + m_Gravity * Body.m_GravityScale * DeltaTime);
	}

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

			FVector2D Normal;
			float Depth;

			const FOrientedBox2D Other = static_cast<const UBoxCollision*>(Primitives[i])->GetWorldBox();

			if (!Box.GetWorldBox().FindContact(Other, Normal, Depth) || Depth <= 0.0f)
			{
				continue;
			}

			Translate(Box, Normal * Depth);
			ResolveVelocity(Body, Normal);

			bMoved = true;
		}

		if (!bMoved)
		{
			break;
		}
	}

	float RemainingTime = DeltaTime;
	bool bWasFollowingFoothold = false;

	for (int32 Pass = 0; Pass < 16 && RemainingTime > 0.0f; Pass++)
	{
		const FFoothold* Support = !Body.m_bIsClimbing && Body.m_Velocity.m_Y >= 0.0f ? FindSupportingFoothold(Box, Body.m_Velocity.m_X, Body.m_IgnoredFootholdId) : nullptr;

		float MoveTime = RemainingTime;

		FVector2D Delta;

		if (Support)
		{
			Body.m_Velocity.m_Y = 0.0f;
			const FVector2D Foot = GetFootPosition(Box);

			if (FMath::Abs(Body.m_Velocity.m_X) > FMath::SMALL_NUMBER)
			{
				const float EdgeX = Body.m_Velocity.m_X > 0.0f ? Support->GetMaxX() : Support->GetMinX();
				MoveTime = FMath::Min(MoveTime, (EdgeX - Foot.m_X) / Body.m_Velocity.m_X);
			}

			Delta.m_X = Body.m_Velocity.m_X * MoveTime;
			Delta.m_Y = Support->GetHeightAtX(Foot.m_X + Delta.m_X) - Foot.m_Y;
		}

		else
		{
			if (bWasFollowingFoothold)
			{
				Body.m_Velocity.m_Y = FMath::Min(Body.m_MaxFallSpeed, m_Gravity * Body.m_GravityScale * RemainingTime);
			}

			Delta = Body.m_Velocity * MoveTime;
		}

		bWasFollowingFoothold = Support != nullptr;

		if (Delta.IsNearlyZero(FMath::SMALL_NUMBER))
		{
			break;
		}

		float Earliest = 1.0f;
		FVector2D Normal;
		const FFoothold* HitFoothold = nullptr;
		bool bHit = false;

		for (int32 i = 0; i < Primitives.Num(); i++)
		{
			if (!IsBlocker(Box, *Primitives[i]))
			{
				continue;
			}

			float Time;
			FVector2D CandidateNormal;

			if (Box.GetWorldBox().Sweep(static_cast<const UBoxCollision*>(Primitives[i])->GetWorldBox(), Delta, Time, CandidateNormal) && Time <= Earliest)
			{
				Earliest = Time;
				Normal = CandidateNormal;
				bHit = true;
			}
		}

		if (!Body.m_bIsClimbing && (Box.GetCollisionMask() & CollisionChannelMask(ECollisionChannel::WorldStatic)))
		{
			const FFoothold* GroupFoothold = FindWallGroupFoothold(Box, Support, Body.m_LastFootholdId);
			if (GroupFoothold && FMath::Abs(Delta.m_X) > FMath::SMALL_NUMBER)
			{
				const FVector2D Foot = GetFootPosition(Box);
				for (int32 i = 0; i < m_VerticalFootholds.Num(); i++)
				{
					const FVerticalFoothold& Wall = m_VerticalFootholds[i];
					if (Wall.m_Layer != GroupFoothold->GetLayer() || Wall.m_Group != GroupFoothold->GetGroup()
						|| !(Wall.m_CollisionMask & CollisionChannelMask(Box.GetCollisionObjectType())))
					{
						continue;
					}

					// WZ 수직 선분의 끝점 방향이 막는 접근 방향을 결정한다.
					// 오른쪽 이동은 위로 향하는 선분, 왼쪽 이동은 아래로 향하는 선분만 막는다.
					if ((Delta.m_X > 0.0f && Wall.m_Start.m_Y < Wall.m_End.m_Y)
						|| (Delta.m_X < 0.0f && Wall.m_Start.m_Y > Wall.m_End.m_Y))
					{
						continue;
					}

					const float Time = (Wall.m_Start.m_X - Foot.m_X) / Delta.m_X;
					if (Time < 0.0f || Time > Earliest || (bHit && Time == Earliest))
					{
						continue;
					}

					const float HitY = Foot.m_Y + Delta.m_Y * Time;
					if (HitY < FMath::Min(Wall.m_Start.m_Y, Wall.m_End.m_Y)
						|| HitY > FMath::Max(Wall.m_Start.m_Y, Wall.m_End.m_Y))
					{
						continue;
					}

					Earliest = Time;
					Normal = FVector2D(Delta.m_X > 0.0f ? -1.0f : 1.0f, 0.0f);
					HitFoothold = nullptr;
					bHit = true;
				}
			}
		}

		for (int32 i = 0; !Body.m_bIsClimbing && i < m_Footholds.Num(); i++)
		{
			const FFoothold& Foothold = m_Footholds[i];

			if (&Foothold == Support || Foothold.GetId() == Body.m_IgnoredFootholdId || !CanUseFoothold(Box, Foothold))
			{
				continue;
			}

			float Time;

			if (SweepFoothold(Foothold, GetFootPosition(Box), Delta, Time) && (Time < Earliest || (!bHit && Time <= Earliest)))
			{
				Earliest = Time;
				HitFoothold = &Foothold;
				bHit = true;
			}
		}

		Translate(Box, Delta * Earliest);

		RemainingTime -= MoveTime * Earliest;

		if (!bHit)
		{
			// 선분 끝점까지만 이동했다면 남은 시간으로 다음 선분이나 공중 이동을 처리한다.
			if (RemainingTime > FMath::SMALL_NUMBER)
			{
				continue;
			}

			break;
		}

		if (HitFoothold)
		{
			const FVector2D Foot = GetFootPosition(Box);
			Translate(Box, FVector2D(0.0f, HitFoothold->GetHeightAtX(Foot.m_X) - Foot.m_Y));
			Body.m_Velocity.m_Y = 0.0f;
		}

		else
		{
			ResolveVelocity(Body, Normal);
		}
	}

	if (Body.m_bIsClimbing)
	{
		Body.m_bIsGrounded = false;

		const UClimbableComponent* ActiveClimbable = FindClimbable(Box, Body, Primitives);
		if (!ActiveClimbable)
		{
			Body.StopClimbing();
		}

		else
		{
			TryExitClimbable(Body, Box, *ActiveClimbable, Primitives);
			return;
		}
	}

	// 월드 Y 속도 대신 면의 법선 방향 속도로 이탈을 판단해 경사면 위쪽 이동도 지지한다.
	Body.m_bIsGrounded = false;
	for (int32 i = 0; i < Primitives.Num(); i++)
	{
		if (!IsBlocker(Box, *Primitives[i]))
		{
			continue;
		}

		FVector2D Normal;
		float Depth;
		if (Box.GetWorldBox().FindContact(static_cast<const UBoxCollision*>(Primitives[i])->GetWorldBox(), Normal, Depth, 0.001f) && Normal.m_Y < -0.5f && Body.m_Velocity.Dot(Normal) <= 0.0001f)
		{
			Body.m_bIsGrounded = true;
			break;
		}
	}

	if (!Body.m_bIsGrounded && Body.m_Velocity.m_Y >= 0.0f)
	{
		const FFoothold* Support = FindSupportingFoothold(Box, 0.0f, Body.m_IgnoredFootholdId);

		if (Support)
		{
			Body.m_bIsGrounded = true;
			Body.m_CurrentFootholdId = Support->GetId();
			Body.m_LastFootholdId = Support->GetId();
		}
	}

	if (Body.m_bIsGrounded && Body.m_CurrentFootholdId == INDEX_NONE)
	{
		Body.m_LastFootholdId = INDEX_NONE;
	}

	if (Body.m_bIsGrounded)
	{
		Body.m_IgnoredFootholdId = INDEX_NONE;
	}
}

bool FPhysicsWorld::Raycast(const FVector2D& Start, const FVector2D& End, FHitResult& OutHit, uint32 ObjectMask, const AActor* IgnoreActor) const
{
	OutHit = FHitResult();
	TArray<UPrimitiveComponent*> Primitives;
	GatherPrimitives(Primitives);
	const FVector2D Delta = End - Start;

	for (int32 i = 0; i < Primitives.Num(); i++)
	{
		UPrimitiveComponent& Shape = *Primitives[i];

		if (!Shape.IsCollisionEnabled() || Shape.GetOwner() == IgnoreActor || !(ObjectMask & CollisionChannelMask(Shape.GetCollisionObjectType())))
		{
			continue;
		}

		float Time = 0.0f;
		FVector2D Normal;
		bool bInside;

		if (Shape.GetShapeType() == ECollisionShape::Box)
		{
			if (!static_cast<const UBoxCollision&>(Shape).GetWorldBox().Raycast(Start, End, Time, Normal, bInside))
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

	if (ObjectMask & CollisionChannelMask(ECollisionChannel::WorldStatic))
	{
		for (int32 i = 0; i < m_Footholds.Num(); i++)
		{
			const FFoothold& Foothold = m_Footholds[i];
			float Time;
			if (!Foothold.Raycast(Start, End, Time) || (OutHit.m_bBlockingHit && Time >= OutHit.m_Time))
			{
				continue;
			}

			OutHit.m_bBlockingHit = true;
			OutHit.m_bStartPenetrating = false;
			OutHit.m_Time = Time;
			OutHit.m_Normal = Foothold.GetNormal();
			OutHit.m_Point = Start + Delta * Time;
			OutHit.m_pComponent = nullptr;
			OutHit.m_FootholdId = Foothold.GetId();
		}
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

#ifdef _DEBUG
void FPhysicsWorld::DrawDebug(FSpriteBatch& SpriteBatch, const FCamera2D& Camera) const
{
	TArray<UPrimitiveComponent*> Primitives;
	GatherPrimitives(Primitives);
	TArray<uint8> ContactFlags;

	for (int32 i = 0; i < Primitives.Num(); i++)
	{
		ContactFlags.Add(0);
	}

	// 응답 마스크를 존중하며 Trigger 겹침도 접촉으로 표시한다.
	for (int32 i = 0; i < Primitives.Num(); i++)
	{
		for (int32 j = i + 1; j < Primitives.Num(); j++)
		{
			if (Primitives[i]->GetOwner() == Primitives[j]->GetOwner() || !Primitives[i]->CanCollideWith(*Primitives[j]))
			{
				continue;
			}

			bool bContact = Primitives[i]->Overlaps(*Primitives[j]);

			if (!bContact && Primitives[i]->GetShapeType() == ECollisionShape::Box && Primitives[j]->GetShapeType() == ECollisionShape::Box)
			{
				FVector2D Normal;
				float Depth;
				// 정적 Box의 접지 판정과 같은 오차를 허용해 아주 작은 간격도 접촉으로 표시한다.
				bContact = static_cast<const UBoxCollision*>(Primitives[i])->GetWorldBox().FindContact(
					static_cast<const UBoxCollision*>(Primitives[j])->GetWorldBox(), Normal, Depth, 0.001f);
			}

			if (bContact)
			{
				ContactFlags[i] = ContactFlags[j] = 1;
			}
		}

		const URigidbody* Body = Primitives[i]->GetOwner()->GetComponent<URigidbody>();

		if (Body && Body->m_bIsClimbing && Cast<UBoxCollision>(Primitives[i]))
		{
			ContactFlags[i] = 1;

			for (int32 j = 0; j < Primitives.Num(); j++)
			{
				if (Primitives[j]->GetOwner()->GetActorId() == Body->m_ClimbableActorId && Cast<UClimbableComponent>(Primitives[j]))
				{
					ContactFlags[j] = 1;
				}
			}
		}
	}

	const FLinearColor ContactColor(1.0f, 0.0f, 0.0f);
	const FLinearColor ClearColor(0.0f, 1.0f, 0.0f);
	const FRect Clip = Camera.GetScaledClipRect();
	const float Thickness = 1.5f / Camera.GetZoom();

	for (int32 i = 0; i < m_Footholds.Num(); i++)
	{
		const FFoothold& Foothold = m_Footholds[i];
		bool bContact = false;

		for (int32 j = 0; j < Primitives.Num(); j++)
		{
			const UBoxCollision* Box = Cast<UBoxCollision>(Primitives[j]);
			const URigidbody* Body = Box ? Box->GetOwner()->GetComponent<URigidbody>() : nullptr;
			if (Body && Body->m_bIsGrounded && Body->m_CurrentFootholdId == Foothold.GetId() && CanUseFoothold(*Box, Foothold))
			{
				bContact = true;
				ContactFlags[j] = 1;
			}
		}

		const FRect Bounds(Foothold.GetMinX(), FMath::Min(Foothold.GetStart().m_Y, Foothold.GetEnd().m_Y), Foothold.GetMaxX(), FMath::Max(Foothold.GetStart().m_Y, Foothold.GetEnd().m_Y));

		if (Bounds.Overlaps(Clip))
		{
			SpriteBatch.DrawDebugLine(Foothold.GetStart(), Foothold.GetEnd(), bContact ? ContactColor : ClearColor, Thickness);
		}
	}

	for (int32 i = 0; i < m_VerticalFootholds.Num(); i++)
	{
		const FVerticalFoothold& Wall = m_VerticalFootholds[i];
		bool bContact = false;
		for (int32 j = 0; j < Primitives.Num(); j++)
		{
			const UBoxCollision* Box = Cast<UBoxCollision>(Primitives[j]);
			const URigidbody* Body = Box ? Box->GetOwner()->GetComponent<URigidbody>() : nullptr;
			if (!Body || !Body->IsSimulatingPhysics() || Body->m_bIsClimbing || !Box->IsCollisionEnabled() || !(Box->GetCollisionMask() & CollisionChannelMask(ECollisionChannel::WorldStatic)) || !(Wall.m_CollisionMask & CollisionChannelMask(Box->GetCollisionObjectType())))
			{
				continue;
			}

			const FFoothold* Group = FindWallGroupFoothold(*Box, FindFoothold(Body->m_CurrentFootholdId), Body->m_LastFootholdId);
			const FVector2D Foot = GetFootPosition(*Box);

			if (Group && Group->GetLayer() == Wall.m_Layer && Group->GetGroup() == Wall.m_Group
				&& FMath::Abs(Foot.m_X - Wall.m_Start.m_X) <= 0.001f
				&& Foot.m_Y >= FMath::Min(Wall.m_Start.m_Y, Wall.m_End.m_Y)
				&& Foot.m_Y <= FMath::Max(Wall.m_Start.m_Y, Wall.m_End.m_Y))
			{
				bContact = true;
				ContactFlags[j] = 1;
			}
		}
		const FRect Bounds(Wall.m_Start.m_X, FMath::Min(Wall.m_Start.m_Y, Wall.m_End.m_Y), Wall.m_Start.m_X, FMath::Max(Wall.m_Start.m_Y, Wall.m_End.m_Y));
		if (Bounds.Overlaps(Clip))
		{
			SpriteBatch.DrawDebugLine(Wall.m_Start, Wall.m_End, bContact ? ContactColor : ClearColor, Thickness);
		}
	}

	for (int32 i = 0; i < Primitives.Num(); i++)
	{
		const UPrimitiveComponent& Primitive = *Primitives[i];

		if (!Primitive.IsCollisionEnabled() || !Primitive.GetWorldBounds().Overlaps(Clip))
		{
			continue;
		}

		const FLinearColor& Color = ContactFlags[i] ? ContactColor : ClearColor;
		const FVector2D Center = Primitive.GetWorldCenter();

		if (Primitive.GetShapeType() == ECollisionShape::Box)
		{
			// 외접 AABB 대신 충돌에 쓰는 OBB의 회전과 스케일을 그대로 표시한다.
			const FVector2D Extent = static_cast<const UBoxCollision&>(Primitive).GetScaledBoxExtent();
			const float Angle = Primitive.GetWorldTransform().m_Rotation;
			const FVector2D AxisX(FMath::Cos(Angle) * Extent.m_X, FMath::Sin(Angle) * Extent.m_X);
			const FVector2D AxisY(-FMath::Sin(Angle) * Extent.m_Y, FMath::Cos(Angle) * Extent.m_Y);
			const FVector2D Corners[] = { Center - AxisX - AxisY, Center + AxisX - AxisY, Center + AxisX + AxisY, Center - AxisX + AxisY };
			for (int32 Edge = 0; Edge < 4; Edge++)
			{
				SpriteBatch.DrawDebugLine(Corners[Edge], Corners[(Edge + 1) % 4], Color, Thickness);
			}
		}

		else if (Primitive.GetShapeType() == ECollisionShape::Circle)
		{
			const float Radius = static_cast<const UCircleCollision&>(Primitive).GetWorldRadius();
			FVector2D Previous = Center + FVector2D(Radius, 0.0f);

			for (int32 Segment = 1; Segment <= 32; Segment++)
			{
				const float Angle = FMath::TWO_PI * Segment / 32.0f;
				const FVector2D Next = Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius;
				SpriteBatch.DrawDebugLine(Previous, Next, Color, Thickness);
				Previous = Next;
			}
		}
	}
}
#endif