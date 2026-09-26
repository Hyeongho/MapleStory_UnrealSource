#pragma once

#include "EnginePCH.h"
#include "Object/AActor.h"
#include "Core/Math/FVector2D.h"

class UPlayerInput;
class UInputComponent;
class FCamera2D;

// 현재는 단일 로컬 플레이어를 지원하며 조작 정책을 엔진 시스템에서 분리한다.
class APlayerController : public AActor
{
	DECLARE_CLASS(APlayerController, AActor)

public:
	// 생성 / 소멸
	APlayerController();
	virtual ~APlayerController() override;
	APlayerController(const APlayerController&) = delete;
	APlayerController& operator=(const APlayerController&) = delete;

	// 카메라 연결과 입력 준비 / 수명 종료
	bool InitPlayer(FCamera2D& Camera);
	virtual void EndPlay() override;

	// 입력 처리는 월드 갱신 전, 카메라 갱신은 월드 갱신 후에 실행한다.
	virtual void ProcessPlayerInput(float DeltaTime);
	virtual void UpdateCamera(float DeltaTime);

	// 플레이어별 입력 시스템 조회
	UPlayerInput& GetPlayerInput() const;
	UInputComponent& GetInputComponent() const;

	// 기본 카메라 조작 설정
	void SetCameraInputEnabled(bool bEnabled);
	void SetCameraMoveSpeed(float Speed);

protected:
	// Game의 컨트롤러 파생 클래스에서 키 매핑과 함수 바인딩을 각각 재정의한다.
	virtual void SetupInputMappings();
	virtual void SetupInputComponent();
	virtual void MoveHorizontal(float Value);
	virtual void MoveVertical(float Value);

	UPlayerInput* m_pPlayerInput = nullptr; // 컨트롤러가 소유한다.
	UInputComponent* m_pInputComponent = nullptr; // AActor의 컴포넌트 배열이 소유한다.
	FCamera2D* m_pCamera = nullptr; // 엔진이 소유하며 월드보다 오래 살아 있다.
	FVector2D m_MoveInput = FVector2D::Zero;

private:
	bool m_bCameraInputEnabled = true;
	float m_CameraMoveSpeed = 400.0f;
};
