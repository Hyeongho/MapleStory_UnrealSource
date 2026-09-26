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
	Entry.m_Z0 = ZOrder;
	Entry.m_Layer = Layer;
	Entry.m_ParallaxFactor = ParallaxFactor;

	m_Entries.Add(Entry);
}

void FRenderQueue::Flush(FSpriteBatch& SpriteBatch)
{
	SortEntries();

	DirectX::XMMATRIX Transform = GCamera2D->GetViewMatrix();

	// DirectXTK는 블렌드 상태를 Begin에서만 받으므로, 블렌드가 바뀌는 지점마다
	// 배치를 끊는다. 이미 정렬된 배열을 순서대로 훑으며 끊는 것이라 같은 블렌드
	// 구간은 연속이고, 배치를 나눠도 그리는 순서 자체는 그대로 유지된다.
	bool bBatchOpen = false;
	EBlendMode CurrentBlend = EBlendMode::NonPremultiplied;

	for (int32 i = 0; i < m_Entries.Num(); i++)
	{
		const FRenderQueueEntry& Entry = m_Entries[i];

		if (Entry.m_Layer == ELayer::UI)
		{
			continue; // UI는 화면 좌표라 FlushUI()에서 항등 변환으로 따로 그린다.
		}

		if (!bBatchOpen || Entry.m_Blend != CurrentBlend)
		{
			if (bBatchOpen)
			{
				SpriteBatch.End();
			}

			CurrentBlend = Entry.m_Blend;
			SpriteBatch.Begin(Transform, CurrentBlend);
			bBatchOpen = true;
		}

		FVector2D BasePosition = Entry.m_Position + GCamera2D->GetLocation() * (1.0f - Entry.m_ParallaxFactor);

		// 기본값(L,T,R,B = 0,0,1,1)이면 원점에 한 번만 그려서 기존 동작과 같다.
		for (int32 TileY = Entry.m_TileT; TileY < Entry.m_TileB; TileY++)
		{
			for (int32 TileX = Entry.m_TileL; TileX < Entry.m_TileR; TileX++)
			{
				FVector2D DrawPosition = BasePosition + FVector2D(Entry.m_TileOffset.m_X * (float)TileX, Entry.m_TileOffset.m_Y * (float)TileY);

				SpriteBatch.DrawSprite(Entry.m_pTexture, DrawPosition, Entry.m_Scale, Entry.m_RotationRadians, Entry.m_Tint);
			}
		}
	}

	if (bBatchOpen)
	{
		SpriteBatch.End();
	}
}

void FRenderQueue::FlushUI(FSpriteBatch& SpriteBatch)
{
	SortEntries();

	bool bBatchOpen = false;
	EBlendMode CurrentBlend = EBlendMode::NonPremultiplied;

	for (int32 i = 0; i < m_Entries.Num(); i++)
	{
		const FRenderQueueEntry& Entry = m_Entries[i];

		if (Entry.m_Layer != ELayer::UI)
		{
			continue;
		}

		if (!bBatchOpen || Entry.m_Blend != CurrentBlend)
		{
			if (bBatchOpen)
			{
				SpriteBatch.End();
			}

			CurrentBlend = Entry.m_Blend;
			SpriteBatch.Begin(DirectX::XMMatrixIdentity(), CurrentBlend);
			bBatchOpen = true;
		}

		SpriteBatch.DrawSprite(Entry.m_pTexture, Entry.m_Position, Entry.m_Scale, Entry.m_RotationRadians, Entry.m_Tint);
	}

	Clear();
}

void FRenderQueue::Clear()
{
	m_Entries.Reset();
}

const TArray<FRenderQueueEntry>& FRenderQueue::GetSortedEntries()
{
	SortEntries();
	return m_Entries;
}

void FRenderQueue::SortEntries()
{
	m_Entries.StableSort([](const FRenderQueueEntry& A, const FRenderQueueEntry& B)
		{
			if (A.m_Layer != B.m_Layer)
			{
				return A.m_Layer < B.m_Layer;
			}

			if (A.m_ContainerOrder != B.m_ContainerOrder)
			{
				return A.m_ContainerOrder < B.m_ContainerOrder;
			}

			if (A.m_Z0 != B.m_Z0)
			{
				return A.m_Z0 < B.m_Z0;
			}

			return A.m_Z1 < B.m_Z1;
		});
}
