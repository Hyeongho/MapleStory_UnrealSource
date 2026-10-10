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

	// 다른 키를 사용하려면 여기서 EKeys를 교체한다. 월드의 Y는 아래쪽이 양수다.
	PlayerInput.AddAxisMapping(FInputAxisKeyMapping(HorizontalAxis, EKeys::Left, -1.0f));
	PlayerInput.AddAxisMapping(FInputAxisKeyMapping(HorizontalAxis, EKeys::Right, 1.0f));
	PlayerInput.AddAxisMapping(FInputAxisKeyMapping(VerticalAxis, EKeys::Up, -1.0f));
	PlayerInput.AddAxisMapping(FInputAxisKeyMapping(VerticalAxis, EKeys::Down, 1.0f));
	PlayerInput.AddActionMapping(FInputActionKeyMapping(JumpAction, EKeys::Alt));
}

void AMaplePlayerController::SetupInputComponent()
{
	// 현재는 기본 카메라 이동에 연결한다. 캐릭터 조작은 이후 여기서 바인딩한다.
	Super::SetupInputComponent();

	GetInputComponent().BindAction<AMaplePlayerController, &AMaplePlayerController::OnJumpPressed>(FName(L"Jump"), EInputEvent::Pressed, this);
}

void AMaplePlayerController::OnJumpPressed()
{
	if (ACharacter* pCharacter = GetCharacter())
	{
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
