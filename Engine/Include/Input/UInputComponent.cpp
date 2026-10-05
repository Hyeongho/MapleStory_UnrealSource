#include "EnginePCH.h"
#include "Input/UInputComponent.h"
#include "Input/UPlayerInput.h"

UInputComponent::UInputComponent() = default;

UInputComponent::~UInputComponent() = default;

void UInputComponent::BindAxis(FName AxisName, const FInputAxisDelegate& Delegate)
{
	if (AxisName.IsNone() || !Delegate.IsBound())
	{
		return;
	}
	FInputAxisBinding Binding;
	Binding.m_AxisName = AxisName;
	Binding.m_Delegate = Delegate;
	m_AxisBindings.Add(Binding);
}

void UInputComponent::RemoveAxisBindings(FName AxisName)
{
	for (int32 i = m_AxisBindings.Num() - 1; i >= 0; i--)
	{
		if (m_AxisBindings[i].m_AxisName == AxisName)
		{
			m_AxisBindings.RemoveAt(i);
		}
	}
}

void UInputComponent::ClearAxisBindings()
{
	m_AxisBindings.Empty();
}

void UInputComponent::BindAction(FName ActionName, EInputEvent InputEvent, const FInputActionDelegate& Delegate)
{
	if (ActionName.IsNone() || !Delegate.IsBound())
	{
		return;
	}
	FInputActionBinding Binding;
	Binding.m_ActionName = ActionName;
	Binding.m_InputEvent = InputEvent;
	Binding.m_Delegate = Delegate;
	m_ActionBindings.Add(Binding);
}

void UInputComponent::RemoveActionBindings(FName ActionName)
{
	for (int32 i = m_ActionBindings.Num() - 1; i >= 0; i--)
	{
		if (m_ActionBindings[i].m_ActionName == ActionName)
		{
			m_ActionBindings.RemoveAt(i);
		}
	}
}

void UInputComponent::ClearActionBindings()
{
	m_ActionBindings.Empty();
}

void UInputComponent::ProcessInput(const UPlayerInput& PlayerInput)
{
	// 콜백이 바인딩을 변경해도 현재 프레임의 순회가 무효화되지 않게 복사한다.
	// 기본 두 축은 인라인 공간을 사용하므로 프레임마다 힙 할당하지 않는다.
	const TArray<FInputAxisBinding, TInlineAllocator<4>> Bindings = m_AxisBindings;
	for (int32 i = 0; i < Bindings.Num(); i++)
	{
		const FInputAxisBinding& Binding = Bindings[i];
		Binding.m_Delegate.Execute(PlayerInput.GetAxisValue(Binding.m_AxisName));
	}

	// 동작도 스냅샷을 순회해 콜백에서 바인딩을 수정할 수 있게 한다.
	const TArray<FInputActionBinding, TInlineAllocator<4>> Actions = m_ActionBindings;
	for (int32 i = 0; i < Actions.Num(); i++)
	{
		const FInputActionBinding& Binding = Actions[i];
		const bool bTriggered = Binding.m_InputEvent == EInputEvent::Pressed
			? PlayerInput.WasActionPressed(Binding.m_ActionName)
			: PlayerInput.WasActionReleased(Binding.m_ActionName);
		if (bTriggered)
		{
			Binding.m_Delegate.Execute();
		}
	}
}
