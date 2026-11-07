#include "EnginePCH.h"
#include "AMaplePlayerController.h"
#include "../Character/AMapleCharacter.h"
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
	const FName AttackAction(L"Attack");

	PlayerInput.RemoveAxisMappings(HorizontalAxis);
	PlayerInput.RemoveAxisMappings(VerticalAxis);
	PlayerInput.RemoveActionMappings(JumpAction);
	PlayerInput.RemoveActionMappings(AttackAction);

	// 다른 키를 사용하려면 여기서 EKeys를 교체한다. 월드의 Y는 아래쪽이 양수다.
	PlayerInput.AddAxisMapping(FInputAxisKeyMapping(HorizontalAxis, EKeys::Left, -1.0f));
	PlayerInput.AddAxisMapping(FInputAxisKeyMapping(HorizontalAxis, EKeys::Right, 1.0f));
	PlayerInput.AddAxisMapping(FInputAxisKeyMapping(VerticalAxis, EKeys::Up, -1.0f));
	PlayerInput.AddAxisMapping(FInputAxisKeyMapping(VerticalAxis, EKeys::Down, 1.0f));
	PlayerInput.AddActionMapping(FInputActionKeyMapping(JumpAction, EKeys::Alt));
	PlayerInput.AddActionMapping(FInputActionKeyMapping(AttackAction, EKeys::Ctrl));

#ifdef _DEBUG
	const FName DamageAction(L"DebugDamage");
	const FName ResetAction(L"DebugResetHealth");

	PlayerInput.RemoveActionMappings(DamageAction);
	PlayerInput.RemoveActionMappings(ResetAction);
	PlayerInput.AddActionMapping(FInputActionKeyMapping(DamageAction, FKey(VK_F1)));
	PlayerInput.AddActionMapping(FInputActionKeyMapping(ResetAction, FKey(VK_F2)));
#endif
}

void AMaplePlayerController::SetupInputComponent()
{
	// 기본 축 바인딩을 유지하고 점프는 누른 순간에만 실행한다.
	Super::SetupInputComponent();
	GetInputComponent().BindAction<AMaplePlayerController, &AMaplePlayerController::OnJumpPressed>(FName(L"Jump"), EInputEvent::Pressed, this);
	GetInputComponent().BindAction<AMaplePlayerController, &AMaplePlayerController::OnAttackPressed>(FName(L"Attack"), EInputEvent::Pressed, this);

#ifdef _DEBUG
	GetInputComponent().BindAction<AMaplePlayerController, &AMaplePlayerController::OnDebugDamagePressed>(FName(L"DebugDamage"), EInputEvent::Pressed, this);
	GetInputComponent().BindAction<AMaplePlayerController, &AMaplePlayerController::OnDebugResetHealthPressed>(FName(L"DebugResetHealth"), EInputEvent::Pressed, this);
#endif
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

void AMaplePlayerController::OnAttackPressed()
{
	if (AMapleCharacter* pCharacter = Cast<AMapleCharacter>(GetCharacter()))
	{
		if (!pCharacter->IsAttacking())
		{
			pCharacter->Attack();
		}
	}
}

#ifdef _DEBUG
void AMaplePlayerController::OnDebugDamagePressed()
{
	if (AMapleCharacter* pCharacter = Cast<AMapleCharacter>(GetCharacter()))
	{
		pCharacter->TakeDamage(10.0f);
	}
}

void AMaplePlayerController::OnDebugResetHealthPressed()
{
	if (AMapleCharacter* pCharacter = Cast<AMapleCharacter>(GetCharacter()))
	{
		pCharacter->ResetHealth();
	}
}
#endif
