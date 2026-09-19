#pragma once

#include "EnginePCH.h"
#include "Core/Containers/TArray.h"
#include "Core/Math/FVector2D.h"
#include "Core/Math/FLinearColor.h"
#include "Render/SpriteBatch.h"

class FSpriteBatch;

// 그리는 순서는 MapRender의 씬 컨테이너 순서를 그대로 따른다
// (MapScene.cs:11-25 → Back → Layers 0~7 → Fly → Front → Effect).
// 레퍼런스는 "컨테이너별로 따로 정렬"하고 컨테이너끼리는 절대 섞이지 않는데,
// 우리는 큐 하나에 전부 모이므로 컨테이너 정체성을 이 enum 값에 담아
// 정렬 1순위로 쓴다 — 결과 순서는 동일하다.
//
// 레이어 안쪽 순서도 레퍼런스와 같다(LayerNode = Obj, Reactor, Tile 순).
enum class ELayer : uint8
{
	Background = 0, // Scene.Back — back 중 front=0

	// 맵 레이어 0~7은 10~33을 쓴다(아래 MakeMapXxxLayer 참고).

	// 게임플레이 스프라이트(캐릭터 등) 기본값 — 맵 레이어 전부보다 앞,
	// 포털보다는 뒤. life(몹/NPC)를 레퍼런스처럼 발판 레이어에 넣는 건
	// 아직 범위 밖이라 단일 값으로 둔다.
	Object = 35,

	Portal = 40, // Scene.Fly.Portal
	Front = 50,  // Scene.Front — back 중 front=1
	Effect = 60,
	UI = 70,

	// 이전 이름 유지 — front=1 back을 가리키던 값이라 Front와 같다.
	BackFront = Front,
};

// 맵 레이어 N(0~7)은 Obj / Reactor / Tile 세 칸을 차지한다 — 레이어 사이 순서가
// 레이어 안쪽 순서를 항상 지배하고, 안쪽은 레퍼런스 LayerNode와 같은 순서다.
constexpr int32 MAP_LAYER_BASE = 10;
constexpr int32 MAP_LAYER_STRIDE = 3;

inline ELayer MakeMapObjLayer(int32 LayerIndex)
{
	return (ELayer)(MAP_LAYER_BASE + LayerIndex * MAP_LAYER_STRIDE);
}

inline ELayer MakeMapReactorLayer(int32 LayerIndex)
{
	return (ELayer)(MAP_LAYER_BASE + LayerIndex * MAP_LAYER_STRIDE + 1);
}

inline ELayer MakeMapTileLayer(int32 LayerIndex)
{
	return (ELayer)(MAP_LAYER_BASE + LayerIndex * MAP_LAYER_STRIDE + 2);
}

struct FRenderQueueEntry
{
	ID3D11ShaderResourceView* m_pTexture = nullptr; // non-owning
	FVector2D m_Position;
	FVector2D m_Scale = FVector2D(1.0f, 1.0f);
	float m_RotationRadians = 0.0f;
	FLinearColor m_Tint = FLinearColor::White;

	// 정렬 키 — 레퍼런스 MeshItem의 Z0/Z1과 같은 의미.
	// Z0: back=0, obj=WZ의 z, tile=캔버스의 z, 그 외=프레임 z
	// Z1: WZ 슬롯 번호(배열 순서가 아님)
	int32 m_Z0 = 0;
	int32 m_Z1 = 0;

	ELayer m_Layer = ELayer::Object;
	EBlendMode m_Blend = EBlendMode::NonPremultiplied;

	float m_ParallaxFactor = 1.0f; // 1.0 = 카메라와 완전히 같이 움직임(기본)

	// 타일링 반복(back 전용) — [m_TileL, m_TileR) × [m_TileT, m_TileB) 범위의
	// (x, y)마다 m_Position + m_TileOffset * (x, y)에 한 번씩 그린다.
	// 기본값은 "원점에 한 번만"이라 일반 스프라이트는 신경 쓸 필요가 없다.
	FVector2D m_TileOffset = FVector2D::Zero;
	int32 m_TileL = 0, m_TileT = 0, m_TileR = 1, m_TileB = 1;
};

class FRenderQueue
{
public:
	FRenderQueue();
	~FRenderQueue();

	void Submit(const FRenderQueueEntry& Entry);
	void SubmitSprite(ID3D11ShaderResourceView* pTexture, const FVector2D& Position, int32 ZOrder, const FVector2D& Scale = FVector2D(1.0f, 1.0f), float RotationRadians = 0.0f, const FLinearColor& Tint = FLinearColor::White, ELayer Layer = ELayer::Object, float ParallaxFactor = 1.0f);

	// Transform은 월드 좌표 패스에 쓸 카메라 행렬 — 블렌드 상태가 바뀌는
	// 지점마다 배치를 다시 열어야 해서 Flush가 직접 Begin/End를 관리한다.
	void Flush(FSpriteBatch& SpriteBatch);
	void FlushUI(FSpriteBatch& SpriteBatch);
	void Clear();

	int32 Num() const
	{
		return m_Entries.Num();
	}

private:
	void SortEntries();

	TArray<FRenderQueueEntry> m_Entries;
};

extern FRenderQueue* GRenderQueue;
