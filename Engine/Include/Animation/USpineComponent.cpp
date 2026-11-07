#include "EnginePCH.h"
#include "Animation/USpineComponent.h"

USpineComponent::USpineComponent() = default;
USpineComponent::~USpineComponent() = default;

void USpineComponent::SetAnimation(const TSharedPtr<FWzSpineAnimation>& Animation)
{
	m_Animation = Animation;
}

void USpineComponent::SetMapOrder(ELayer Layer, int32 Z0, int32 Z1)
{
	m_Layer = Layer;
	m_Z0 = Z0;
	m_Z1 = Z1;
}

void USpineComponent::SetFlipHorizontal(bool bFlip)
{
	m_bFlipHorizontal = bFlip;
}

void USpineComponent::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (m_Animation)
	{
		m_Animation->Tick(DeltaTime);
	}
}

void USpineComponent::Render(FRenderQueue& Queue)
{
	if (!m_Animation)
	{
		return;
	}
	const FTransform2D Transform = GetWorldTransform();
	FRenderQueueEntry Entry;
	Entry.m_pSpine = m_Animation.Get();
	Entry.m_Position = Transform.m_Location;
	Entry.m_Scale = Transform.m_Scale;
	if (m_bFlipHorizontal)
	{
		Entry.m_Scale.m_X *= -1.0f;
	}
	Entry.m_RotationRadians = Transform.m_Rotation;
	Entry.m_Layer = m_Layer;
	Entry.m_Z0 = m_Z0;
	Entry.m_Z1 = m_Z1;
	Queue.Submit(Entry);
}
