#include "EnginePCH.h"
#include "Animation/UFlipbookComponent.h"
#include "Animation/UAnimNotify.h"

UFlipbookComponent::UFlipbookComponent() = default;
UFlipbookComponent::~UFlipbookComponent() = default;

bool UFlipbookComponent::SetFlipbook(const TSharedPtr<UFlipbook>& Flipbook)
{
	if (m_Flipbook == Flipbook)
	{
		return false;
	}
	m_Flipbook = Flipbook;
	m_CurrentFrameIndex = 0;
	m_ElapsedInFrame = 0.0;
	++m_PlaybackRevision;
	if (!HasTexture())
	{
		m_bPlaying = false;
	}
	return true;
}

const TSharedPtr<UFlipbook>& UFlipbookComponent::GetFlipbook() const
{
	return m_Flipbook;
}

const FFlipbookFrame* UFlipbookComponent::GetCurrentFrame() const
{
	return m_Flipbook ? m_Flipbook->GetFrame(m_CurrentFrameIndex) : nullptr;
}

int32 UFlipbookComponent::GetCurrentFrameIndex() const
{
	return m_CurrentFrameIndex;
}

bool UFlipbookComponent::HasTexture() const
{
	return GetCurrentFrame() != nullptr;
}

void UFlipbookComponent::Play()
{
	m_bPlaying = HasTexture();
}

void UFlipbookComponent::PlayFromStart()
{
	m_CurrentFrameIndex = 0;
	m_ElapsedInFrame = 0.0;
	++m_PlaybackRevision;
	Play();
}

void UFlipbookComponent::Stop()
{
	m_bPlaying = false;
}

void UFlipbookComponent::SetLooping(bool bLoop)
{
	m_bLoop = bLoop;
}

bool UFlipbookComponent::IsLooping() const
{
	return m_bLoop;
}

bool UFlipbookComponent::IsPlaying() const
{
	return m_bPlaying && HasTexture();
}

void UFlipbookComponent::SetPlayRate(float Rate)
{
	if (_finite(Rate) && Rate >= 0.0f)
	{
		m_PlayRate = Rate;
	}
}

float UFlipbookComponent::GetPlayRate() const
{
	return m_PlayRate;
}

void UFlipbookComponent::Tick(float DeltaTime)
{
	if (!IsPlaying() || !_finite(DeltaTime) || DeltaTime < 0.0f)
	{
		return;
	}
	m_ElapsedInFrame += (double)DeltaTime * m_PlayRate;
	const uint32 Revision = m_PlaybackRevision;
	const FFlipbookFrame* pCurrent = GetCurrentFrame();
	while (m_ElapsedInFrame >= pCurrent->m_Duration)
	{
		m_ElapsedInFrame -= pCurrent->m_Duration;
		if (m_CurrentFrameIndex + 1 < m_Flipbook->GetNumFrames())
		{
			++m_CurrentFrameIndex;
		}
		else if (m_bLoop)
		{
			m_CurrentFrameIndex = 0;
		}
		else
		{
			m_bPlaying = false;
			m_ElapsedInFrame = pCurrent->m_Duration;
			break;
		}
		pCurrent = GetCurrentFrame();
		if (pCurrent->m_pNotify)
		{
			pCurrent->m_pNotify->Notify(GetOwner());
			// 콜백이 에셋을 교체하면 이전 프레임 포인터를 다시 사용하지 않는다.
			if (Revision != m_PlaybackRevision || !m_bPlaying)
			{
				return;
			}
		}
	}
}

void UFlipbookComponent::Render(FRenderQueue& Queue)
{
	const FFlipbookFrame* pFrame = GetCurrentFrame();
	if (!pFrame)
	{
		return;
	}
	const FTransform2D Transform = GetWorldTransform();
	const float OriginX = m_bFlipHorizontal ? pFrame->m_Origin.m_X : -pFrame->m_Origin.m_X;
	const float Progress = FMath::Clamp((float)(m_ElapsedInFrame / pFrame->m_Duration), 0.0f, 1.0f);
	const int32 FrameAlpha = (int32)(pFrame->m_A0 + (pFrame->m_A1 - pFrame->m_A0) * Progress);

	FRenderQueueEntry Entry;
	Entry.m_pTexture = pFrame->m_pTexture;
	Entry.m_Position = Transform.TransformPoint(FVector2D(OriginX, -pFrame->m_Origin.m_Y));
	Entry.m_Scale = Transform.m_Scale;
	if (m_bFlipHorizontal)
	{
		Entry.m_Scale.m_X *= -1.0f;
	}
	Entry.m_RotationRadians = Transform.m_Rotation;
	Entry.m_Tint = m_Tint;
	Entry.m_Tint.m_A *= ((float)m_Alpha / 255.0f) * ((float)FrameAlpha / 255.0f);
	Entry.m_Layer = m_Layer;
	Entry.m_ContainerOrder = m_ContainerOrder;
	Entry.m_Z0 = m_bUseFrameZ ? pFrame->m_Z : m_Z0;
	Entry.m_Z1 = m_Z1;
	Entry.m_Blend = pFrame->m_bBlend ? EBlendMode::Additive : EBlendMode::NonPremultiplied;
	Queue.Submit(Entry);
}