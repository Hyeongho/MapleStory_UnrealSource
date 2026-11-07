#pragma once

#include "Object/UActorComponent.h"
#include "Core/Containers/TMap.h"
#include "Core/String/FName.h"
#include "Animation/UFlipbook.h"

class UAnimStateMachine :
    public UActorComponent
{
	DECLARE_CLASS(UAnimStateMachine, UActorComponent)
public:
	UAnimStateMachine();
	virtual ~UAnimStateMachine() override;

	// 공유 에셋을 상태에 등록한다. 빈 포인터는 거부하고 기존 상태를 유지한다.
	bool RegisterState(FName StateName, const TSharedPtr<UFlipbook>& Flipbook, bool bLoop = true);

	// StateName으로 전환한다. 이미 그 상태면 아무 것도 안 함(no-op) — 매
	// 호출마다 같은 애니메이션을 처음부터 재시작하지 않기 위함. 등록 안 된
	// 이름이면 ensure() 경고만 남기고 조용히 무시(예외 없는 엔진이라
	// 크래시하지 않음).
	void SetState(FName StateName);
	bool HasState(FName StateName) const;

	FName GetCurrentState() const { return m_CurrentState; }

	virtual void BeginPlay() override;

private:
	void ApplyCurrentState();

private:
	struct FAnimStateData
	{
		TSharedPtr<UFlipbook> m_Flipbook;
		bool m_bLoop = true;
	};

	TMap<FName, FAnimStateData> m_States;

	FName m_CurrentState; // 기본 생성 시 IsNone() — SetState() 호출 전
};

