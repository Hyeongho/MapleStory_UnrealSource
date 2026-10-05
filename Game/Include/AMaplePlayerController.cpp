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
