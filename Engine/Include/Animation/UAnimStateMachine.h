#pragma once

#include "Object/UActorComponent.h"
#include "Core/Containers/TMap.h"
#include "Core/String/FName.h"
#include "Animation/UFlipbook.h"

class UAnimStateMachine : public UActorComponent
{
	DECLARE_CLASS(UAnimStateMachine, UActorComponent)

public:
	// 생성 / 소멸
	UAnimStateMachine();
	virtual ~UAnimStateMachine() override;

	// 공유 에셋을 상태에 등록한다. 빈 포인터는 거부하고 기존 상태를 유지한다.
	bool RegisterState(FName StateName, const TSharedPtr<UFlipbook>& Flipbook, bool bLoop = true);
	void SetState(FName StateName);
	bool HasState(FName StateName) const;
	FName GetCurrentState() const;

	// 재정의
	virtual void BeginPlay() override;

private:
	void ApplyCurrentState();

	struct FAnimStateData
	{
		TSharedPtr<UFlipbook> m_Flipbook;
		bool m_bLoop = true;
	};
	TMap<FName, FAnimStateData> m_States;
	FName m_CurrentState;
};