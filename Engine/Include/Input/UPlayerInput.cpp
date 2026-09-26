#include "EnginePCH.h"
#include "Input/UPlayerInput.h"

UPlayerInput::UPlayerInput() = default;

UPlayerInput::~UPlayerInput() = default;

void UPlayerInput::BeginFrame()
{
	// 누르고 있는 상태는 유지하고 이번 프레임의 전환 기록만 비운다.
	for (int32 i = 0; i < 256; i++)
	{
		m_Keys[i].m_bPressed = false;
		m_Keys[i].m_bReleased = false;
	}
}

void UPlayerInput::Reset()
{
	// 포커스를 잃을 때 키 상태만 초기화하고 게임이 설정한 매핑은 유지한다.
	for (int32 i = 0; i < 256; i++)
	{
		m_Keys[i] = FKeyState();
	}
}

void UPlayerInput::SetFocus(bool bHasFocus)
{
	if (m_bHasFocus != bHasFocus)
	{
		Reset();
	}
	m_bHasFocus = bHasFocus;
}

bool UPlayerInput::HasFocus() const
{
	return m_bHasFocus;
}

void UPlayerInput::InputKey(FKey Key, bool bDown)
{
	if (!m_bHasFocus || !Key.IsValid())
	{
		return;
	}
	FKeyState& State = m_Keys[Key.GetCode()];
	if (bDown && !State.m_bDown)
	{
		State.m_bPressed = true;
	}
	else if (!bDown && State.m_bDown)
	{
		State.m_bReleased = true;
	}
	State.m_bDown = bDown;
}

void UPlayerInput::AddAxisMapping(const FInputAxisKeyMapping& Mapping)
{
	if (Mapping.m_AxisName.IsNone() || !Mapping.m_Key.IsValid() || !_finite(Mapping.m_Scale))
	{
		return;
	}
	for (int32 i = 0; i < m_AxisMappings.Num(); i++)
	{
		FInputAxisKeyMapping& Existing = m_AxisMappings[i];
		if (Existing.m_AxisName == Mapping.m_AxisName && Existing.m_Key == Mapping.m_Key)
		{
			Existing.m_Scale = Mapping.m_Scale;
			return;
		}
	}
	m_AxisMappings.Add(Mapping);
}

void UPlayerInput::RemoveAxisMappings(FName AxisName)
{
	for (int32 i = m_AxisMappings.Num() - 1; i >= 0; i--)
	{
		if (m_AxisMappings[i].m_AxisName == AxisName)
		{
			m_AxisMappings.RemoveAt(i);
		}
	}
}

float UPlayerInput::GetAxisValue(FName AxisName) const
{
	if (!m_bHasFocus)
	{
		return 0.0f;
	}
	float Value = 0.0f;
	for (int32 i = 0; i < m_AxisMappings.Num(); i++)
	{
		const FInputAxisKeyMapping& Mapping = m_AxisMappings[i];
		if (Mapping.m_AxisName == AxisName && IsKeyDown(Mapping.m_Key))
		{
			Value += Mapping.m_Scale;
		}
	}
	return Value;
}

bool UPlayerInput::IsKeyDown(FKey Key) const
{
	return Key.IsValid() && m_Keys[Key.GetCode()].m_bDown;
}

bool UPlayerInput::WasKeyPressed(FKey Key) const
{
	return Key.IsValid() && m_Keys[Key.GetCode()].m_bPressed;
}

bool UPlayerInput::WasKeyReleased(FKey Key) const
{
	return Key.IsValid() && m_Keys[Key.GetCode()].m_bReleased;
}
