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
		m_Frames.Add(Frames[i]);
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
