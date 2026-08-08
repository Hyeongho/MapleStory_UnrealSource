#pragma once

#include "EnginePCH.h"
#include "Core/Containers/TArray.h"
#include "Core/Math/FVector2D.h"
#include "Core/Math/FLinearColor.h"
#include <d3d11.h>

class FSpriteBatch;

struct FRenderQueueEntry
{
	ID3D11ShaderResourceView* m_pTexture = nullptr; // non-owning
	FVector2D m_Position;
	FVector2D m_Scale = FVector2D(1.0f, 1.0f);
	float m_RotationRadians = 0.0f;
	FLinearColor m_Tint = FLinearColor::White;
	int32 m_ZOrder = 0;
};

class FRenderQueue
{
public:
	FRenderQueue();
	~FRenderQueue();

	void Submit(const FRenderQueueEntry& Entry);
	void SubmitSprite(ID3D11ShaderResourceView* pTexture, const FVector2D& Position, int32 ZOrder, const FVector2D& Scale = FVector2D(1.0f, 1.0f), float RotationRadians = 0.0f, const FLinearColor& Tint = FLinearColor::White);

	void Flush(FSpriteBatch& SpriteBatch);
	void Clear();

	int32 Num() const 
	{ 
		return m_Entries.Num(); 
	}

private:
	TArray<FRenderQueueEntry> m_Entries;
};

extern FRenderQueue* GRenderQueue;
