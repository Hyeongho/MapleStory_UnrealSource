#include "EnginePCH.h"
#include "Render/RenderQueue.h"
#include "Render/SpriteBatch.h"

FRenderQueue* GRenderQueue = nullptr;

FRenderQueue::FRenderQueue() = default;
FRenderQueue::~FRenderQueue() = default;

void FRenderQueue::Submit(const FRenderQueueEntry& Entry)
{
	m_Entries.Add(Entry);
}

void FRenderQueue::SubmitSprite(
	ID3D11ShaderResourceView* pTexture,
	const FVector2D& Position,
	int32 ZOrder,
	const FVector2D& Scale,
	float RotationRadians,
	const FLinearColor& Tint,
	ELayer Layer)
{
	FRenderQueueEntry Entry;
	Entry.m_pTexture = pTexture;
	Entry.m_Position = Position;
	Entry.m_Scale = Scale;
	Entry.m_RotationRadians = RotationRadians;
	Entry.m_Tint = Tint;
	Entry.m_ZOrder = ZOrder;
	Entry.m_Layer = Layer;

	m_Entries.Add(Entry);
}

void FRenderQueue::SortEntries()
{
	// StableSort — Layer가 1차 키, 같은 Layer 안에서는 ZOrder가 2차 키.
	// 둘 다 같으면 제출한 순서를 유지한다.
	m_Entries.StableSort([](const FRenderQueueEntry& A, const FRenderQueueEntry& B)
		{
			if (A.m_Layer != B.m_Layer)
			{
				return A.m_Layer < B.m_Layer;
			}
			return A.m_ZOrder < B.m_ZOrder;
		});
}

void FRenderQueue::Flush(FSpriteBatch& SpriteBatch)
{
	SortEntries();

	for (int32 i = 0; i < m_Entries.Num(); i++)
	{
		const FRenderQueueEntry& Entry = m_Entries[i];
		if (Entry.m_Layer == ELayer::UI)
		{
			continue; // UI는 화면 좌표라 FlushUI()에서 항등 변환으로 따로 그린다.
		}
		SpriteBatch.DrawSprite(Entry.m_pTexture, Entry.m_Position, Entry.m_Scale, Entry.m_RotationRadians, Entry.m_Tint);
	}
}

void FRenderQueue::FlushUI(FSpriteBatch& SpriteBatch)
{
	SortEntries();

	for (int32 i = 0; i < m_Entries.Num(); i++)
	{
		const FRenderQueueEntry& Entry = m_Entries[i];
		if (Entry.m_Layer != ELayer::UI)
		{
			continue;
		}
		SpriteBatch.DrawSprite(Entry.m_pTexture, Entry.m_Position, Entry.m_Scale, Entry.m_RotationRadians, Entry.m_Tint);
	}

	Clear();
}

void FRenderQueue::Clear()
{
	m_Entries.Reset();
}
