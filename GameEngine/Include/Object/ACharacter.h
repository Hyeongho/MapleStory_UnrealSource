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

	void SetFacingRight(bool bFacingRight);

	// 맵의 Life 컨테이너 설정. INDEX_NONE은 발판에 속하지 않는 Sky 컨테이너다.
	void SetMapLayer(int32 LayerIndex, int32 FootholdOrder = 0);
	int32 GetMapLayer() const;

	USpriteComponent* GetSpriteComponent() const 
	{ 
		return m_pSpriteComponent; 
	}

private:
	USpriteComponent* m_pSpriteComponent = nullptr;
	int32 m_MapLayer = INDEX_NONE;
};

