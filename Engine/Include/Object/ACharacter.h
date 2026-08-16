#pragma once
#include "Object/AActor.h"
#include "Render/USpriteComponent.h"
#include "Render/WzTextureLoader.h"

// 최소한의 "그릇" — 지금까지 main.cpp에서 전역 변수로 테스트해온 아바타
// 렌더링을 실제 AActor/UActorComponent 프레임워크 위로 옮기기 위한 첫
// 게임 오브젝트다. Phase 16(캐릭터) 본 작업(스탯/스킬/인벤토리 등)이
// 아니라, 그 전에 다른 엔진 시스템(애니메이션/물리/입력 등)이 컴포넌트로
// 붙어나갈 수 있는 뼈대만 만든다.
class ACharacter : public AActor
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
