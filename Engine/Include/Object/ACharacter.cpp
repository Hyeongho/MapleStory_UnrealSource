#include "EnginePCH.h"
#include "Object/ACharacter.h"

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
	FTransform2D Transform = m_pSpriteComponent->GetRelativeTransform();
	Transform.m_Location = Location;
	m_pSpriteComponent->SetRelativeTransform(Transform);
}

FVector2D ACharacter::GetLocation() const
{
	return m_pSpriteComponent->GetWorldTransform().m_Location;
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

void ACharacter::Tick(float DeltaTime)
{
	FVector2D Direction = m_PendingMovementInput;
	m_PendingMovementInput = FVector2D::Zero;
	if (_finite(DeltaTime) && DeltaTime > 0.0f)
	{
		if (Direction.SizeSquared() > 1.0f)
		{
			Direction.Normalize();
		}
		if (!Direction.IsNearlyZero())
		{
			// 물리 몸체 연결 전에는 입력으로 스프라이트 위치를 직접 갱신한다.
			SetLocation(GetLocation() + Direction * m_MoveSpeed * DeltaTime);
			if (Direction.m_X != 0.0f)
			{
				SetFacingRight(Direction.m_X > 0.0f);
			}
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
