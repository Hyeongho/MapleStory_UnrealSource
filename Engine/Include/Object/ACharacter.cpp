#include "EnginePCH.h"
#include "ACharacter.h"
#include "Physics/UBoxCollision.h"
#include "Physics/URigidbody.h"

ACharacter::ACharacter()
{
	m_pSpriteComponent = AddComponent<USpriteComponent>();
	SetMapLayer(INDEX_NONE);
}

ACharacter::~ACharacter()
{
}

void ACharacter::LoadAvatar(FDXDevice& Device, const char* WzPath, const char* LoadoutSpec, const char* ActionName, int32 FrameIndex, const char* EmotionName, int32 EmotionFrameIndex)
{
	FAvatarTexture Avatar = FWzTextureLoader::LoadAvatarTexture(Device, WzPath, LoadoutSpec, ActionName, FrameIndex, EmotionName, EmotionFrameIndex);
	if (Avatar.m_pTexture)
	{
		m_pSpriteComponent->SetTexture(Avatar.m_pTexture, Avatar.m_Origin);
	}
}

void ACharacter::SetLocation(const FVector2D& Location)
{
	USceneComponent* Root = m_pCollisionComponent ? static_cast<USceneComponent*>(m_pCollisionComponent) : m_pSpriteComponent;

	FTransform2D Transform = Root->GetRelativeTransform();

	Transform.m_Location = Location;
	Root->SetRelativeTransform(Transform);
}

FVector2D ACharacter::GetLocation() const
{
	return m_pSpriteComponent->GetWorldTransform().m_Location;
}

void ACharacter::EnablePhysics(const FVector2D& CollisionHalfExtent)
{
	if (m_pCollisionComponent || !_finite(CollisionHalfExtent.m_X) || !_finite(CollisionHalfExtent.m_Y)
		|| CollisionHalfExtent.m_X <= 0.0f || CollisionHalfExtent.m_Y <= 0.0f)
	{
		return;
	}

	const FVector2D Location = GetLocation();
	m_pCollisionComponent = AddComponent<UBoxCollision>();
	m_pCollisionComponent->SetBoxExtent(CollisionHalfExtent);
	m_pCollisionComponent->SetCenterOffset(FVector2D(0.0f, -CollisionHalfExtent.m_Y));
	m_pCollisionComponent->SetCollisionObjectType(ECollisionChannel::Player);
	m_pCollisionComponent->SetCollisionMask(CollisionChannelMask(ECollisionChannel::WorldStatic) | CollisionChannelMask(ECollisionChannel::Trigger));
	m_pCollisionComponent->SetRelativeTransform(FTransform2D(Location, 0.0f, FVector2D(1.0f, 1.0f)));

	// 스프라이트 원점(발 위치)을 물리 루트에 붙여 두 좌표를 함께 이동시킨다.
	m_pSpriteComponent->SetRelativeTransform(FTransform2D::Identity());
	m_pSpriteComponent->SetAttachParent(m_pCollisionComponent);
	m_pRigidbody = AddComponent<URigidbody>();
}

void ACharacter::AddMovementInput(const FVector2D& Direction)
{
	if (_finite(Direction.m_X) && _finite(Direction.m_Y))
	{
		m_PendingMovementInput += Direction;
	}
}

void ACharacter::SetMoveSpeed(float Speed)
{
	if (_finite(Speed) && Speed >= 0.0f)
	{
		m_MoveSpeed = Speed;
	}
}

void ACharacter::SetJumpSpeed(float Speed)
{
	if (_finite(Speed) && Speed > 0.0f)
	{
		m_JumpSpeed = Speed;
	}
}

void ACharacter::Jump()
{
	if (m_pRigidbody)
	{
		m_pRigidbody->TryJump(m_JumpSpeed);
	}
}

void ACharacter::DropThroughFoothold()
{
	if (m_pRigidbody)
	{
		m_pRigidbody->RequestDropThroughFoothold();
	}
}

void ACharacter::Tick(float DeltaTime)
{
	FVector2D Direction = m_PendingMovementInput;
	m_PendingMovementInput = FVector2D::Zero;

	if (_finite(DeltaTime) && DeltaTime > 0.0f)
	{
		if (m_pRigidbody)
		{
			// 수평 속도만 조작하고 중력·착지에 따른 수직 속도는 물리 월드에 맡긴다.
			FVector2D Velocity = m_pRigidbody->GetVelocity();
			Velocity.m_X = FMath::Clamp(Direction.m_X, -1.0f, 1.0f) * m_MoveSpeed;
			m_pRigidbody->SetVelocity(Velocity);
			m_pRigidbody->SetClimbInput(Direction.m_Y);
		}

		else
		{
			if (Direction.SizeSquared() > 1.0f)
			{
				Direction.Normalize();
			}

			if (!Direction.IsNearlyZero())
			{
				SetLocation(GetLocation() + Direction * m_MoveSpeed * DeltaTime);
			}
		}

		if (Direction.m_X != 0.0f)
		{
			SetFacingRight(Direction.m_X > 0.0f);
		}
	}

	Super::Tick(DeltaTime);
}

void ACharacter::SetFacingRight(bool bFacingRight)
{
	m_pSpriteComponent->SetFlipHorizontal(bFacingRight);
}

void ACharacter::SetMapLayer(int32 LayerIndex, int32 FootholdOrder)
{
	if (LayerIndex == INDEX_NONE)
	{
		m_MapLayer = INDEX_NONE;
		m_pSpriteComponent->SetLayer(ELayer::Sky);
		m_pSpriteComponent->SetContainerOrder(0);
		return;
	}

	if (LayerIndex < 0 || LayerIndex > 7)
	{
		return;
	}

	m_MapLayer = LayerIndex;
	m_pSpriteComponent->SetLayer(MakeMapLifeLayer(LayerIndex));
	m_pSpriteComponent->SetContainerOrder(FootholdOrder);
}

int32 ACharacter::GetMapLayer() const
{
	return m_MapLayer;
}
