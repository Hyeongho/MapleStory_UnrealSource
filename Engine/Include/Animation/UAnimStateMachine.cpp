#include "EnginePCH.h"
#include "Animation/UAnimStateMachine.h"
#include "Object/AActor.h"

UAnimStateMachine::UAnimStateMachine()
{
}

UAnimStateMachine::~UAnimStateMachine()
{
	// 등록된 모든 상태(지금 활성 상태가 아닌 것 포함)에 대해 RegisterState()가
	// AddRef해서 보관해온 텍스처를 전부 Release().
	for (auto It = m_States.begin(); It != m_States.end(); ++It)
	{
		for (int32 i = 0; i < It->Value.m_Frames.Num(); i++)
		{
			It->Value.m_Frames[i].m_pTexture->Release();
		}
	}
}

void UAnimStateMachine::RegisterState(FName StateName, const TArray<FFlipbookFrame>& Frames, bool bLoop)
{
	FAnimStateData& Data = m_States.FindOrAdd(StateName);

	// 재등록(같은 이름으로 다시 호출)이면 기존 보관분을 먼저 Release —
	// TArray::Reset()은 COM 레퍼런스를 모르므로 직접 해제해야 한다.
	for (int32 i = 0; i < Data.m_Frames.Num(); i++)
	{
		Data.m_Frames[i].m_pTexture->Release();
	}
	Data.m_Frames.Reset();

	for (int32 i = 0; i < Frames.Num(); i++)
	{
		Frames[i].m_pTexture->AddRef(); // 이 컴포넌트가 수명 동안 보관할 몫
		Data.m_Frames.Add(Frames[i]);
	}

	Data.m_bLoop = bLoop;
}

void UAnimStateMachine::SetState(FName StateName)
{
	if (StateName == m_CurrentState)
	{
		return;
	}

	FAnimStateData* pData = m_States.Find(StateName);
	if (!ensure(pData != nullptr))
	{
		return;
	}

	m_CurrentState = StateName;

	if (m_pFlipbook)
	{
		// SetFrames()가 각 텍스처를 자체적으로 다시 AddRef해서 "재생용 사본"을
		// 만들고, 이전에 갖고 있던 프레임들은 알아서 Release()한다 — 여기서
		// Data.m_Frames(이 컴포넌트의 영구 보관 몫)는 절대 손대지 않는다. 즉
		// "떠나는 상태"를 위한 Release 로직이 SetState()엔 필요 없다.
		m_pFlipbook->SetFrames(pData->m_Frames, pData->m_bLoop);
		m_pFlipbook->Play();
	}
}

void UAnimStateMachine::BeginPlay()
{
	m_pFlipbook = GetOwner() ? GetOwner()->GetComponent<UFlipbookComponent>() : nullptr;
}
