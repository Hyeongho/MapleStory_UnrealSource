#include "EnginePCH.h"
#include "Animation/UAnimStateMachine.h"
#include "Animation/UFlipbookComponent.h"
#include "Object/AActor.h"

UAnimStateMachine::UAnimStateMachine() = default;
UAnimStateMachine::~UAnimStateMachine() = default;

bool UAnimStateMachine::RegisterState(FName StateName, const TSharedPtr<UFlipbook>& Flipbook, bool bLoop)
{
	if (!Flipbook)
	{
		UE_LOG(LogRenderer, Warning, L"RegisterState rejected null flipbook");
		return false;
	}

	FAnimStateData& Data = m_States.FindOrAdd(StateName);

	Data.m_Flipbook = Flipbook;
	Data.m_bLoop = bLoop;

	if (StateName == m_CurrentState)
	{
		ApplyCurrentState();
	}
	return true;
}

void UAnimStateMachine::SetState(FName StateName)
{
	if (StateName == m_CurrentState)
	{
		return;
	}
	if (!ensure(m_States.Find(StateName) != nullptr))
	{
		return;
	}
	m_CurrentState = StateName;
	ApplyCurrentState();
}

void UAnimStateMachine::ApplyCurrentState()
{
	const FAnimStateData* pData = m_States.Find(m_CurrentState);
	UFlipbookComponent* pComponent = GetOwner() ? GetOwner()->GetComponent<UFlipbookComponent>() : nullptr;
	if (pData && pComponent)
	{
		// 텍스처별 참조를 변경하지 않고 공유 에셋만 선택한다.
		pComponent->SetFlipbook(pData->m_Flipbook);
		pComponent->SetLooping(pData->m_bLoop);
		pComponent->Play();
	}
}

void UAnimStateMachine::BeginPlay()
{
	ApplyCurrentState();
}

bool UAnimStateMachine::HasState(FName StateName) const
{
	return m_States.Contains(StateName);
}

FName UAnimStateMachine::GetCurrentState() const
{
	return m_CurrentState;
}