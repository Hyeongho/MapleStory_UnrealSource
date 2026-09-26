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
	m_pSpriteComponent->SetRelativeTransform(FTransform2D(Location, 0.0f, FVector2D(1.0f, 1.0f)));
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
