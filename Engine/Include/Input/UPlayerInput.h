#pragma once

#include "EnginePCH.h"
#include "Object/UObject.h"
#include "Input/InputTypes.h"
#include "Core/Containers/TArray.h"

class UPlayerInput : public UObject
{
	DECLARE_CLASS(UPlayerInput, UObject)

public:
	// 생성 / 소멸
	UPlayerInput();
	virtual ~UPlayerInput() override;

	// 프레임 시작 / 포커스 관리
	void BeginFrame();
	void Reset();
	void SetFocus(bool bHasFocus);
	bool HasFocus() const;

	// 플랫폼 키 입력 등록
	void InputKey(FKey Key, bool bDown);

	// 게임별 축 매핑 설정 / 조회
	void AddAxisMapping(const FInputAxisKeyMapping& Mapping);
	void RemoveAxisMappings(FName AxisName);
	float GetAxisValue(FName AxisName) const;

	// 게임별 동작 매핑 설정 / 눌림·해제 조회
	void AddActionMapping(const FInputActionKeyMapping& Mapping);
	void RemoveActionMappings(FName ActionName);
	bool WasActionPressed(FName ActionName) const;
	bool WasActionReleased(FName ActionName) const;
	bool HasKeyMapping(FKey Key) const;

	// 키 상태 조회
	bool IsKeyDown(FKey Key) const;
	bool WasKeyPressed(FKey Key) const;
	bool WasKeyReleased(FKey Key) const;

private:
	struct FKeyState
	{
		bool m_bDown = false;
		bool m_bPressed = false;
		bool m_bReleased = false;
	};

	FKeyState m_Keys[256];
	TArray<FInputAxisKeyMapping> m_AxisMappings;
	TArray<FInputActionKeyMapping> m_ActionMappings;
	bool m_bHasFocus = false;
};

