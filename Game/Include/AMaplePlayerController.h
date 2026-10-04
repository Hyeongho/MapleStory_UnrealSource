#pragma once

#include "EnginePCH.h"
#include "Object/APlayerController.h"

class AMaplePlayerController :
    public APlayerController
{
    DECLARE_CLASS(AMaplePlayerController, APlayerController)

public:
    AMaplePlayerController();
    virtual ~AMaplePlayerController() override;
    virtual void ProcessPlayerInput(float DeltaTime) override;

protected:
    // 키를 변경할 때는 매핑을, 동작을 변경할 때는 바인딩을 수정한다.
    virtual void SetupInputMappings() override;
    virtual void SetupInputComponent() override;
};

