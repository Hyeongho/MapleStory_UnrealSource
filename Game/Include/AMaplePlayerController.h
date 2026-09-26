#pragma once

#include "EnginePCH.h"
#include "Object/APlayerController.h"

// MapleStory의 키 설정과 조작 바인딩을 확장하는 게임 컨트롤러다.
class AMaplePlayerController : public APlayerController
{
	DECLARE_CLASS(AMaplePlayerController, APlayerController)

public:
	// 생성 / 소멸
	AMaplePlayerController();
	virtual ~AMaplePlayerController() override;

protected:
	// 키를 변경할 때는 매핑을, 동작을 변경할 때는 바인딩을 수정한다.
	virtual void SetupInputMappings() override;
	virtual void SetupInputComponent() override;
};
