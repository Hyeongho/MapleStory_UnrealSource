#pragma once

#include "EnginePCH.h"
#include "UGameInstance.h"

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
	// 물리 갱신 후 게임 캐릭터에 애니메이션 선택을 위임한다.
	virtual void Tick(float DeltaTime) override;

protected:
	// 게임별 키 설정과 동작 바인딩을 담당하는 컨트롤러를 생성한다.
	virtual APlayerController* CreatePlayerController(UWorld& World) override;

	// 게임 캐릭터의 생성과 맵 시작 위치 설정
	void InitPlayer();
};
