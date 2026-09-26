#pragma once

#include "EnginePCH.h"
#include "Object/UActorComponent.h"
#include "Core/String/FName.h"
#include "Core/Containers/TArray.h"
#include "Input/InputDelegates.h"

class UPlayerInput;

// 키 해석은 UPlayerInput에 맡기고 이름이 있는 입력 축을 게임 함수에 연결한다.
class UInputComponent : public UActorComponent
{
	DECLARE_CLASS(UInputComponent, UActorComponent)

public:
	// 생성 / 소멸
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

	// 컨트롤러가 월드 갱신 전에 호출한다. 입력이 없을 때도 0을 전달한다.
	void ProcessInput(const UPlayerInput& PlayerInput);

private:
	struct FInputAxisBinding
	{
		FName m_AxisName;
		FInputAxisDelegate m_Delegate;
	};

	TArray<FInputAxisBinding, TInlineAllocator<4>> m_AxisBindings;
};
