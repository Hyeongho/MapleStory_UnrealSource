#include "EnginePCH.h"
#include "Animation/UFlipbook.h"

UFlipbook::UFlipbook() = default;

UFlipbook::~UFlipbook()
{
	for (int32 i = 0; i < m_Frames.Num(); i++)
	{
		m_Frames[i].m_pTexture->Release();
	}
}

TSharedPtr<UFlipbook> UFlipbook::Create(const TArray<FFlipbookFrame>& Frames)
{
	for (int32 i = 0; i < Frames.Num(); i++)
	{
		if (!Frames[i].m_pTexture)
		{
			UE_LOG(LogRenderer, Warning, L"Flipbook rejected null texture at frame %d", i);
			return TSharedPtr<UFlipbook>();
		}
	}

	TSharedPtr<UFlipbook> Flipbook = MakeShared<UFlipbook>();
	Flipbook->m_Frames.Reserve(Frames.Num());

	for (int32 i = 0; i < Frames.Num(); i++)
	{
		FFlipbookFrame Frame = Frames[i];

		if (!_finite(Frame.m_Duration) || Frame.m_Duration <= 0.0f)
		{
			Frame.m_Duration = 0.001f;
		}

		Frame.m_pTexture->AddRef();
		Flipbook->m_TotalDuration += Frame.m_Duration;
		Flipbook->m_Frames.Add(Frame);
	}

	return Flipbook;
}

int32 UFlipbook::GetNumFrames() const
{
	return m_Frames.Num();
}

const FFlipbookFrame* UFlipbook::GetFrame(int32 FrameIndex) const
{
	return FrameIndex >= 0 && FrameIndex < m_Frames.Num() ? &m_Frames[FrameIndex] : nullptr;
}

double UFlipbook::GetTotalDuration() const
{
	return m_TotalDuration;
}