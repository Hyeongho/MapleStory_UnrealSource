#pragma once
#include "Object/UActorComponent.h"
#include "Core/Containers/TMap.h"
#include "Core/String/FName.h"
#include "Animation/UFlipbookComponent.h" // FFlipbookFrame 정의 필요

// 이름(FName)으로 구분된 애니메이션 상태(프레임 시퀀스 + 루프 여부) 묶음을
// 등록해두고, SetState(Name) 호출 한 번으로 형제 UFlipbookComponent의 재생
// 목록을 갈아끼우는 상태 전환기. Input 시스템(Phase 13)이 아직 없어서 지금은
// 실제 게임플레이가 SetState()를 호출하는 곳이 없다 — main.cpp 데모 코드나
// 나중의 Input 핸들러가 직접 호출해주는, 순수 이벤트 구동(Tick 없음)
// 컴포넌트다.
//
// 붙이는 순서 주의: AddComponent<T>()는 액터가 이미 BeginPlay를 마쳤으면
// 새 컴포넌트에 즉시 BeginPlay()를 호출한다(AActor.h 참고) — 이
// UAnimStateMachine::BeginPlay()는 형제 UFlipbookComponent를 그 시점에
// GetComponent<T>()로 찾아 캐싱하므로, 반드시 UFlipbookComponent를 먼저
// AddComponent한 뒤에 UAnimStateMachine을 붙여야 한다. 반대 순서면 형제가
// 아직 m_Components에 없어서 캐시가 영구히 nullptr로 굳는다.
class UAnimStateMachine : public UActorComponent
{
	DECLARE_CLASS(UAnimStateMachine, UActorComponent)
public:
	UAnimStateMachine();
	virtual ~UAnimStateMachine() override;

	// StateName으로 재생할 프레임 목록을 등록(또는 갱신)한다.
	// 소유권 계약(UFlipbookComponent::SetFrames()와 같은 패턴을 한 단계 위에서
	// 반복): Frames의 각 텍스처를 AddRef해서 이 컴포넌트가 "수명 동안, 등록된
	// 모든 상태에 대해 동시에" 별도 보관한다 — 지금 활성 상태인지 여부와
	// 무관하다. 호출자가 넘긴 원본 배열의 레퍼런스는 호출 후 자기 몫을
	// Release()해도 안전. 이미 같은 이름으로 등록된 상태가 있으면 기존에
	// 보관 중이던 텍스처를 먼저 전부 Release()한 뒤 새 목록으로 통째로
	// 교체한다.
	void RegisterState(FName StateName, const TArray<FFlipbookFrame>& Frames, bool bLoop = true);

	// StateName으로 전환한다. 이미 그 상태면 아무 것도 안 함(no-op) — 매
	// 호출마다 같은 애니메이션을 처음부터 재시작하지 않기 위함. 등록 안 된
	// 이름이면 ensure() 경고만 남기고 조용히 무시(예외 없는 엔진이라
	// 크래시하지 않음).
	void SetState(FName StateName);

	FName GetCurrentState() const { return m_CurrentState; }

	virtual void BeginPlay() override;

private:
	struct FAnimStateData
	{
		TArray<FFlipbookFrame> m_Frames; // 소유 — 각 텍스처 AddRef된 채 보관
		bool m_bLoop = true;
	};

	TMap<FName, FAnimStateData> m_States;
	UFlipbookComponent* m_pFlipbook = nullptr; // 캐시, 비소유
	FName m_CurrentState; // 기본 생성 시 IsNone() — SetState() 호출 전
};
