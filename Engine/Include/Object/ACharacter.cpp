#include "EnginePCH.h"
#include "Object/ACharacter.h"

ACharacter::ACharacter()
{
	m_pSpriteComponent = AddComponent<USpriteComponent>();
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
