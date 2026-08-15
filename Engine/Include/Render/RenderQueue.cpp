#include "EnginePCH.h"
#include "RenderQueue.h"
#include "SpriteBatch.h"
#include "Render/FCamera2D.h"

FRenderQueue* GRenderQueue = nullptr;

FRenderQueue::FRenderQueue() = default;
FRenderQueue::~FRenderQueue() = default;

void FRenderQueue::Submit(const FRenderQueueEntry& Entry)
{
	m_Entries.Add(Entry);
}

void FRenderQueue::SubmitSprite(ID3D11ShaderResourceView* pTexture, const FVector2D& Position, int32 ZOrder, const FVector2D& Scale, float RotationRadians, const FLinearColor& Tint, ELayer Layer, float ParallaxFactor)
{
	FRenderQueueEntry Entry;
	Entry.m_pTexture = pTexture;
	Entry.m_Position = Position;
	Entry.m_Scale = Scale;
	Entry.m_RotationRadians = RotationRadians;
	Entry.m_Tint = Tint;
	Entry.m_ZOrder = ZOrder;
	Entry.m_Layer = Layer;
	Entry.m_ParallaxFactor = ParallaxFactor;

	m_Entries.Add(Entry);
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

		FVector2D DrawPosition = Entry.m_Position + GCamera2D->GetLocation() * (1.0f - Entry.m_ParallaxFactor);

		SpriteBatch.DrawSprite(Entry.m_pTexture, DrawPosition, Entry.m_Scale, Entry.m_RotationRadians, Entry.m_Tint);
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

void FRenderQueue::SortEntries()
{
	m_Entries.StableSort([](const FRenderQueueEntry& A, const FRenderQueueEntry& B)
		{
			if (A.m_Layer != B.m_Layer)
			{
				return A.m_Layer < B.m_Layer;
			}

			return A.m_ZOrder < B.m_ZOrder;
		});
}
