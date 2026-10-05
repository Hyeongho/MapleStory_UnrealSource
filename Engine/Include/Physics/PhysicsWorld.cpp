#include "EnginePCH.h"
#include "Physics/PhysicsWorld.h"
#include "Physics/UBoxCollision.h"
#include "Physics/UCircleCollision.h"
#include "Physics/UClimbableComponent.h"
#include "Physics/URigidbody.h"
#include "World/UWorld.h"

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

void FPhysicsWorld::ClearFootholds()
{
	m_Footholds.Empty();
	m_VerticalFootholds.Empty();
	InvalidateFootholdSupport(INDEX_NONE);
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
		if (Id == INDEX_NONE || Body->m_LastFootholdId == Id)
		{
			Body->m_LastFootholdId = INDEX_NONE;
		}
		if (Id == INDEX_NONE || Body->m_IgnoredFootholdId == Id)
		{
			Body->m_IgnoredFootholdId = INDEX_NONE;
		}
		if (Id == INDEX_NONE)
		{
			Body->m_bDropThroughRequested = false;
		}
	}
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
		if (Foothold.GetLayer() == INDEX_NONE || Foothold.GetGroup() == INDEX_NONE
			|| !CanUseFoothold(Box, Foothold) || !Foothold.ContainsX(Foot.m_X))
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

const FFoothold* FPhysicsWorld::FindDropLandingFoothold(const UBoxCollision& Box, int32 CurrentFootholdId, float MaxDropHeight) const
{
	if (!_finite(MaxDropHeight) || MaxDropHeight <= 1.0f)
	{
		return nullptr;
	}

	const FVector2D Foot = GetFootPosition(Box);
	const FFoothold* Current = FindFoothold(CurrentFootholdId);
	if (!Current || !CanUseFoothold(Box, *Current) || !Current->ContainsX(Foot.m_X)
		|| FMath::Abs(Foot.m_Y - Current->GetHeightAtX(Foot.m_X)) > 0.001f)
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

const UClimbableComponent* FPhysicsWorld::FindClimbable(const UBoxCollision& Box, const TArray<UPrimitiveComponent*>& Primitives)
{
	for (int32 i = 0; i < Primitives.Num(); i++)
	{
		const UClimbableComponent* Climbable = Cast<UClimbableComponent>(Primitives[i]);

		if (Climbable && Climbable->GetOwner() != Box.GetOwner() && Climbable->GetCollisionObjectType() == ECollisionChannel::Trigger && Box.Overlaps(*Climbable))
		{
			return Climbable;
		}
	}
	return nullptr;
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
	if (Body.m_bDropThroughRequested)
	{
		Body.m_bDropThroughRequested = false;
		if (Body.m_bIsGrounded && !Body.m_bIsClimbing
			&& FindDropLandingFoothold(Box, Body.m_CurrentFootholdId, Body.m_MaxDropHeight))
		{
			// 아래에 도달 가능한 발판이 있을 때만 현재 발판을 통과한다.
			Body.m_IgnoredFootholdId = Body.m_CurrentFootholdId;
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

	const UClimbableComponent* Climbable = FindClimbable(Box, Primitives);
	if (Body.m_bIsClimbing && !Climbable)
	{
		Body.StopClimbing();
	}

	if (!Body.m_bIsClimbing && Climbable && FMath::Abs(Body.m_ClimbInput) > FMath::SMALL_NUMBER)
	{
		Body.m_bIsClimbing = true;
	}

	if (Body.m_bIsClimbing)
	{
		// 진입 시 기존 낙하·수평 속도를 버리고 이동 입력으로만 움직인다.
		Body.m_Velocity = FVector2D(0.0f, Body.m_ClimbInput * Body.m_ClimbSpeed);
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
				Body.m_Velocity.m_Y = FMath::Min(Body.m_MaxFallSpeed,
					m_Gravity * Body.m_GravityScale * RemainingTime);
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
		if (!FindClimbable(Box, Primitives))
		{
			Body.StopClimbing();
		}

		else
		{
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
