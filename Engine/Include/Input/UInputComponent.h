#pragma once

#include "EnginePCH.h"
#include "Object/UActorComponent.h"
#include "Core/String/FName.h"
#include "Core/Containers/TArray.h"
#include "Input/InputDelegates.h"
#include "Input/InputTypes.h"

class UPlayerInput;

class UInputComponent :
    public UActorComponent
{
    DECLARE_CLASS(UInputComponent, UActorComponent)

public:
    UInputComponent();
    virtual ~UInputComponent() override;

	// 축과 동작 함수 연결 / 해제
	void BindAxis(FName AxisName, const FInputAxisDelegate& Delegate);
	void RemoveAxisBindings(FName AxisName);
	void ClearAxisBindings();

	template<typename T, void(T::* Method)(float)>
	void BindAxis(FName AxisName, T* pObject)
	{
		BindAxis(AxisName, FInputAxisDelegate::CreateRaw<T, Method>(pObject));
	}

	void BindAction(FName ActionName, EInputEvent InputEvent, const FInputActionDelegate& Delegate);
	void RemoveActionBindings(FName ActionName);
	void ClearActionBindings();

	template<typename T, void(T::* Method)()>
	void BindAction(FName ActionName, EInputEvent InputEvent, T* pObject)
	{
		BindAction(ActionName, InputEvent, FInputActionDelegate::CreateRaw<T, Method>(pObject));
	}

	// 컨트롤러가 월드 갱신 전에 호출한다. 축은 입력이 없을 때도 0을 전달한다.
	void ProcessInput(const UPlayerInput& PlayerInput);

private:
	struct FInputAxisBinding
	{
		FName m_AxisName;
		FInputAxisDelegate m_Delegate;
	};

	struct FInputActionBinding
	{
		FName m_ActionName;
		EInputEvent m_InputEvent = EInputEvent::Pressed;
		FInputActionDelegate m_Delegate;
	};

	TArray<FInputActionBinding, TInlineAllocator<4>> m_ActionBindings;
	TArray<FInputAxisBinding, TInlineAllocator<4>> m_AxisBindings;
};

