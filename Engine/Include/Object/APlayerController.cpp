#include "EnginePCH.h"
#include "Object/APlayerController.h"
#include "Object/ACharacter.h"
#include "Input/UPlayerInput.h"
#include "Input/UInputComponent.h"
#include "Render/FCamera2D.h"

APlayerController::APlayerController()
{
	m_pPlayerInput = new UPlayerInput();
	m_pInputComponent = AddComponent<UInputComponent>();
}

APlayerController::~APlayerController()
{
	// 델리게이트가 컨트롤러를 참조하므로 입력 객체를 해제하기 전에 끊는다.
	m_pInputComponent->ClearAxisBindings();
	m_pInputComponent->ClearActionBindings();
	delete m_pPlayerInput;
	m_pPlayerInput = nullptr;
}

bool APlayerController::InitPlayer(FCamera2D& Camera)
{
	if (m_pCamera)
	{
		return false;
	}
	m_pCamera = &Camera;
	SetupInputMappings();
	SetupInputComponent();
	return true;
}

void APlayerController::EndPlay()
{
	UnPossess();
	m_pInputComponent->ClearAxisBindings();
	m_pInputComponent->ClearActionBindings();
	m_pPlayerInput->SetFocus(false);
	m_MoveInput = FVector2D::Zero;
	m_pCamera = nullptr;
	Super::EndPlay();
}

void APlayerController::Possess(ACharacter* Character)
{
	m_pCharacter = Character;
	if (m_pCamera && Character)
	{
		m_pCamera->SetLocation(Character->GetLocation());
	}
}

void APlayerController::UnPossess()
{
	m_pCharacter = nullptr;
}

ACharacter* APlayerController::GetCharacter() const
{
	return m_pCharacter;
}

void APlayerController::SetupInputMappings()
{
	const FName HorizontalAxis(L"MoveHorizontal");
	const FName VerticalAxis(L"MoveVertical");
	m_pPlayerInput->RemoveAxisMappings(HorizontalAxis);
	m_pPlayerInput->RemoveAxisMappings(VerticalAxis);
	m_pPlayerInput->AddAxisMapping(FInputAxisKeyMapping(HorizontalAxis, EKeys::Left, -1.0f));
	m_pPlayerInput->AddAxisMapping(FInputAxisKeyMapping(HorizontalAxis, EKeys::Right, 1.0f));
	m_pPlayerInput->AddAxisMapping(FInputAxisKeyMapping(VerticalAxis, EKeys::Up, -1.0f));
	m_pPlayerInput->AddAxisMapping(FInputAxisKeyMapping(VerticalAxis, EKeys::Down, 1.0f));
}

void APlayerController::SetupInputComponent()
{
	m_pInputComponent->ClearAxisBindings();
	m_pInputComponent->ClearActionBindings();
	m_pInputComponent->BindAxis<APlayerController, &APlayerController::MoveHorizontal>(FName(L"MoveHorizontal"), this);
	m_pInputComponent->BindAxis<APlayerController, &APlayerController::MoveVertical>(FName(L"MoveVertical"), this);
}

void APlayerController::ProcessPlayerInput(float DeltaTime)
{
	(void)DeltaTime;
	// 바인딩을 제거해도 이전 프레임의 이동 입력이 남지 않도록 먼저 비운다.
	m_MoveInput = FVector2D::Zero;
	if (m_pCamera)
	{
		m_pInputComponent->ProcessInput(*m_pPlayerInput);
		if (m_pCharacter)
		{
			m_pCharacter->AddMovementInput(m_MoveInput);
		}
	}
}

void APlayerController::MoveHorizontal(float Value)
{
	m_MoveInput.m_X = _finite(Value) ? FMath::Clamp(Value, -1.0f, 1.0f) : 0.0f;
}

void APlayerController::MoveVertical(float Value)
{
	m_MoveInput.m_Y = _finite(Value) ? FMath::Clamp(Value, -1.0f, 1.0f) : 0.0f;
}

void APlayerController::UpdateCamera(float DeltaTime)
{
	if (!m_pCamera)
	{
		return;
	}
	if (m_pCharacter)
	{
		// 월드 갱신 후의 캐릭터 위치를 화면 중심으로 사용한다.
		m_pCamera->SetLocation(m_pCharacter->GetLocation());
		return;
	}
	if (!m_bCameraInputEnabled || !m_pPlayerInput->HasFocus() || !_finite(DeltaTime) || DeltaTime <= 0.0f)
	{
		return;
	}
	FVector2D Direction = m_MoveInput;
	// 두 방향키를 함께 눌러도 대각선 이동 속도가 더 빨라지지 않게 한다.
	if (Direction.SizeSquared() > 1.0f)
	{
		Direction.Normalize();
	}
	m_pCamera->SetLocation(m_pCamera->GetLocation() + Direction * m_CameraMoveSpeed * DeltaTime);
}

UPlayerInput& APlayerController::GetPlayerInput() const
{
	check(m_pPlayerInput);
	return *m_pPlayerInput;
}

UInputComponent& APlayerController::GetInputComponent() const
{
	check(m_pInputComponent);
	return *m_pInputComponent;
}

void APlayerController::SetCameraInputEnabled(bool bEnabled)
{
	m_bCameraInputEnabled = bEnabled;
}

void APlayerController::SetCameraMoveSpeed(float Speed)
{
	if (_finite(Speed) && Speed >= 0.0f)
	{
		m_CameraMoveSpeed = Speed;
	}
}
