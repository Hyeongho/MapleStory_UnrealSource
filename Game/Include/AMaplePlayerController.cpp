#include "EnginePCH.h"
#include "AMaplePlayerController.h"
#include "Input/UPlayerInput.h"
#include "Input/UInputComponent.h"
#include "Object/ACharacter.h"
#include "Physics/URigidbody.h"

AMaplePlayerController::AMaplePlayerController() = default;

AMaplePlayerController::~AMaplePlayerController() = default;

void AMaplePlayerController::SetupInputMappings()
{
	UPlayerInput& PlayerInput = GetPlayerInput();
	const FName HorizontalAxis(L"MoveHorizontal");
	const FName VerticalAxis(L"MoveVertical");
	const FName JumpAction(L"Jump");
	PlayerInput.RemoveAxisMappings(HorizontalAxis);
	PlayerInput.RemoveAxisMappings(VerticalAxis);
	PlayerInput.RemoveActionMappings(JumpAction);
	// 방향키와 점프 키는 여기서 교체할 수 있다. 월드의 Y는 아래쪽이 양수다.
	PlayerInput.AddAxisMapping(FInputAxisKeyMapping(HorizontalAxis, EKeys::Left, -1.0f));
	PlayerInput.AddAxisMapping(FInputAxisKeyMapping(HorizontalAxis, EKeys::Right, 1.0f));
	PlayerInput.AddAxisMapping(FInputAxisKeyMapping(VerticalAxis, EKeys::Up, -1.0f));
	PlayerInput.AddAxisMapping(FInputAxisKeyMapping(VerticalAxis, EKeys::Down, 1.0f));
	PlayerInput.AddActionMapping(FInputActionKeyMapping(JumpAction, EKeys::Alt));
}

void AMaplePlayerController::SetupInputComponent()
{
	// 기본 축 바인딩을 유지하고 점프는 누른 순간에만 실행한다.
	Super::SetupInputComponent();
	GetInputComponent().BindAction<AMaplePlayerController, &AMaplePlayerController::OnJumpPressed>(FName(L"Jump"), EInputEvent::Pressed, this);
}

void AMaplePlayerController::OnJumpPressed()
{
	if (ACharacter* pCharacter = GetCharacter())
	{
		// 축 입력은 액션보다 먼저 처리돼 같은 프레임의 아래 방향키를 읽을 수 있다.
		// 매달려 있을 때는 아래 입력도 점프 이탈로 처리하고, 지상에서만 하단 점프한다.
		const URigidbody* pBody = pCharacter->GetComponent<URigidbody>();
		if (m_MoveInput.m_Y > 0.0f && (!pBody || !pBody->IsClimbing()))
		{
			pCharacter->DropThroughFoothold();
		}
		else
		{
			pCharacter->Jump();
		}
	}
}
