#include "EnginePCH.h"
#include "Animation/UFlipbookComponent.h"
#include "Object/AActor.h"
#include "Render/USpriteComponent.h"

UFlipbookComponent::UFlipbookComponent()
{
}

UFlipbookComponent::~UFlipbookComponent()
{
	for (int32 i = 0; i < m_Frames.Num(); i++)
	{
		m_Frames[i].m_pTexture->Release();
	}
}

void UFlipbookComponent::SetFrames(const TArray<FFlipbookFrame>& Frames, bool bLoop)
{
	for (int32 i = 0; i < m_Frames.Num(); i++)
	{
		m_Frames[i].m_pTexture->Release();
	}
	m_Frames.Reset();

	for (int32 i = 0; i < Frames.Num(); i++)
	{
		Frames[i].m_pTexture->AddRef();

		FFlipbookFrame Frame = Frames[i];
		if (Frame.m_Duration <= 0.0f)
		{
			// WZ 딜레이(외부 데이터) 값이 0/음수로 들어올 가능성에 대한 방어 —
			// Duration이 0 이하로 남으면 Tick()의 프레임 전환 while 루프가
			// 절대 안 끝나는 무한 루프가 된다(m_ElapsedInFrame이 매 반복 그대로라
			// 조건이 계속 참). 화면에 티도 안 날 만큼 짧은 최소값으로만 올려둔다.
			Frame.m_Duration = 0.001f;
		}
		m_Frames.Add(Frame);
	}

	m_bLoop = bLoop;
	m_CurrentFrameIndex = 0;
	m_ElapsedInFrame = 0.0f;
}

void UFlipbookComponent::Play()
{
	m_bPlaying = m_Frames.Num() > 0;
}

void UFlipbookComponent::Stop()
{
	m_bPlaying = false;
}

void UFlipbookComponent::BeginPlay()
{
	m_pTargetSprite = GetOwner() ? GetOwner()->GetComponent<USpriteComponent>() : nullptr;
}

void UFlipbookComponent::Tick(float DeltaTime)
{
	if (!m_bPlaying || m_Frames.Num() == 0)
	{
		return;
	}

	m_ElapsedInFrame += DeltaTime;

	const FFlipbookFrame* pCurrent = &m_Frames[m_CurrentFrameIndex];
	while (m_ElapsedInFrame >= pCurrent->m_Duration)
	{
		m_ElapsedInFrame -= pCurrent->m_Duration;

		if (m_CurrentFrameIndex + 1 < m_Frames.Num())
		{
			m_CurrentFrameIndex++;
		}
		else if (m_bLoop)
		{
			m_CurrentFrameIndex = 0;
		}
		else
		{
			m_bPlaying = false;
			m_ElapsedInFrame = 0.0f;
			break;
		}

		pCurrent = &m_Frames[m_CurrentFrameIndex];
	}

	if (m_pTargetSprite)
	{
		pCurrent->m_pTexture->AddRef();
		m_pTargetSprite->SetTexture(pCurrent->m_pTexture, pCurrent->m_Origin);
	}
}
