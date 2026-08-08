#include "EnginePCH.h"
#include "RenderQueue.h"
#include "SpriteBatch.h"

FRenderQueue* GRenderQueue = nullptr;

FRenderQueue::FRenderQueue() = default;
FRenderQueue::~FRenderQueue() = default;

void FRenderQueue::Submit(const FRenderQueueEntry& Entry)
{
	m_Entries.Add(Entry);
}

void FRenderQueue::SubmitSprite( ID3D11ShaderResourceView* pTexture, const FVector2D& Position, int32 ZOrder, const FVector2D& Scale, float RotationRadians, const FLinearColor& Tint)
{
	FRenderQueueEntry Entry;
	Entry.m_pTexture = pTexture;
	Entry.m_Position = Position;
	Entry.m_Scale = Scale;
	Entry.m_RotationRadians = RotationRadians;
	Entry.m_Tint = Tint;
	Entry.m_ZOrder = ZOrder;

	m_Entries.Add(Entry);
}

void FRenderQueue::Flush(FSpriteBatch& SpriteBatch)
{
	// StableSort — 같은 ZOrder를 가진 항목끼리는 제출한 순서를 유지한다.
	m_Entries.StableSort([](const FRenderQueueEntry& A, const FRenderQueueEntry& B)
		{
			return A.m_ZOrder < B.m_ZOrder;
		});

	for (int32 i = 0; i < m_Entries.Num(); i++)
	{
		const FRenderQueueEntry& Entry = m_Entries[i];
		SpriteBatch.DrawSprite(Entry.m_pTexture, Entry.m_Position, Entry.m_Scale, Entry.m_RotationRadians, Entry.m_Tint);
	}

	Clear();
}

void FRenderQueue::Clear()
{
	m_Entries.Reset();
}