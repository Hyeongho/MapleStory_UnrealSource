#include "EnginePCH.h"
#include "AMaplePlayerController.h"
#include "Input/UPlayerInput.h"
#include "Input/UInputComponent.h"
#include "Object/ACharacter.h"
#include "Animation/UAnimStateMachine.h"
#include "Physics/URigidbody.h"

AMaplePlayerController::AMaplePlayerController() = default;

AMaplePlayerController::~AMaplePlayerController() = default;

void AMaplePlayerController::ProcessPlayerInput(float DeltaTime)
{
	Super::ProcessPlayerInput(DeltaTime);
	ACharacter* pCharacter = GetCharacter();
	if (!pCharacter)
	{
		return;
	}

	UAnimStateMachine* pStateMachine = pCharacter->GetComponent<UAnimStateMachine>();

	if (pStateMachine && pStateMachine->HasState(FName(L"Idle")) && pStateMachine->HasState(FName(L"Move")))
	{
		// 물리 캐릭터는 수평 이동이나 실제 사다리 이동 중에만 걷기 상태로 바꾼다.
		const URigidbody* Body = pCharacter->GetComponent<URigidbody>();
		const bool bMoving = Body ? m_MoveInput.m_X != 0.0f || Body->IsClimbing() : !m_MoveInput.IsNearlyZero();
		pStateMachine->SetState(bMoving ? FName(L"Move") : FName(L"Idle"));
	}
}

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
		if (m_MoveInput.m_Y > 0.0f)
		{
			pCharacter->DropThroughFoothold();
		}

		else
		{
			pCharacter->Jump();
		}
	}
}
