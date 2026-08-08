#pragma once
#include "EnginePCH.h"
#include "Core/Containers/TArray.h"
#include "Core/Math/FVector2D.h"
#include "Core/Math/FLinearColor.h"
#include <d3d11.h>

class FSpriteBatch;

// 배경/오브젝트/이펙트는 월드 좌표(카메라 변환 적용), UI는 화면 좌표(항등 변환) —
// 값 순서가 그대로 정렬 우선순위(배경이 가장 먼저 그려짐).
enum class ELayer : uint8
{
	Background = 0,
	Object     = 1,
	Effect     = 2,
	UI         = 3,
};

struct FRenderQueueEntry
{
	ID3D11ShaderResourceView* m_pTexture = nullptr; // non-owning
	FVector2D m_Position;
	FVector2D m_Scale = FVector2D(1.0f, 1.0f);
	float m_RotationRadians = 0.0f;
	FLinearColor m_Tint = FLinearColor::White;
	int32 m_ZOrder = 0;
	ELayer m_Layer = ELayer::Object;
};

// 매 프레임 그릴 스프라이트를 모았다가 (Layer, ZOrder) 순으로 정렬해 그리는 큐.
// Flush()/FlushUI()는 SpriteBatch.Begin()/End()를 직접 호출하지 않는다 — 프레임 브래킷은 호출자 책임.
// Flush()는 Background/Object/Effect(월드 좌표, 카메라 변환)를, FlushUI()는 UI(화면 좌표, 항등 변환)를 그린다.
// 한 프레임에 Flush() → FlushUI() 순서로 둘 다 호출해야 큐가 비워진다(FlushUI()가 마지막에 Clear() 호출).
class FRenderQueue
{
public:
	FRenderQueue();
	~FRenderQueue();

	void Submit(const FRenderQueueEntry& Entry);
	void SubmitSprite(
		ID3D11ShaderResourceView* pTexture,
		const FVector2D& Position,
		int32 ZOrder,
		const FVector2D& Scale = FVector2D(1.0f, 1.0f),
		float RotationRadians = 0.0f,
		const FLinearColor& Tint = FLinearColor::White,
		ELayer Layer = ELayer::Object);

	void Flush(FSpriteBatch& SpriteBatch);
	void FlushUI(FSpriteBatch& SpriteBatch);
	void Clear();

	int32 Num() const { return m_Entries.Num(); }

private:
	void SortEntries();

	TArray<FRenderQueueEntry> m_Entries;
};

extern FRenderQueue* GRenderQueue;
