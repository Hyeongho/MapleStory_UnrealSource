#pragma once

#include "EnginePCH.h"
#include "UGameInstance.h"
#include "Timer/FTimerHandle.h"
#include "Core/String/FName.h"

class ACharacter;
class UAnimStateMachine;

class UMapleGameInstance :
    public UGameInstance
{
    DECLARE_CLASS(UMapleGameInstance, UGameInstance)

public:
	// 생성 / 소멸
	UMapleGameInstance();
	virtual ~UMapleGameInstance() override;

	// 게임 수명 / 프레임 갱신
	virtual bool Init(UEngine& Engine) override;
	virtual void Shutdown() override;
	virtual void Tick(float DeltaTime) override;

protected:
	// 게임 구현을 확장할 때 사용하는 캐릭터 및 애니메이션 준비 단계
	void InitPlayer();
	bool RegisterAvatarState(FName StateName, const char* ActionName);
	ACharacter* m_pPlayerCharacter = nullptr; // 실제 소유자는 엔진의 UWorld다.
	UAnimStateMachine* m_pAnimStateMachine = nullptr;

private:
	// 입력을 연결하기 전까지 기존 데모의 상태 전환을 유지한다.
	void ToggleAnimDemoState();
	FTimerHandle m_AnimDemoToggleHandle;
	bool m_bMoving = false;
};

