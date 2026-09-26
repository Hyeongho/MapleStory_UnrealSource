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
}
