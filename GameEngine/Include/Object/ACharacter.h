#pragma once
#include "AActor.h"
#include "Render/USpriteComponent.h"
#include "Render/WzTextureLoader.h"

class ACharacter :
    public AActor
{
    DECLARE_CLASS(ACharacter, AActor)
public:
    ACharacter();
    virtual ~ACharacter() override;

	// FWzTextureLoader::LoadAvatarTexture를 감싸서, 결과를 내부 USpriteComponent에
	// 바로 세팅한다. 실패(DLL/노드 없음)해도 조용히 무시 — 호출자는
	// GetSpriteComponent()->HasTexture()로 확인.
	void LoadAvatar(FDXDevice& Device, const char* WzPath, const char* LoadoutSpec, const char* ActionName, int32 FrameIndex, const char* EmotionName = "default", int32 EmotionFrameIndex = 0);

	void SetLocation(const FVector2D& Location);

	USpriteComponent* GetSpriteComponent() const { return m_pSpriteComponent; }

private:
	USpriteComponent* m_pSpriteComponent = nullptr;
};

