#include "EnginePCH.h"
#include "AMaplePlayerController.h"
#include "Input/UPlayerInput.h"
#include "Input/UInputComponent.h"

AMaplePlayerController::AMaplePlayerController() = default;

AMaplePlayerController::~AMaplePlayerController() = default;

void AMaplePlayerController::SetupInputMappings()
{
	UPlayerInput& PlayerInput = GetPlayerInput();
	const FName HorizontalAxis(L"MoveHorizontal");
	const FName VerticalAxis(L"MoveVertical");
	PlayerInput.RemoveAxisMappings(HorizontalAxis);
	PlayerInput.RemoveAxisMappings(VerticalAxis);
	// 다른 키를 사용하려면 여기서 EKeys를 교체한다. 월드의 Y는 아래쪽이 양수다.
	PlayerInput.AddAxisMapping(FInputAxisKeyMapping(HorizontalAxis, EKeys::Left, -1.0f));
	PlayerInput.AddAxisMapping(FInputAxisKeyMapping(HorizontalAxis, EKeys::Right, 1.0f));
	PlayerInput.AddAxisMapping(FInputAxisKeyMapping(VerticalAxis, EKeys::Up, -1.0f));
	PlayerInput.AddAxisMapping(FInputAxisKeyMapping(VerticalAxis, EKeys::Down, 1.0f));
}

void AMaplePlayerController::SetupInputComponent()
{
	// 현재는 기본 카메라 이동에 연결한다. 캐릭터 조작은 이후 여기서 바인딩한다.
	Super::SetupInputComponent();
}
