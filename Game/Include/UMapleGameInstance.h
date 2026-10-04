#pragma once

#include "EnginePCH.h"
#include "UGameInstance.h"
#include "Core/String/FName.h"

class ACharacter;
class UAnimStateMachine;

// 맵과 플레이어 캐릭터의 초기화 등 MapleStory 전용 설정을 담당한다.
class UMapleGameInstance : public UGameInstance
{
	DECLARE_CLASS(UMapleGameInstance, UGameInstance)

public:
	// 생성 / 소멸
	UMapleGameInstance();
	virtual ~UMapleGameInstance() override;

	// 게임 수명
	virtual bool Init(UEngine& Engine) override;
	virtual void Shutdown() override;

protected:
	// 게임별 키 설정과 동작 바인딩을 담당하는 컨트롤러를 생성한다.
	virtual APlayerController* CreatePlayerController(UWorld& World) override;

	// 게임 구현을 확장할 때 사용하는 캐릭터 및 애니메이션 준비 단계
	void InitPlayer();
	bool RegisterAvatarState(FName StateName, const char* ActionName);
	ACharacter* m_pPlayerCharacter = nullptr; // 실제 소유자는 엔진의 UWorld다.
	UAnimStateMachine* m_pAnimStateMachine = nullptr;
};
